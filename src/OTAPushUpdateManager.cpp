#include "OTAPushUpdateManager.h"
#include "OTAManager.h"
#include "handlers/StaticFileHandler.h"
#include "handlers/WebAssetManager.h"
#include "webPage/css.h"
#include "webPage/favicon.h"
// #include "webPage/filesystem.h"
#include "webPage/scripts.h"
#include "webPage/system.h"
#include "webPage/update.h"
#include <algorithm>
#include <vector>

// ============ INICIALIZAÇÃO DE VARIÁVEIS ESTÁTICAS ============

AsyncWebServer *OTAPushUpdateManager::_server = nullptr;
bool OTAPushUpdateManager::_updating = false;
String OTAPushUpdateManager::_username = "";
String OTAPushUpdateManager::_password = "";
bool OTAPushUpdateManager::_authenticated = false;
bool OTAPushUpdateManager::_running = false;
String OTAPushUpdateManager::_mdnsHostname = "";

bool (*OTAPushUpdateManager::_pullUpdateAvailableCallback)() = nullptr;
void (*OTAPushUpdateManager::_performUpdateCallback)() = nullptr;

// FreeRTOS
TaskHandle_t OTAPushUpdateManager::_webPageTaskHandle = nullptr;
bool OTAPushUpdateManager::_taskRunning = false;
uint32_t OTAPushUpdateManager::_taskStackSize = 8192;
UBaseType_t OTAPushUpdateManager::_taskPriority = 1;

// NTP Client
WiFiUDP *OTAPushUpdateManager::_ntpUDP = nullptr;
NTPClient *OTAPushUpdateManager::_timeClient = nullptr;
bool OTAPushUpdateManager::_timeSynced = false;

// ============ IMPLEMENTAÇÃO DOS MÉTODOS ============

void OTAPushUpdateManager::begin(uint16_t port)
{
    if (_server == nullptr)
    {
        _server = new AsyncWebServer(port);
    }

    if (!initLittleFS())
    {
        LOG_ERROR("❌ Falha crítica: LittleFS não pode ser inicializado");
        return;
    }

    // Configura o timezone e servidor NTP do sistema
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    // Inicializa NTP Client para uso interno
    if (_ntpUDP == nullptr)
    {
        _ntpUDP = new WiFiUDP();
        _timeClient = new NTPClient(*_ntpUDP, "pool.ntp.org", -3 * 3600, 60000);
        _timeClient->begin();

        if (_timeClient->forceUpdate())
        {
            _timeSynced = true;
            LOG_INFO("NTP Client sincronizado com sucesso");
        }
        else
        {
            LOG_ERROR("Falha na sincronização NTP inicial");
        }
    }

    // ============ CONFIGURAÇÃO DAS ROTAS ============
    _server->on("/toggle-theme", HTTP_GET, handleToggleTheme);
    // Upload de firmware
    _server->on("/doUpdate", HTTP_POST, [](AsyncWebServerRequest *request)
                { request->send(200, "text/plain", "Upload processed"); }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
                { handleDoUpload(request, filename, index, data, len, final); });

    // Rotas de teste
    _server->on("/test-css", HTTP_GET, handleTestCSS);
    _server->on("/test-js", HTTP_GET, handleTestJS);

    // Rota para arquivos não encontrados
    _server->onNotFound(handleNotFound);

    // Inicia servidor
    _server->begin();
    _running = true;

    LOG_INFO("Servidor OTA Push inicializado na porta: %d", port);

    if (_mdnsHostname != "")
    {
        LOG_INFO("Acesse: http://%s.local/update", _mdnsHostname.c_str());
    }
    else
    {
        LOG_INFO("Acesse: http://%s/update", WiFi.localIP().toString().c_str());
    }
}

// ============ HANDLERS PRINCIPAIS ============

bool OTAPushUpdateManager::checkAuthentication(AsyncWebServerRequest *request)
{
    if (_username == "" || _password == "")
    {
        return true;
    }

    if (!request->authenticate(_username.c_str(), _password.c_str()))
    {
        request->requestAuthentication();
        return false;
    }

    _authenticated = true;
    return true;
}

void OTAPushUpdateManager::handleRoot(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    if (!isLittleFSMounted())
    {
        LOG_ERROR("❌ LittleFS não está montado em handleRoot()");
        request->send(500, "text/plain", "File system error");
        return;
    }

    if (!LittleFS.exists("/web/index.html"))
    {
        LOG_ERROR("❌ index.html não encontrado em /web/index.html");
        request->send(404, "text/plain", "index.html not found");
        return;
    }

    request->send(LittleFS, "/web/index.html", "text/html");
}

