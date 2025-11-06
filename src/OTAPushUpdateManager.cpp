#include "OTAPushUpdateManager.h"
#include "OTAManager.h"
#include "webPage/css.h"
#include "webPage/favicon.h"
#include "webPage/filesystem.h"
#include "webPage/index.h"
#include "webPage/scripts.h"
#include "webPage/system.h"
#include "webPage/update.h"
#include "webPage/updateSection.h"

// ============ INICIALIZAÇÃO DE VARIÁVEIS ESTÁTICAS ============

WebServer *OTAPushUpdateManager::_server = nullptr;
bool OTAPushUpdateManager::_updating = false;
String OTAPushUpdateManager::_username = "";
String OTAPushUpdateManager::_password = "";
bool OTAPushUpdateManager::_authenticated = false;
bool OTAPushUpdateManager::_running = false;
String OTAPushUpdateManager::_mdnsHostname = "";

// ✅ ADICIONAR ESTAS LINhas - DEFINIÇÃO DAS VARIÁVEIS DE CALLBACK
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

// static bool (*_pullUpdateAvailableCallback)() = nullptr;
// static void (*_performUpdateCallback)() = nullptr;

void OTAPushUpdateManager::setPullUpdateCallback(bool (*callback)())
{
    _pullUpdateAvailableCallback = callback;
}

void OTAPushUpdateManager::setPerformUpdateCallback(void (*callback)())
{
    _performUpdateCallback = callback;
}

String OTAPushUpdateManager::getPullUpdateStatus()
{
    if (_pullUpdateAvailableCallback)
    {
        return _pullUpdateAvailableCallback() ? "available" : "none";
    }
    return "disabled";
}

// ============ IMPLEMENTAÇÃO DOS MÉTODOS FREERTOS ============

void OTAPushUpdateManager::run(uint32_t stackSize, UBaseType_t priority)
{
    if (_taskRunning)
    {
        LOG_WARN("Thread FreeRTOS já está em execução");
        return;
    }

    _taskStackSize = stackSize;
    _taskPriority = priority;

    // Cria a task FreeRTOS
    BaseType_t result = xTaskCreate(
        taskFunction,       // Função da task
        "WebPageTask",      // Nome da task
        _taskStackSize,     // Stack size
        nullptr,            // Parâmetros
        _taskPriority,      // Prioridade
        &_webPageTaskHandle // Handle da task
    );

    if (result == pdPASS)
    {
        _taskRunning = true;
        LOG_INFO("✅ Thread FreeRTOS iniciada (Stack: %u, Priority: %u, Core: %d)",
                 _taskStackSize, _taskPriority);
    }
    else
    {
        LOG_ERROR("❌ Falha ao criar thread FreeRTOS");
        _taskRunning = false;
    }
}

void OTAPushUpdateManager::stop()
{
    stopTask();
}

void OTAPushUpdateManager::stopTask()
{
    if (!_taskRunning)
    {
        return;
    }

    if (_webPageTaskHandle != nullptr)
    {
        _taskRunning = false;

        // Aguarda a task terminar (timeout de 2 segundos)
        if (xTaskGetCurrentTaskHandle() != _webPageTaskHandle)
        {
            // Só deleta se não for a própria task chamando
            vTaskDelete(_webPageTaskHandle);
        }

        _webPageTaskHandle = nullptr;
        LOG_INFO("🛑 Thread FreeRTOS parada");
    }
}