void OTAPushUpdateManager::handleUpdate(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    String html = processTemplate(htmlUpdate, "Firmware Upload", "", request);
    request->send(200, "text/html", html);
}

void OTAPushUpdateManager::handleSystemInfo(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    String content = getFullSystemInfoContent();
    String html = processTemplate(htmlSystem, "System Information", content, request);
    request->send(200, "text/html", html);
}

void OTAPushUpdateManager::handleToggleTheme(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    String currentTheme = request->hasArg("theme") ? request->arg("theme") : "light";
    String newTheme = (currentTheme == "light") ? "dark" : "light";
    String currentPath = request->hasArg("path") ? request->arg("path") : "/";

    String redirectUrl = "/filesystem?path=" + currentPath + "&theme=" + newTheme;
    request->redirect(redirectUrl);
}

// ============ HANDLERS DE UPLOAD ============

void OTAPushUpdateManager::handleDoUpload(AsyncWebServerRequest *request, const String &filename,
                                          size_t index, uint8_t *data, size_t len, bool final)
{
    if (!checkAuthentication(request))
        return;

    static File uploadFile;
    static String detectedVersion = "";
    static String currentVersion = OTAPullUpdateManager::getCurrentVersion();

    if (index == 0)
    {
        LOG_INFO("📤 Iniciando upload OTA: %s", filename.c_str());

        if (!filename.endsWith(".bin"))
        {
            request->send(400, "text/plain", "Error: Only .bin files are allowed");
            return;
        }

        _updating = true;
        detectedVersion = "";

        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            LOG_ERROR("❌ Falha ao iniciar update: %s", Update.errorString());
            request->send(500, "text/plain", "Update begin failed: " + String(Update.errorString()));
            _updating = false;
            return;
        }
    }

    // Processa dados do upload
    if (Update.write(data, len) != len)
    {
        LOG_ERROR("❌ Erro na escrita: %s", Update.errorString());
    }

    if (final)
    {
        LOG_INFO("📋 Upload finalizado, total: %u bytes", index + len);

        if (Update.end(true))
        {
            LOG_INFO("🎉 Update aplicado com sucesso!");
            request->send(200, "text/plain", "Update successful! Restarting...");
            delay(2000);
            ESP.restart();
        }
        else
        {
            LOG_ERROR("💥 Falha ao finalizar update: %s", Update.errorString());
            request->send(500, "text/plain", "Update failed: " + String(Update.errorString()));
        }
        _updating = false;
    }
}

// ============ HANDLERS DE API ============

void OTAPushUpdateManager::handleCheckUpdates(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    OTAManager::checkForUpdates();
    String status = "";

    if (OTAManager::isUpdateAvailable())
    {
        String latestVersion = OTAManager::getLatestVersion();
        String currentVersion = OTAPullUpdateManager::getCurrentVersion();
        status = "🎯 Nova versão disponível: v" + latestVersion + " (atual: v" + currentVersion + ")";
    }
    else
    {
        status = "✅ Firmware está atualizado (v" + OTAPullUpdateManager::getCurrentVersion() + ")";
    }

    String jsonResponse = "{\"status\":\"success\", \"message\":\"" + status + "\"}";
    request->send(200, "application/json", jsonResponse);
}

void OTAPushUpdateManager::handlePerformUpdate(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    if (OTAManager::isUpdateAvailable())
    {
        request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Iniciando atualização...\"}");
        delay(1000);
        OTAManager::performUpdate();
    }
    else
    {
        request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Nenhuma atualização disponível\"}");
    }
}

// ============ HANDLERS DE TESTE ============

void OTAPushUpdateManager::handleTestCSS(AsyncWebServerRequest *request)
{
    LOG_INFO("🧪 Testando rota de CSS...");
    serveStaticFile(request, "/web/css/styles.css", "text/css");
}

void OTAPushUpdateManager::handleTestJS(AsyncWebServerRequest *request)
{
    LOG_INFO("🧪 Testando rota de JS...");
    serveStaticFile(request, "/web/js/scripts.js", "application/javascript");
}

void OTAPushUpdateManager::handleNotFound(AsyncWebServerRequest *request)
{
    LOG_WARN("⚠️ Rota não encontrada: %s", request->url().c_str());
    LOG_WARN("🔍 Método: %s", request->methodToString());

    request->send(404, "text/plain", "Route not found: " + request->url());
}

// ============ SERVIÇO DE ARQUIVOS ESTÁTICOS ============

void OTAPushUpdateManager::serveStaticFile(AsyncWebServerRequest *request, const String &path, const String &defaultContentType)
{
    if (!isLittleFSMounted())
    {
        LOG_ERROR("❌ LittleFS não está montado ao tentar servir: %s", path.c_str());
        request->send(500, "text/plain", "File system error");
        return;
    }

    if (!LittleFS.exists(path))
    {
        LOG_WARN("⚠️ Arquivo não encontrado: %s", path.c_str());
        request->send(404, "text/plain", "File not found: " + path);
        return;
    }

    String contentType = getContentType(path, defaultContentType);
    request->send(LittleFS, path, contentType);
}

String OTAPushUpdateManager::getContentType(const String &filename, const String &defaultType)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    if (filename.endsWith(".png"))
        return "image/png";
    if (filename.endsWith(".jpg") || filename.endsWith(".jpeg"))
        return "image/jpeg";
    if (filename.endsWith(".gif"))
        return "image/gif";
    if (filename.endsWith(".ico"))
        return "image/x-icon";
    if (filename.endsWith(".json"))
        return "application/json";
    if (filename.endsWith(".txt"))
        return "text/plain";
    return defaultType;
}

String OTAPushUpdateManager::getCurrentDateTime()
{
    // Usa o tempo interno do sistema (que deve estar sincronizado com NTP)
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    // Mapeamento de meses em português
    const char *months_Pt_BR[] = {"Jan", "Fev", "Mar", "Abr", "Mai", "Jun",
                                  "Jul", "Ago", "Set", "Out", "Nov", "Dez"};

    // Extrair componentes da data
    int day = timeinfo.tm_mday;
    int month = timeinfo.tm_mon; // 0-11
    int year = timeinfo.tm_year + 1900;
    int hour = timeinfo.tm_hour;
    int min = timeinfo.tm_min;
    int sec = timeinfo.tm_sec;

    // Formatar no estilo DD/MMM/AAAA HH:MM:SS
    char timeStr[64];
    snprintf(timeStr, sizeof(timeStr), "%02d/%03s/%04d %02d:%02d:%02d",
             day, months_Pt_BR[month], year, hour, min, sec);

    return String(timeStr);
}

String OTAPushUpdateManager::formatUptime(unsigned long milliseconds)
{
    unsigned long seconds = milliseconds / 1000;
    unsigned long days = seconds / 86400;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;

    char uptimeStr[32];
    if (days > 0)
    {
        snprintf(uptimeStr, sizeof(uptimeStr), "%lud %02luh %02lum %02lus", days, hours, minutes, secs);
    }
    else if (hours > 0)
    {
        snprintf(uptimeStr, sizeof(uptimeStr), "%02luh %02lum %02lus", hours, minutes, secs);
    }
    else
    {
        snprintf(uptimeStr, sizeof(uptimeStr), "%02lum %02lus", minutes, secs);
    }

    return String(uptimeStr);
}

// ============ CONTEÚDO DAS PÁGINAS ============