void OTAPushUpdateManager::taskFunction(void *parameter)
{
    LOG_INFO("🔄 Thread FreeRTOS iniciada");

    while (_taskRunning)
    {
        // Processa requisições web
        if (_server && _running)
        {
            _server->handleClient();
        }

        // Atualiza tempo NTP periodicamente
        static unsigned long lastTimeUpdate = 0;
        if (millis() - lastTimeUpdate >= 60000)
        { // A cada 60 segundos
            updateTime();
            lastTimeUpdate = millis();
        }

        // Pequena pausa para não sobrecarregar a CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    LOG_INFO("🔄 Thread FreeRTOS finalizada");
    vTaskDelete(nullptr);
}

// ============ IMPLEMENTAÇÃO DOS MÉTODOS ============

void OTAPushUpdateManager::begin(uint16_t port)
{
    if (_server == nullptr)
    {
        _server = new WebServer(port);
    }

    // ✅ CORREÇÃO: Inicialização robusta do LittleFS
    if (!LittleFS.begin(true))
    {
        LOG_ERROR("❌ Falha ao inicializar LittleFS - Tentando formatar...");

        // Tenta formatar se a inicialização falhar
        if (LittleFS.format())
        {
            LOG_INFO("✅ LittleFS formatado com sucesso");
            if (LittleFS.begin(true))
            {
                LOG_INFO("✅ LittleFS inicializado após formatação");
            }
            else
            {
                LOG_ERROR("❌ Falha crítica: LittleFS não pode ser inicializado");
                return; // Não continua se o filesystem não funcionar
            }
        }
        else
        {
            LOG_ERROR("❌ Falha crítica: Não foi possível formatar LittleFS");
            return;
        }
    }
    else
    {
        LOG_INFO("✅ LittleFS inicializado com sucesso");

        // Verifica se pode escrever no filesystem
        File testFile = LittleFS.open("/test_write.txt", "w");
        if (!testFile)
        {
            LOG_ERROR("❌ LittleFS montado mas sem permissão de escrita");
        }
        else
        {
            testFile.println("Teste de escrita");
            testFile.close();
            LittleFS.remove("/test_write.txt");
            LOG_INFO("✅ LittleFS com permissão de escrita verificada");
        }
    }

    // Resto do código permanece igual...
    // Configura o timezone e servidor NTP do sistema
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // UTC-3 (Brasília)

    // Inicializa NTP Client para uso interno
    if (_ntpUDP == nullptr)
    {
        _ntpUDP = new WiFiUDP();
        _timeClient = new NTPClient(*_ntpUDP, "pool.ntp.org", -3 * 3600, 60000);
        _timeClient->begin();

        // Força uma atualização inicial
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

    // Configura endpoints (o resto do código permanece igual)...
    _server->on("/favicon.ico", HTTP_GET, []()
                {
        LOG_INFO("✅ Favicon solicitado - Enviando imagem PNG (%d bytes)", favicon_ico_size);
        _server->setContentLength(favicon_ico_size);
        _server->send(200, "image/png");
        _server->sendContent_P((const char*)favicon_ico, favicon_ico_size); });

    _server->on("/", HTTP_GET, handleRoot);
    _server->on("/update", HTTP_GET, handleUpdate);
    _server->on("/doUpdate", HTTP_POST, []()
                { OTAPushUpdateManager::_server->send(200, "text/plain", "Upload processed"); }, handleDoUpload);
    _server->on("/system", HTTP_GET, handleSystemInfo);
    _server->on("/toggle-theme", HTTP_GET, handleToggleTheme);

    // File System Routes
    _server->on("/filesystem", HTTP_GET, handleFilesystem);
    _server->on("/filesystem-upload", HTTP_POST, []()
                { _server->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Upload completed\"}"); }, handleFilesystemUpload);
    _server->on("/filesystem-delete", HTTP_POST, handleFilesystemDelete);
    _server->on("/filesystem-mkdir", HTTP_POST, handleFilesystemMkdir);
    _server->on("/filesystem-download", HTTP_GET, handleFilesystemDownload);

    _server->on("/check-updates", HTTP_GET, []()
                {
        if (!checkAuthentication()) {
            return;
        }
        
        OTAManager::checkForUpdates();
        String status = "";
        
        if (OTAManager::isUpdateAvailable()) {
            String latestVersion = OTAManager::getLatestVersion();
            String currentVersion = OTAPullUpdateManager::getCurrentVersion();
            status = "🎯 Nova versão disponível: v" + latestVersion + " (atual: v" + currentVersion + ")";
        } else {
            status = "✅ Firmware está atualizado (v" + OTAPullUpdateManager::getCurrentVersion() + ")";
        }
        
        _server->send(200, "application/json", 
            "{\"status\":\"success\", \"message\":\"" + status + "\"}"); });

    _server->on("/perform-update", HTTP_GET, []()
                {
        if (!checkAuthentication()) {
            return;
        }
        
        if (OTAManager::isUpdateAvailable()) {
            _server->send(200, "application/json", 
                "{\"status\":\"success\", \"message\":\"Iniciando atualização...\"}");
            
            // Pequeno delay para enviar a resposta antes de reiniciar
            delay(1000);
            OTAManager::performUpdate();
        } else {
            _server->send(400, "application/json", 
                "{\"status\":\"error\", \"message\":\"Nenhuma atualização disponível\"}");
        } });

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

void OTAPushUpdateManager::updateTime()
{
    if (_timeClient)
    {
        _timeClient->update();

        // Sincroniza o tempo interno do sistema quando o NTP estiver atualizado
        if (!_timeSynced && _timeClient->getEpochTime() > 1600000000)
        { // Tempo válido (após 2020)
            timeval tv;
            tv.tv_sec = _timeClient->getEpochTime();
            tv.tv_usec = 0;
            settimeofday(&tv, nullptr);
            _timeSynced = true;
            LOG_INFO("Tempo interno do sistema sincronizado com NTP");
        }
    }
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

unsigned long OTAPushUpdateManager::getCurrentTimestamp()
{
    time_t now;
    time(&now);
    return now;
}

String OTAPushUpdateManager::formatTime(unsigned long rawTime)
{
    if (rawTime == 0)
    {
        return "Sincronizando...";
    }

    // Calcula data/hora
    unsigned long hours = (rawTime % 86400L) / 3600;
    unsigned long minutes = (rawTime % 3600) / 60;
    unsigned long seconds = rawTime % 60;

    // Calcula data (simplificado)
    unsigned long days = rawTime / 86400L;

    // Ano base (1970) + dias convertidos para anos aproximados
    int year = 1970 + (days / 365);
    int month = ((days % 365) / 30) + 1;
    int day = (days % 30) + 1;

    // Formata para string
    char timeStr[64];
    snprintf(timeStr, sizeof(timeStr), "%04d-%02d-%02d %02lu:%02lu:%02lu",
             year, month, day, hours, minutes, seconds);

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

String OTAPushUpdateManager::formatBuildDate()
{
    String buildDate = __DATE__; // Formato: "MMM DD YYYY" (ex: "Dec 15 2023")

    // Mapeamento de meses
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const char *months_Pt_BR[] = {"Jan", "Fev", "Mar", "Abr", "Mai", "Jun",
                                  "Jul", "Ago", "Set", "Out", "Nov", "Dez"}; // Português BR

    // Extrair partes da data
    char monthStr[4];
    char dayStr[3];
    char yearStr[5];

    // Parse da string __DATE__ (formato: "MMM DD YYYY")
    sscanf(buildDate.c_str(), "%3s %2s %4s", monthStr, dayStr, yearStr);

    // Encontrar número do mês
    int monthNum = 1;
    for (int i = 0; i < 12; i++)
    {
        if (strcmp(monthStr, months[i]) == 0)
        {
            monthNum = i;
            break;
        }
    }

    char formattedDate[15];
    snprintf(formattedDate, sizeof(formattedDate), "%02s/%03s/%04s",
             dayStr, months_Pt_BR[monthNum], yearStr);

    return String(formattedDate);
}

// REMOVIDO COMPLETAMENTE: loadHTMLTemplates() e loadFile()

void OTAPushUpdateManager::setMDNS(const String &hostname)
{
    _mdnsHostname = hostname;

    if (MDNS.begin(hostname.c_str()))
    {
        MDNS.addService("http", "tcp", 80);
        LOG_INFO("mDNS configurado: %s.local", hostname.c_str());
    }
    else
    {
        LOG_ERROR("Falha ao configurar mDNS");
    }
}

void OTAPushUpdateManager::setCredentials(const String &username, const String &password)
{
    _username = username;
    _password = password;
    LOG_DEBUG("Credenciais OTA definidas");
}

void OTAPushUpdateManager::handleClient()
{
    if (_taskRunning)
    {
        return;
    }
    if (_server && _running)
    {
        _server->handleClient();
    }
}

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
    else
    {
        return "http://" + WiFi.localIP().toString();
    }
}

// ============ HANDLERS DO SERVIDOR WEB ============

bool OTAPushUpdateManager::checkAuthentication()
{
    if (_username == "" || _password == "")
    {
        return true;
    }

    if (_server->authenticate(_username.c_str(), _password.c_str()))
    {
        _authenticated = true;
        return true;
    }

    _server->requestAuthentication();
    return false;
}

void OTAPushUpdateManager::handleRoot()
{
    if (!checkAuthentication())
        return;

    // ✅ ALTERADO: Padrão dark quando não há parâmetro
    String theme = _server->hasArg("theme") ? _server->arg("theme") : "dark";
    bool darkMode = (theme == "dark");

    String content = getSystemInfoContent();
    String html = processTemplate(htmlIndex, "ESP32 OTA Update", content, darkMode);

    _server->send(200, "text/html", html);
}

void OTAPushUpdateManager::handleUpdate()
{
    if (!checkAuthentication())
        return;

    // ✅ ALTERADO: Padrão dark quando não há parâmetro
    String theme = _server->hasArg("theme") ? _server->arg("theme") : "dark";
    bool darkMode = (theme == "dark");

    String html = processTemplate(htmlUpdate, "Firmware Upload", "", darkMode);
    _server->send(200, "text/html", html);
}

void OTAPushUpdateManager::handleSystemInfo()
{
    if (!checkAuthentication())
        return;

    // ✅ ALTERADO: Padrão dark quando não há parâmetro
    String theme = _server->hasArg("theme") ? _server->arg("theme") : "dark";
    bool darkMode = (theme == "dark");

    String content = getFullSystemInfoContent();
    String html = processTemplate(htmlSystem, "System Information", content, darkMode);

    _server->send(200, "text/html", html);
}

void OTAPushUpdateManager::handleToggleTheme()
{
    if (!checkAuthentication())
        return;

    String currentTheme = _server->hasArg("theme") ? _server->arg("theme") : "light";
    String newTheme = (currentTheme == "light") ? "dark" : "light";

    // ✅ CORREÇÃO: Manter o path quando disponível
    String currentPath = _server->hasArg("path") ? _server->arg("path") : "/";

    _server->sendHeader("Location", "/filesystem?path=" + currentPath + "&theme=" + newTheme, true);
    _server->send(302, "text/plain", "");
}

void OTAPushUpdateManager::handleDoUpload()
{
    if (!checkAuthentication())
        return;

    HTTPUpload &upload = _server->upload();
    static String detectedVersion = "";
    static String currentVersion = OTAPullUpdateManager::getCurrentVersion();

    if (upload.status == UPLOAD_FILE_START)
    {
        _updating = true;
        detectedVersion = ""; // Reseta para novo upload
        LOG_INFO("📤 Iniciando upload OTA: %s", upload.filename.c_str());
        LOG_INFO("📦 Tamanho do arquivo: %u bytes", upload.totalSize);

        // ✅ NOVO: Verifica se é um arquivo .bin
        if (!upload.filename.endsWith(".bin"))
        {
            LOG_ERROR("❌ Arquivo não é .bin: %s", upload.filename.c_str());
            _server->send(400, "text/plain", "Error: Only .bin files are allowed");
            _updating = false;
            return;
        }

        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            LOG_ERROR("❌ Falha ao iniciar update: %s", Update.errorString());
            _server->send(500, "text/plain", "Update begin failed: " + String(Update.errorString()));
            _updating = false;
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        // ✅ NOVO: Tenta detectar versão durante o upload (apenas uma vez)
        if (detectedVersion == "" && upload.totalSize > 1024)
        { // Aguarda pelo menos 1KB
            detectedVersion = extractVersionFromBinary(upload.buf, upload.currentSize);

            if (detectedVersion != "unknown")
            {
                LOG_INFO("🔍 Versão detectada no upload: %s", detectedVersion.c_str());

                // Compara com versão atual
                OTAManager::VersionComparison comp = OTAManager::compareVersions(detectedVersion, currentVersion);

                switch (comp)
                {
                case OTAManager::VERSION_NEWER:
                    LOG_INFO("✅ Nova versão mais recente: %s → %s",
                             currentVersion.c_str(), detectedVersion.c_str());
                    break;
                case OTAManager::VERSION_EQUAL:
                    LOG_WARN("⚠️  Mesma versão: %s", currentVersion.c_str());
                    break;
                case OTAManager::VERSION_OLDER:
                    LOG_WARN("⚠️  Versão mais antiga: %s ← %s",
                             currentVersion.c_str(), detectedVersion.c_str());
                    break;
                }
            }
        }

        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            LOG_ERROR("❌ Erro na escrita: %s", Update.errorString());
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        LOG_INFO("📋 Upload finalizado, total: %u bytes", upload.totalSize);

        // ✅ NOVO: Verificação final da versão antes de instalar
        bool shouldProceed = true;
        String versionMessage = "";

        if (detectedVersion != "unknown")
        {
            OTAManager::VersionComparison comp = OTAManager::compareVersions(detectedVersion, currentVersion);

            switch (comp)
            {
            case OTAManager::VERSION_NEWER:
                versionMessage = "📈 Atualizando para versão MAIS NOVA: " + detectedVersion;
                break;
            case OTAManager::VERSION_EQUAL:
                versionMessage = "🔄 Mesma versão: " + detectedVersion + " - Continuando...";
                break;
            case OTAManager::VERSION_OLDER:
                versionMessage = "⚠️  AVISO: Versão MAIS ANTIGA detectada: " + detectedVersion;
                // Não bloqueia, apenas alerta
                break;
            }

            LOG_INFO("%s", versionMessage.c_str());
        }
        else
        {
            versionMessage = "🔍 Versão não detectada no firmware - Instalando...";
            LOG_WARN("%s", versionMessage.c_str());
        }

        if (Update.end(true))
        {
            LOG_INFO("🎉 Update aplicado com sucesso! %s", versionMessage.c_str());
            _updating = false;

            // ✅ NOVO: Mensagem mais informativa
            String successMessage = "Update successful! ";
            successMessage += versionMessage;
            successMessage += " Restarting...";

            _server->send(200, "text/plain", successMessage);

            delay(2000);
            ESP.restart();
        }
        else
        {
            LOG_ERROR("💥 Falha ao finalizar update: %s", Update.errorString());
            _server->send(500, "text/plain", "Update failed: " + String(Update.errorString()));
            _updating = false;
        }
    }
    if (upload.status == UPLOAD_FILE_END || upload.status == UPLOAD_FILE_ABORTED)
    {
        detectedVersion = ""; // Limpar memória
        currentVersion = "";
    }
}
// ✅ ADICIONAR: Função para validar formato de versão
bool OTAPushUpdateManager::isValidVersion(const String &version)
{
    if (version.length() == 0 || version == "unknown")
    {
        return false;
    }

    // Verifica se segue formato semântico (ex: 1.2.3, v1.0.0, 0.5.1)
    // Pelo menos deve ter um ponto
    if (version.indexOf('.') == -1)
    {
        return false;
    }

    // Remove 'v' prefix se existir
    String cleanVersion = version;
    if (cleanVersion.startsWith("v") || cleanVersion.startsWith("V"))
    {
        cleanVersion = cleanVersion.substring(1);
    }

    // Verifica se todos os caracteres são válidos (dígitos e pontos)
    for (size_t i = 0; i < cleanVersion.length(); i++)
    {
        char c = cleanVersion[i];
        if (!isdigit(c) && c != '.')
        {
            return false;
        }
    }

    return true;
}

// ✅ ADICIONAR: Função para extrair versão do binário (busca por padrões comuns)
String OTAPushUpdateManager::extractVersionFromBinary(const uint8_t *data, size_t length)
{
    // Converte para string para busca
    String binaryString;
    binaryString.reserve(length);
    for (size_t i = 0; i < length; i++)
    {
        if (data[i] >= 32 && data[i] <= 126)
        { // Caracteres ASCII imprimíveis
            binaryString += (char)data[i];
        }
        else
        {
            binaryString += ' '; // Substitui caracteres não imprimíveis
        }
    }

    // Padrões comuns para buscar versão no firmware
    const char *patterns[] = {
        "FIRMWARE_VERSION", "FW_VERSION", "VERSION",
        "firmware_version", "version", "Version"};

    for (const char *pattern : patterns)
    {
        int patternIndex = binaryString.indexOf(pattern);
        if (patternIndex != -1)
        {
            // Encontrou o padrão, agora extrai a versão
            int start = patternIndex + strlen(pattern);

            // Procura pelos próximos caracteres que formam a versão
            int end = start;
            while (end < binaryString.length() &&
                   (isalnum(binaryString[end]) || binaryString[end] == '.' ||
                    binaryString[end] == 'v' || binaryString[end] == 'V' ||
                    binaryString[end] == '"' || binaryString[end] == '\''))
            {
                end++;
            }

            String potentialVersion = binaryString.substring(start, end);
            potentialVersion.trim();

            // Limpa caracteres extras
            potentialVersion.replace("\"", "");
            potentialVersion.replace("'", "");
            potentialVersion.replace("=", "");
            potentialVersion.replace(":", "");
            potentialVersion.trim();

            if (isValidVersion(potentialVersion))
            {
                LOG_INFO("📦 Versão detectada no binário: %s (padrão: %s)",
                         potentialVersion.c_str(), pattern);
                return potentialVersion;
            }
        }
    }

    return "unknown";
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
String OTAPushUpdateManager::processTemplate(const String &templateHTML, const String &title, const String &content, bool darkMode)
{
    String html = templateHTML;

    // DEBUG: Log para verificar substituição
    LOG_DEBUG("Processando template - Título: %s", title.c_str());

    // ✅ ALTERADO: Padrão agora é dark (true) em vez de light (false)
    bool isDarkMode = darkMode;

    // Se não foi especificado um tema, usa dark como padrão
    if (!_server->hasArg("theme"))
    {
        isDarkMode = true; // ✅ PADRÃO DARK
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

String OTAPushUpdateManager::resetReason(esp_reset_reason_t reset)
{
    switch (reset)
    {
    case ESP_RST_UNKNOWN:
        return "Reset reason can not be determined";
    case ESP_RST_POWERON:
        return "Reset due to power-on event";
    case ESP_RST_EXT:
        return "Reset by external pin (not applicable for ESP32)";
    case ESP_RST_SW:
        return "Software reset via esp_restart";
    case ESP_RST_PANIC:
        return "Software reset due to exception/panic";
    case ESP_RST_INT_WDT:
        return "Reset (software or hardware) due to interrupt watchdog";
    case ESP_RST_TASK_WDT:
        return "Reset due to task watchdog";
    case ESP_RST_WDT:
        return "Reset due to other watchdogs";
    case ESP_RST_DEEPSLEEP:
        return "Reset after exiting deep sleep mode";
    case ESP_RST_BROWNOUT:
        return "Brownout reset (software or hardware)";
    case ESP_RST_SDIO:
        return "Reset over SDIO";
    default:
        return "Unknown reset reason";
    };
}

// Adicione este novo handler após os outros handlers do filesystem:

void OTAPushUpdateManager::handleFilesystemDownload()
{
    if (!checkAuthentication())
        return;

    if (!_server->hasArg("path"))
    {
        _server->send(400, "text/plain", "Caminho do arquivo não especificado");
        return;
    }

    String filePath = _server->arg("path");

    if (!isLittleFSMounted())
    {
        _server->send(500, "text/plain", "Sistema de arquivos não disponível");
        return;
    }

    // ✅ VERIFICAÇÃO: Garantir que o path é válido
    if (filePath.length() == 0 || filePath == "/")
    {
        _server->send(400, "text/plain", "Caminho inválido");
        return;
    }

    File file = LittleFS.open(filePath, "r");
    if (!file)
    {
        _server->send(404, "text/plain", "Arquivo não encontrado: " + filePath);
        return;
    }

    if (file.isDirectory())
    {
        file.close();
        _server->send(400, "text/plain", "Não é possível baixar uma pasta");
        return;
    }

    // Extrai apenas o nome do arquivo do caminho completo
    String fileName = filePath;
    int lastSlash = fileName.lastIndexOf('/');
    if (lastSlash != -1)
    {
        fileName = fileName.substring(lastSlash + 1);
    }

    LOG_INFO("📥 Iniciando download do arquivo: %s (%d bytes)", filePath.c_str(), file.size());

    // ✅ CORREÇÃO CRÍTICA: Configurar headers corretamente para download
    _server->setContentLength(file.size());

    // ✅ HEADERS CORRETOS para forçar download
    _server->sendHeader("Content-Type", "application/octet-stream");
    _server->sendHeader("Content-Disposition", "attachment; filename=\"" + fileName + "\"");
    _server->sendHeader("Content-Transfer-Encoding", "binary");
    _server->sendHeader("Cache-Control", "no-cache");
    _server->sendHeader("Pragma", "no-cache");
    _server->sendHeader("Expires", "0");

    // ✅ Enviar status 200 antes do conteúdo
    _server->send(200);

    // ✅ Enviar o arquivo em chunks menores
    uint8_t buffer[512]; // Tamanho menor para melhor controle
    size_t totalSent = 0;

    while (file.available())
    {
        size_t bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
            // ✅ Usar sendContent (não sendContent_P) para dados dinâmicos
            _server->sendContent((const char *)buffer, bytesRead);
            totalSent += bytesRead;
        }

        // ✅ Pequena pausa para estabilidade
        delay(1);
    }

    file.close();

    LOG_INFO("✅ Download concluído: %s (%d bytes enviados)", filePath.c_str(), totalSent);

    // ✅ NÃO chamar client().stop() - deixa o servidor fechar naturalmente
}

void OTAPushUpdateManager::handleFilesystem()
{
    if (!checkAuthentication())
        return;

    String theme = _server->hasArg("theme") ? _server->arg("theme") : "dark";
    bool darkMode = (theme == "dark");

    String path = _server->hasArg("path") ? _server->arg("path") : "/";
    if (!path.startsWith("/"))
    {
        path = "/" + path;
    }

    String content = getFilesystemContent(path);

    // Processa o template específico do filesystem
    String html = htmlFilesystem;

    bool isDarkMode = darkMode;
    if (!_server->hasArg("theme"))
    {
        isDarkMode = true; // ✅ PADRÃO DARK
    }

    html.replace("{{TITLE}}", "File Manager");
    html.replace("{{CONTENT}}", content);
    html.replace("{{THEME}}", isDarkMode ? "dark" : "light");
    html.replace("{{THEME_BUTTON}}", isDarkMode ? "☀️" : "🌙");
    html.replace("{{HOST_INFO}}", _mdnsHostname != "" ? _mdnsHostname + ".local" : WiFi.localIP().toString());
    html.replace("{{CSS}}", cssStyles);
    html.replace("{{JS}}", jsScript);
    html.replace("{{ESP32_TIME}}", getCurrentDateTime());
    html.replace("{{UPTIME}}", formatUptime(millis()));
    html.replace("{{BREADCRUMB}}", generateBreadcrumb(path));
    html.replace("{{CURRENT_PATH}}", path);
    html.replace("{{FILE_LIST}}", generateFileList(path));

    _server->send(200, "text/html", html);
}

void OTAPushUpdateManager::handleFilesystemUpload()
{
    if (!checkAuthentication())
        return;

    // ✅ VERIFICA SE O FILESYSTEM ESTÁ MONTADO
    if (!isLittleFSMounted())
    {
        _server->send(500, "application/json",
                      "{\"status\":\"error\", \"message\":\"Sistema de arquivos não disponível\"}");
        return;
    }

    HTTPUpload &upload = _server->upload();
    static String uploadPath = "/"; // Padrão para raiz

    if (upload.status == UPLOAD_FILE_START)
    {
        // Obtém o path do formulário ou usa o padrão
        if (_server->hasArg("path"))
        {
            uploadPath = _server->arg("path");

            // ✅ CORREÇÃO CRÍTICA: Garantir que o path comece com / e não termine com /
            if (!uploadPath.startsWith("/"))
            {
                uploadPath = "/" + uploadPath;
            }
            if (uploadPath.endsWith("/") && uploadPath != "/")
            {
                uploadPath = uploadPath.substring(0, uploadPath.length() - 1);
            }
        }
        else
        {
            uploadPath = "/";
        }

        LOG_INFO("📤 Iniciando upload para: %s/%s", uploadPath.c_str(), upload.filename.c_str());
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        // ✅ VERIFICA NOVAMENTE ANTES DE ESCREVER
        if (!isLittleFSMounted())
        {
            LOG_ERROR("❌ Filesystem não disponível durante upload");
            return;
        }

        // ✅ CORREÇÃO: Construir o caminho completo corretamente
        String fullPath;
        if (uploadPath == "/")
        {
            fullPath = "/" + upload.filename;
        }
        else
        {
            fullPath = uploadPath + "/" + upload.filename;
        }

        // ✅ CORREÇÃO: Criar diretórios se necessário
        String dirPath = getDirectoryPath(fullPath);

        if (!LittleFS.exists(dirPath))
        {
            LOG_INFO("📁 Criando diretório: %s", dirPath.c_str());
            if (!createDirectories(dirPath))
            {
                LOG_ERROR("❌ Falha ao criar diretório: %s", dirPath.c_str());
                return;
            }
        }

        // ✅ CORREÇÃO: Abrir arquivo no modo append para escrever em chunks
        File file = LittleFS.open(fullPath, "a");
        if (file)
        {
            size_t written = file.write(upload.buf, upload.currentSize);
            file.close();
            LOG_DEBUG("📝 Escritos %d bytes em: %s", written, fullPath.c_str());
        }
        else
        {
            LOG_ERROR("❌ Não foi possível criar/abrir arquivo: %s", fullPath.c_str());
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        LOG_INFO("✅ Upload concluído: %s/%s (%u bytes)",
                 uploadPath.c_str(), upload.filename.c_str(), upload.totalSize);
        _server->send(200, "application/json",
                      "{\"status\":\"success\", \"message\":\"Arquivo salvo com sucesso\"}");
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        LOG_ERROR("❌ Upload abortado: %s/%s", uploadPath.c_str(), upload.filename.c_str());
        _server->send(500, "application/json",
                      "{\"status\":\"error\", \"message\":\"Upload abortado\"}");
    }
}

String OTAPushUpdateManager::getDirectoryPath(const String &fullPath)
{
    int lastSlash = fullPath.lastIndexOf('/');
    if (lastSlash <= 0)
    {
        return "/";
    }
    return fullPath.substring(0, lastSlash);
}

bool OTAPushUpdateManager::createDirectories(const String &path)
{
    if (path == "/" || path == "")
    {
        return true;
    }

    if (LittleFS.exists(path))
    {
        return true;
    }

    // Cria diretórios recursivamente
    String currentPath = "";
    int start = 0;

    while (start < path.length())
    {
        int end = path.indexOf('/', start + 1);
        if (end == -1)
        {
            end = path.length();
        }

        String part = path.substring(start, end);
        currentPath += part;

        if (!LittleFS.exists(currentPath) && currentPath != "/")
        {
            if (!LittleFS.mkdir(currentPath))
            {
                LOG_ERROR("❌ Falha ao criar diretório: %s", currentPath.c_str());
                return false;
            }
            LOG_DEBUG("📁 Diretório criado: %s", currentPath.c_str());
        }

        start = end;
    }

    return true;
}

void OTAPushUpdateManager::handleFilesystemDelete()
{
    if (!checkAuthentication())
        return;

    if (!isLittleFSMounted())
    {
        _server->send(500, "application/json",
                      "{\"status\":\"error\", \"message\":\"Sistema de arquivos não disponível\"}");
        return;
    }

    String path = _server->arg("path");
    bool isDir = _server->arg("isDir") == "true";

    if (path == "" || path == "/")
    {
        _server->send(400, "application/json",
                      "{\"status\":\"error\", \"message\":\"Caminho inválido\"}");
        return;
    }

    LOG_INFO("🗑️ Excluindo: %s (isDir: %d)", path.c_str(), isDir);

    if (isDir)
    {
        if (LittleFS.rmdir(path))
        {
            _server->send(200, "application/json",
                          "{\"status\":\"success\", \"message\":\"Pasta excluída com sucesso\"}");
        }
        else
        {
            _server->send(500, "application/json",
                          "{\"status\":\"error\", \"message\":\"Falha ao excluir pasta\"}");
        }
    }
    else
    {
        if (LittleFS.remove(path))
        {
            _server->send(200, "application/json",
                          "{\"status\":\"success\", \"message\":\"Arquivo excluído com sucesso\"}");
        }
        else
        {
            _server->send(500, "application/json",
                          "{\"status\":\"error\", \"message\":\"Falha ao excluir arquivo\"}");
        }
    }
}

void OTAPushUpdateManager::handleFilesystemMkdir()
{
    if (!checkAuthentication())
        return;

    if (!isLittleFSMounted())
    {
        _server->send(500, "application/json",
                      "{\"status\":\"error\", \"message\":\"Sistema de arquivos não disponível\"}");
        return;
    }

    String path = _server->arg("path");

    if (path == "" || LittleFS.exists(path))
    {
        _server->send(400, "application/json",
                      "{\"status\":\"error\", \"message\":\"Caminho inválido ou já existe\"}");
        return;
    }

    LOG_INFO("📁 Criando pasta: %s", path.c_str());

    if (LittleFS.mkdir(path))
    {
        _server->send(200, "application/json",
                      "{\"status\":\"success\", \"message\":\"Pasta criada com sucesso\"}");
    }
    else
    {
        _server->send(500, "application/json",
                      "{\"status\":\"error\", \"message\":\"Falha ao criar pasta\"}");
    }
}

String OTAPushUpdateManager::getFilesystemContent(const String &currentPath)
{
    String content = "";

    // Breadcrumb navigation
    content += generateBreadcrumb(currentPath);

    // File list
    content += generateFileList(currentPath);

    return content;
}

String OTAPushUpdateManager::generateBreadcrumb(const String &currentPath)
{
    String breadcrumb = "";
    String accumulatedPath = "";

    // ✅ OBTER O TEMA ATUAL para incluir nos links
    String theme = _server->hasArg("theme") ? _server->arg("theme") : "dark";

    // Split path into parts
    String path = currentPath;
    if (path.endsWith("/") && path != "/")
    {
        path = path.substring(0, path.length() - 1);
    }

    // Se for a raiz, não mostra breadcrumb adicional
    if (path == "/")
    {
        return "";
    }

    int start = 0;
    while (start < path.length())
    {
        int end = path.indexOf('/', start + 1);
        if (end == -1)
            end = path.length();

        String part = path.substring(start, end);
        accumulatedPath += part;

        if (part != "/" && part != "")
        {
            String displayName = part;
            if (displayName.startsWith("/"))
            {
                displayName = displayName.substring(1);
            }
            // ✅ CORREÇÃO: Incluir o tema no breadcrumb
            breadcrumb += " / <a href=\"/filesystem?path=" + accumulatedPath + "&theme=" + theme + "\">" + displayName + "</a>";
        }

        start = end;
    }

    return breadcrumb;
}

String OTAPushUpdateManager::generateFileList(const String &currentPath)
{
    String fileList = "";
    bool hasFiles = false;

    if (!isLittleFSMounted())
    {
        return "<div class=\"empty-state\"><div class=\"icon\">❌</div><p>Erro: Sistema de arquivos não disponível</p></div>";
    }

    File root = LittleFS.open(currentPath);
    if (!root)
    {
        return "<div class=\"empty-state\"><div class=\"icon\">❌</div><p>Erro ao acessar o diretório</p></div>";
    }

    if (!root.isDirectory())
    {
        root.close();
        return "<div class=\"empty-state\"><div class=\"icon\">📄</div><p>Não é um diretório</p></div>";
    }

    // ✅ OBTER O TEMA ATUAL para incluir nos links
    String theme = _server->hasArg("theme") ? _server->arg("theme") : "dark";

    fileList += "<table class=\"file-list\">";
    fileList += "<thead><tr><th>Nome</th><th>Tamanho</th><th>Tipo</th><th>Ações</th></tr></thead><tbody>";

    // Primeiro lista os diretórios
    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            hasFiles = true;
            String fileName = String(file.name());

            // Remove o caminho atual do nome de exibição
            if (fileName.startsWith(currentPath))
            {
                fileName = fileName.substring(currentPath.length());
            }
            if (fileName.endsWith("/"))
            {
                fileName = fileName.substring(0, fileName.length() - 1);
            }

            String fullPath = currentPath;
            if (!fullPath.endsWith("/"))
                fullPath += "/";
            fullPath += fileName;

            fileList += "<tr class=\"file-item\">";
            fileList += "<td><span class=\"file-icon\">📁</span>";

            // ✅ CORREÇÃO: Incluir o tema no link de navegação
            fileList += "<a href=\"/filesystem?path=" + fullPath + "&theme=" + theme + "\" style=\"text-decoration: none; color: inherit;\">";
            fileList += "<div style=\"padding: 8px; border-radius: 4px; cursor: pointer; display: inline-block;\">";
            fileList += fileName;
            fileList += "</div>";
            fileList += "</a>";

            fileList += "</td>";
            fileList += "<td>-</td>";
            fileList += "<td>Pasta</td>";
            fileList += "<td class=\"file-actions-cell\">";
            fileList += "<button class=\"btn btn-sm btn-primary\" onclick=\"downloadFile('" +
                        fullPath + "')\" title=\"Baixar arquivo\">⬇️</button>";
            fileList += "<button class=\"btn btn-sm btn-danger\" onclick=\"showDeleteModal('" +
                        fullPath + "', false, '" + fileName + "')\">🗑️</button>";
            fileList += "</td></tr>";
        }
        file = root.openNextFile();
    }

    root.rewindDirectory();

    // Depois lista os arquivos
    file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            hasFiles = true;
            String fileName = String(file.name());

            // Remove o caminho atual do nome de exibição
            if (fileName.startsWith(currentPath))
            {
                fileName = fileName.substring(currentPath.length());
            }

            String fullPath = currentPath;
            if (!fullPath.endsWith("/"))
                fullPath += "/";
            fullPath += fileName;

            fileList += "<tr class=\"file-item\">";
            fileList += "<td><span class=\"file-icon\">📄</span>";
            fileList += "<div style=\"padding: 8px;\">" + fileName + "</div>";
            fileList += "</td>";
            fileList += "<td>" + formatFileSize(file.size()) + "</td>";
            fileList += "<td>" + getFileExtension(fileName) + "</td>";

            fileList += "<td class=\"file-actions-cell\">";
            fileList += "<button class=\"btn btn-sm btn-primary\" onclick=\"downloadFile('" +
                        fullPath + "')\" title=\"Baixar arquivo\">⬇️</button>";
            fileList += "<button class=\"btn btn-sm btn-danger\" onclick=\"showDeleteModal('" +
                        fullPath + "', false, '" + fileName + "')\">🗑️</button>";
            fileList += "</td></tr>";
        }
        file = root.openNextFile();
    }

    fileList += "</tbody></table>";
    root.close();

    if (!hasFiles)
    {
        return "<div class=\"empty-state\"><div class=\"icon\">📁</div><p>Pasta vazia</p></div>";
    }

    return fileList;
}

String OTAPushUpdateManager::formatFileSize(size_t bytes)
{
    if (bytes < 1024)
        return String(bytes) + " B";
    else if (bytes < 1024 * 1024)
        return String(bytes / 1024.0, 1) + " KB";
    else
        return String(bytes / (1024.0 * 1024.0), 1) + " MB";
}

String OTAPushUpdateManager::getFileExtension(const String &filename)
{
    int dotIndex = filename.lastIndexOf('.');
    if (dotIndex == -1)
        return "Arquivo";

    String ext = filename.substring(dotIndex + 1);
    ext.toLowerCase();

    if (ext == "txt" || ext == "log")
        return "Texto";
    else if (ext == "json")
        return "JSON";
    else if (ext == "html" || ext == "htm")
        return "HTML";
    else if (ext == "css")
        return "CSS";
    else if (ext == "js")
        return "JavaScript";
    else if (ext == "bin")
        return "Binário";
    else if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif")
        return "Imagem";
    else
        return ext + " File";
}

String OTAPushUpdateManager::getFullSystemInfoContent()
{
    String content = "";
    content += "<div class=\"info-grid\">";

    // Network Card
    content += "<div class=\"info-card\">";
    content += "<h3>Network</h3>";
    content += "<p><strong>Local IP:</strong> " + WiFi.localIP().toString() + "</p>";
    if (_mdnsHostname != "")
    {
        content += "<p><strong>mDNS:</strong> " + _mdnsHostname + ".local</p>";
    }
    content += "<p><strong>SSID:</strong> " + WiFi.SSID() + "</p>";
    content += "<p><strong>RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
    content += "<p><strong>MAC:</strong> " + WiFi.macAddress() + "</p>";
    content += "</div>";

    // Memory Card
    content += "<div class=\"info-card\">";
    content += "<h3>Memory</h3>";
    content += "<p><strong>Heap Free:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    content += "<p><strong>Heap Size:</strong> " + String(ESP.getHeapSize()) + " bytes</p>";
    content += "<p><strong>PSRAM Size:</strong> " + String(ESP.getPsramSize()) + " bytes</p>";
    content += "<p><strong>PSRAM Free:</strong> " + String(ESP.getFreePsram()) + " bytes</p>";
    content += "</div>";

    // Hardware Card
    content += "<div class=\"info-card\">";
    content += "<h3>Hardware</h3>";
    content += "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    content += "<p><strong>Flash Size:</strong> " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</p>";
    content += "<p><strong>SDK Version:</strong> " + String(ESP.getSdkVersion()) + "</p>";
    content += "<p><strong>Chip Model:</strong> ESP32-S3</p>";
    content += "</div>";

    // System Card
    content += "<div class=\"info-card\">";
    content += "<h3>System</h3>";

#ifdef FIRMWARE_VERSION
    content += "<p><strong>Version:</strong> " + String(OTAManager::getFirmwareVersion().c_str()) + "</p>";
#else
    content += "<p><strong>Version:</strong> 1.0.0</p>"; // Fallback
#endif

    // Uptime formatado
    String uptimeStr = formatUptime(millis());
    content += "<p><strong>Uptime:</strong> " + uptimeStr + "</p>";

    content += "<p><strong>Firmware built:</strong> " + formatBuildDate() + "  " + String(__TIME__) + "</p>";

    content += "<p><strong>Reset Reason:</strong> " + resetReason(esp_reset_reason()) + "</p>";
    content += "<p><strong>Cycle Count:</strong> " + String(ESP.getCycleCount()) + "</p>";
    content += "</div>";

    content += "</div>";

    return content;
}

bool OTAPushUpdateManager::isLittleFSMounted()
{
    if (!LittleFS.begin(true))
    {
        LOG_ERROR("❌ LittleFS não está montado");
        return false;
    }
    return true;
}