String OTAPushUpdateManager::getSystemInfoContent()
{
    String content = "";
    content += "<div class=\"info-grid\">";

    // Card WiFi
    content += "<div class=\"info-card\">";
    content += "<h3>WiFi</h3>";
    content += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
    if (_mdnsHostname != "")
    {
        content += "<p><strong>mDNS:</strong> " + _mdnsHostname + ".local</p>";
    }
    content += "<p><strong>SSID:</strong> " + WiFi.SSID() + "</p>";
    content += "</div>";

    // Card System
    content += "<div class=\"info-card\">";
    content += "<h3>System</h3>";
    content += "<p><strong>Version:</strong> " + String(OTAManager::getFirmwareVersion().c_str()) + "</p>";
    content += "<p><strong>Heap Free:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    content += "<p><strong>CPU:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    content += "</div>";

    // ============ SEÇÃO DE ATUALIZAÇÃO PULL ============
    String pullStatus = getPullUpdateStatus();
    if (pullStatus != "disabled")
    {
        content += "<div class=\"info-card\" style=\"display: flex; flex-direction: column; justify-content: center;\">";
        content += "<h3>🔄 OTA Remoto</h3>";

        if (pullStatus == "available")
        {
            String currentVersion = OTAPullUpdateManager::getCurrentVersion();
            String latestVersion = OTAManager::getLatestVersion();

            content += "<div style=\"background: var(--success); color: white; padding: 0.5rem; border-radius: 0.375rem; margin: 0.25rem 0; text-align: center;\">";
            content += "<p style=\"margin: 0; font-weight: bold; font-size: 0.85rem;\">📦 Atualização Disponível</p>";
            content += "</div>";

            content += "<div style=\"display: flex; gap: 0.5rem; margin: 0.75rem 0;\">";
            content += "<button class=\"btn\" style=\"background: var(--success); padding: 0.4rem 0.8rem; font-size: 0.8rem; flex: 1;\" onclick=\"performPullUpdate()\">";
            content += "🔄 Instalar v" + latestVersion;
            content += "</button>";
            content += "<button class=\"btn\" style=\"background: var(--accent-primary); padding: 0.4rem 0.8rem; font-size: 0.8rem; flex: 1;\" onclick=\"checkForUpdates()\">";
            content += "Verificar";
            content += "</button>";
            content += "</div>";
        }
        else
        {
            content += "<p style=\"text-align: center; color: var(--success); font-size: 0.9rem; margin: 0.5rem 0;\">✅ Atualizado</p>";
            content += "<div style=\"text-align: center; margin: 0.5rem 0;\">";
            content += "<button class=\"btn\" style=\"background: var(--accent-primary); padding: 0.4rem 0.8rem; font-size: 0.8rem;\" onclick=\"checkForUpdates()\">";
            content += "🔍 Verificar";
            content += "</button>";
            content += "</div>";
        }

        content += "</div>";
    }

    content += "</div>"; // ✅ FECHAR O info-grid

    content += "<div class=\"action-grid\">";
    content += "<a href=\"/update\" class=\"action-card\">";
    content += "<div class=\"action-icon\">📤</div>";
    content += "<h3>Firmware Upload</h3>";
    content += "<p>Send new .bin firmware</p>";
    content += "</a>";
    content += "<a href=\"/system\" class=\"action-card\">";
    content += "<div class=\"action-icon\">ℹ️</div>";
    content += "<h3>System Info</h3>";
    content += "<p>Complete ESP32 details</p>";
    content += "</a>";
    content += "<a href=\"/filesystem\" class=\"action-card\">";
    content += "<div class=\"action-icon\">📁</div>";
    content += "<h3>File Manager</h3>";
    content += "<p>Gerenciar arquivos LittleFS</p>";
    content += "</a>";
    content += "</div>";

    return content;
}

// ============ PROCESSAMENTO DE TEMPLATES ============
String OTAPushUpdateManager::processTemplate(const String &templateHTML, const String &title, const String &content, AsyncWebServerRequest *request)
{
    String html = templateHTML;

    // ✅ CORREÇÃO: Verifica o tema no request
    bool isDarkMode = true; // padrão dark
    if (request && request->hasArg("theme"))
    {
        String theme = request->arg("theme");
        isDarkMode = (theme == "dark");
    }

    // Substitui placeholders PRINCIPAIS
    html.replace("{{TITLE}}", title);
    html.replace("{{CONTENT}}", content);
    html.replace("{{THEME}}", isDarkMode ? "dark" : "light");
    html.replace("{{THEME_BUTTON}}", isDarkMode ? "☀️" : "🌙");
    html.replace("{{HOST_INFO}}", _mdnsHostname != "" ? _mdnsHostname + ".local" : WiFi.localIP().toString());

    // CORREÇÃO CRÍTICA: Substituir {{CSS}} e {{JS}} pelos conteúdos reais
    html.replace("{{CSS}}", cssStyles);
    html.replace("{{JS}}", jsScript);

    // Placeholders adicionais
    html.replace("{{MDNS_HOSTNAME}}", _mdnsHostname);
    html.replace("{{IP_ADDRESS}}", WiFi.localIP().toString());
    html.replace("{{SSID}}", WiFi.SSID());
    html.replace("{{HEAP_FREE}}", String(ESP.getFreeHeap()));
    html.replace("{{CPU_FREQ}}", String(ESP.getCpuFreqMHz()));

    html.replace("{{ESP32_TIME}}", getCurrentDateTime());
    html.replace("{{UPTIME}}", formatUptime(millis()));

    // DEBUG: Verificar se substituição funcionou
    if (html.indexOf("{{CSS}}") != -1)
    {
        LOG_ERROR("❌ Placeholder {{CSS}} NÃO foi substituído!");
    }
    else
    {
        LOG_DEBUG("✅ Placeholder {{CSS}} substituído com sucesso");
    }

    return html;
}

// ============ IMPLEMENTAÇÕES FALTANTES ============

// Gerenciamento de arquivos
void OTAPushUpdateManager::handleFilesystem(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    // String currentPath = request->hasArg("path") ? request->arg("path") : "/";
    // String content = getFilesystemContent(currentPath);
    // String html = processTemplate(htmlFilesystem, "File Manager", content, request);

    // // Adiciona substituições específicas do filesystem
    // html.replace("{{CURRENT_PATH}}", currentPath);
    // html.replace("{{BREADCRUMB}}", generateBreadcrumb(currentPath));

    // request->send(200, "text/html", html);
}

void OTAPushUpdateManager::handleFilesystemUpload(AsyncWebServerRequest *request, const String &filename,
                                                  size_t index, uint8_t *data, size_t len, bool final)
{
    if (!checkAuthentication(request))
        return;

    static File uploadFile;
    static String uploadPath;

    if (index == 0)
    {
        LOG_INFO("📤 Iniciando upload de arquivo: %s", filename.c_str());
        uploadPath = getUploadPathFromRequest(request);

        // Cria diretórios se necessário
        createDirectories(getDirectoryPath(uploadPath + "/" + filename));

        String fullPath = uploadPath + "/" + filename;
        uploadFile = LittleFS.open(fullPath, "w");
        if (!uploadFile)
        {
            LOG_ERROR("❌ Falha ao criar arquivo: %s", fullPath.c_str());
            request->send(500, "text/plain", "Failed to create file");
            return;
        }
    }

    if (uploadFile)
    {
        if (uploadFile.write(data, len) != len)
        {
            LOG_ERROR("❌ Erro na escrita do arquivo");
        }
    }

    if (final)
    {
        if (uploadFile)
        {
            uploadFile.close();
            LOG_INFO("✅ Upload finalizado: %s (%u bytes)", filename.c_str(), index + len);
            request->send(200, "text/plain", "Upload successful");
        }
        else
        {
            request->send(500, "text/plain", "Upload failed");
        }
    }
}

void OTAPushUpdateManager::handleFilesystemDelete(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    if (!request->hasArg("path"))
    {
        request->send(400, "text/plain", "Missing path parameter");
        return;
    }

    String path = request->arg("path");

    if (LittleFS.exists(path))
    {
        if (LittleFS.remove(path))
        {
            LOG_INFO("✅ Arquivo deletado: %s", path.c_str());
            request->send(200, "text/plain", "File deleted");
        }
        else
        {
            // Tenta deletar como diretório
            if (deleteDirectoryRecursive(path))
            {
                LOG_INFO("✅ Diretório deletado: %s", path.c_str());
                request->send(200, "text/plain", "Directory deleted");
            }
            else
            {
                LOG_ERROR("❌ Falha ao deletar: %s", path.c_str());
                request->send(500, "text/plain", "Delete failed");
            }
        }
    }
    else
    {
        request->send(404, "text/plain", "File not found");
    }
}

void OTAPushUpdateManager::handleFilesystemMkdir(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    if (!request->hasArg("path"))
    {
        request->send(400, "text/plain", "Missing path parameter");
        return;
    }

    String path = request->arg("path");

    if (createDirectories(path))
    {
        LOG_INFO("✅ Diretório criado: %s", path.c_str());
        request->send(200, "text/plain", "Directory created");
    }
    else
    {
        LOG_ERROR("❌ Falha ao criar diretório: %s", path.c_str());
        request->send(500, "text/plain", "Failed to create directory");
    }
}

void OTAPushUpdateManager::handleFilesystemDownload(AsyncWebServerRequest *request)
{
    if (!checkAuthentication(request))
        return;

    if (!request->hasArg("path"))
    {
        request->send(400, "text/plain", "Missing path parameter");
        return;
    }

    String path = request->arg("path");
    serveStaticFile(request, path);
}

// FreeRTOS
void OTAPushUpdateManager::run(uint32_t stackSize, UBaseType_t priority)
{
    if (_taskRunning)
        return;

    _taskStackSize = stackSize;
    _taskPriority = priority;

    xTaskCreate(
        taskFunction,
        "OTAWebServer",
        stackSize,
        nullptr,
        priority,
        &_webPageTaskHandle);

    _taskRunning = true;
    LOG_INFO("✅ Task FreeRTOS iniciada");
}

void OTAPushUpdateManager::stop()
{
    stopTask();
}

void OTAPushUpdateManager::taskFunction(void *parameter)
{
    while (_taskRunning)
    {
        updateTime();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(nullptr);
}

void OTAPushUpdateManager::stopTask()
{
    if (_taskRunning && _webPageTaskHandle)
    {
        _taskRunning = false;
        vTaskDelete(_webPageTaskHandle);
        _webPageTaskHandle = nullptr;
        LOG_INFO("✅ Task FreeRTOS parada");
    }
}

// Configurações
void OTAPushUpdateManager::setMDNS(const String &hostname)
{
    _mdnsHostname = hostname;
    if (MDNS.begin(hostname.c_str()))
    {
        LOG_INFO("✅ mDNS iniciado: %s.local", hostname.c_str());
    }
    else
    {
        LOG_ERROR("❌ Falha ao iniciar mDNS");
    }
}

void OTAPushUpdateManager::setCredentials(const String &username, const String &password)
{
    _username = username;
    _password = password;
    LOG_INFO("✅ Credenciais OTA definidas");
}

void OTAPushUpdateManager::setPullUpdateCallback(bool (*callback)())
{
    _pullUpdateAvailableCallback = callback;
}

void OTAPushUpdateManager::setPerformUpdateCallback(void (*callback)())
{
    _performUpdateCallback = callback;
}

// Status e utilitários
bool OTAPushUpdateManager::isUpdating()
{
    return _updating;
}

bool OTAPushUpdateManager::isRunning()
{
    return _running;
}

String OTAPushUpdateManager::getAccessURL()
{
    if (_mdnsHostname != "")
    {
        return "http://" + _mdnsHostname + ".local";
    }
    return "http://" + WiFi.localIP().toString();
}

String OTAPushUpdateManager::getPullUpdateStatus()
{
    if (_pullUpdateAvailableCallback)
    {
        return _pullUpdateAvailableCallback() ? "available" : "updated";
    }
    return "disabled";
}

// NTP Client
void OTAPushUpdateManager::updateTime()
{
    if (_timeClient)
    {
        _timeClient->update();
    }
}

unsigned long OTAPushUpdateManager::getCurrentTimestamp()
{
    if (_timeClient)
    {
        return _timeClient->getEpochTime();
    }
    return 0;
}

String OTAPushUpdateManager::formatTime(unsigned long rawTime)
{
    time_t time = rawTime;
    struct tm *timeinfo = localtime(&time);

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return String(buffer);
}

// Filesystem
bool OTAPushUpdateManager::initLittleFS()
{
    if (!LittleFS.begin(true))
    {
        LOG_ERROR("❌ Falha ao montar LittleFS");
        return false;
    }
    LOG_INFO("✅ LittleFS montado com sucesso");
    return true;
}

bool OTAPushUpdateManager::isLittleFSMounted()
{
    return LittleFS.begin(true); // Tenta montar se não estiver
}

String OTAPushUpdateManager::getFullSystemInfoContent()
{
    // Implementação básica - expanda conforme necessário
    String content = "<div class='system-info'>";
    content += "<h3>System Information</h3>";
    content += "<p><strong>Uptime:</strong> " + formatUptime(millis()) + "</p>";
    content += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    content += "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    content += "</div>";
    return content;
}

// Adicione estas implementações stub para as funções restantes:

String OTAPushUpdateManager::getFilesystemContent(const String &currentPath) { return ""; }
String OTAPushUpdateManager::generateBreadcrumb(const String &currentPath) { return ""; }
String OTAPushUpdateManager::generateFileList(const String &currentPath) { return ""; }
String OTAPushUpdateManager::formatFileSize(size_t bytes) { return ""; }
String OTAPushUpdateManager::getFileExtension(const String &filename) { return ""; }
String OTAPushUpdateManager::getDirectoryPath(const String &fullPath) { return ""; }
bool OTAPushUpdateManager::createDirectories(const String &path) { return false; }
String OTAPushUpdateManager::getUploadPathFromRequest(AsyncWebServerRequest *request) { return ""; }
bool OTAPushUpdateManager::deleteDirectoryRecursive(const String &path) { return false; }
String OTAPushUpdateManager::resetReason(esp_reset_reason_t reset) { return ""; }
String OTAPushUpdateManager::formatBuildDate() { return ""; }
String OTAPushUpdateManager::extractVersionFromBinary(const uint8_t *data, size_t length) { return ""; }
bool OTAPushUpdateManager::isValidVersion(const String &version) { return false; }
