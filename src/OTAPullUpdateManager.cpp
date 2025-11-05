#include "OTAPullUpdateManager.h"
#include "OTAManager.h"
#include "WiFi.h"

// ============ INICIALIZAÇÃO DE VARIÁVEIS ESTÁTICAS ============

String OTAPullUpdateManager::_firmwareUrl = "";
String OTAPullUpdateManager::_versionUrl = "";
bool OTAPullUpdateManager::_updating = false;
uint16_t OTAPullUpdateManager::_serverPort = 8000;
String OTAPullUpdateManager::_versionPath = "/version";
String OTAPullUpdateManager::_firmwarePath = "/firmware";

TaskHandle_t OTAPullUpdateManager::_updateTaskHandle = nullptr;
bool OTAPullUpdateManager::_threadRunning = false;
uint32_t OTAPullUpdateManager::_checkIntervalMs = 60000;

// ============ IMPLEMENTAÇÃO DOS MÉTODOS ============

bool OTAPullUpdateManager::buildUrls(const String &serverUrl)
{
    String baseUrl = serverUrl;

    // Normalização da URL base
    if (baseUrl.endsWith("/"))
    {
        baseUrl = baseUrl.substring(0, baseUrl.length() - 1);
    }

    // Adiciona protocolo se necessário
    if (!baseUrl.startsWith("http://") && !baseUrl.startsWith("https://"))
    {
        baseUrl = "http://" + baseUrl;
    }

    // Remove porta existente para evitar conflito
    int portColon = baseUrl.indexOf(':', baseUrl.indexOf("//") + 2);
    if (portColon != -1)
    {
        baseUrl = baseUrl.substring(0, portColon);
    }

    // Constrói URLs completas
    _versionUrl = baseUrl + ":" + String(_serverPort) + _versionPath;
    _firmwareUrl = baseUrl + ":" + String(_serverPort) + _firmwarePath;

    LOG_DEBUG("URLs construídas - Versão: %s, Firmware: %s",
              _versionUrl.c_str(), _firmwareUrl.c_str());

    return true;
}

bool OTAPullUpdateManager::loadStoredVersion()
{
    if (!LittleFS.begin(true))
    {
        LOG_ERROR("Falha ao montar LittleFS");
        return false;
    }

    // Verifica se arquivo de versão existe
    if (!LittleFS.exists("/ota_version.txt"))
    {
        LOG_WARN("Nenhuma versão armazenada encontrada, usando padrão");
        LittleFS.end();
        return false;
    }

    File file = LittleFS.open("/ota_version.txt", "r");
    if (!file)
    {
        LOG_ERROR("Falha ao abrir arquivo de versão");
        LittleFS.end();
        return false;
    }

    file.close();
    LittleFS.end();

    LOG_INFO("Versão armazenada carregada: %s", OTAManager::getFirmwareVersion().c_str());
    return true;
}

void OTAPullUpdateManager::init(const String &serverUrl)
{
    buildUrls(serverUrl);
    loadStoredVersion();

    LOG_INFO("Gerenciador OTA HTTP inicializado (configurações padrão)");
    LOG_INFO("URL do Firmware: %s", _firmwareUrl.c_str());
    LOG_INFO("URL da Versão: %s", _versionUrl.c_str());

    checkForUpdates();
}

void OTAPullUpdateManager::init(const String &serverUrl, uint16_t port,
                                const String &versionPath,
                                const String &firmwarePath)
{
    _serverPort = port;
    _versionPath = versionPath;
    _firmwarePath = firmwarePath;

    buildUrls(serverUrl);
    loadStoredVersion();

    LOG_INFO("Gerenciador OTA HTTP inicializado (configurações customizadas)");
    LOG_INFO("Porta: %d, Caminho Versão: %s, Caminho Firmware: %s", _serverPort,
             _versionPath.c_str(), _firmwarePath.c_str());

    checkForUpdates();
}

void OTAPullUpdateManager::setServerPort(uint16_t port)
{
    _serverPort = port;
    LOG_INFO("Porta do servidor definida para: %d", _serverPort);
}

void OTAPullUpdateManager::setVersionPath(const String &path)
{
    _versionPath = path;
    if (!_versionPath.startsWith("/"))
    {
        _versionPath = "/" + _versionPath;
    }
    LOG_INFO("Caminho da versão definido para: %s", _versionPath.c_str());
}

void OTAPullUpdateManager::setFirmwarePath(const String &path)
{
    _firmwarePath = path;
    if (!_firmwarePath.startsWith("/"))
    {
        _firmwarePath = "/" + _firmwarePath;
    }
    LOG_INFO("Caminho do firmware definido para: %s", _firmwarePath.c_str());
}

void OTAPullUpdateManager::checkForUpdates()
{
    if (_updating || WiFi.status() != WL_CONNECTED)
    {
        LOG_DEBUG("Verificação de atualizações ignorada: atualização em "
                  "andamento ou WiFi desconectado");
        return;
    }

    LOG_INFO("🔍 Verificando atualizações de firmware...");

    if (checkVersion())
    {
        LOG_INFO("🎯 Nova versão disponível! Iniciando download...");
        if (downloadFirmware())
        {
            LOG_INFO("🔄 Firmware atualizado com sucesso. Reiniciando...");
            delay(500);
            ESP.restart();
        }
    }
    else
    {
        LOG_DEBUG("✅ Firmware está atualizado");
    }
}

bool OTAPullUpdateManager::checkVersion()
{
    // Validações iniciais
    if (_versionUrl.length() == 0)
    {
        LOG_ERROR("URL de versão está vazia!");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_ERROR("WiFi não conectado!");
        return false;
    }

    String versionUrlCopy = _versionUrl; // Cópia para segurança

    LOG_DEBUG("Conectando à: %s", versionUrlCopy.c_str());

    HTTPClient http;
    http.begin(versionUrlCopy);
    http.setTimeout(10000);
    http.setUserAgent("ESP32-OTA-Client");
    http.addHeader("Cache-Control", "no-cache");

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String serverVersion = http.getString();
        serverVersion.trim();

        // Compare servidor vs atual (mais intuitivo)
        OTAManager::VersionComparison comparisonResult = OTAManager::compareVersions(serverVersion, OTAManager::getFirmwareVersion());

        if (comparisonResult == OTAManager::VersionComparison::VERSION_EQUAL)
        {
            LOG_INFO("📋 Versões iguais - servidor: %s, atual: %s",
                     serverVersion.c_str(), OTAManager::getFirmwareVersion().c_str());
            http.end();
            return false;
        }
        else if (comparisonResult == OTAManager::VersionComparison::VERSION_NEWER)
        {
            LOG_INFO("🎯 ATUALIZAÇÃO DISPONÍVEL - servidor: %s é MAIS NOVA que atual: %s",
                     serverVersion.c_str(), OTAManager::getFirmwareVersion().c_str());
            http.end();
            return true; // COM atualização
        }
        else // VERSION_OLDER
        {
            LOG_INFO("📋 Versão servidor %s é MAIS ANTIGA que atual %s - mantendo",
                     serverVersion.c_str(), OTAManager::getFirmwareVersion().c_str());
            http.end();
            return false;
        }
    }
    else
    {
        LOG_ERROR("Falha ao verificar versão. Código HTTP: %d, URL: %s",
                  httpCode, versionUrlCopy.c_str());

        if (httpCode < 0)
        {
            String errorMsg = http.errorToString(httpCode);
            LOG_ERROR("Erro do cliente HTTP: %s", errorMsg.c_str());
        }
    }

    http.end();
    return false;
}

bool OTAPullUpdateManager::downloadFirmware()
{
    _updating = true;

    HTTPClient http;
    http.begin(_firmwareUrl);
    http.setTimeout(60000);

    LOG_INFO("🚀 Iniciando download do firmware de: %s", _firmwareUrl.c_str());

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        int contentLength = http.getSize();
        LOG_INFO("📦 Tamanho do firmware: %d bytes", contentLength);

        if (contentLength <= 0)
        {
            LOG_ERROR("❌ Tamanho do firmware inválido");
            _updating = false;
            return false;
        }

        if (!Update.begin(contentLength))
        {
            LOG_ERROR("❌ Espaço insuficiente para atualização OTA");
            _updating = false;
            return false;
        }

        // Limpar área do terminal para progresso
        for (int i = 0; i < 3; i++)
            Serial.println();

        WiFiClient *stream = http.getStreamPtr();
        uint8_t buffer[1024];
        size_t totalRead = 0;
        int lastProgress = -1;

        while (http.connected() && totalRead < contentLength)
        {
            size_t bytesRead = stream->readBytes(buffer, sizeof(buffer));
            if (bytesRead > 0)
            {
                Update.write(buffer, bytesRead);
                totalRead += bytesRead;

                if (contentLength > 0)
                {
                    int progress = (totalRead * 100) / contentLength;
                    if (progress != lastProgress)
                    {
                        Serial.print("\r🔄 Progresso do download: ");
                        Serial.print(progress);
                        Serial.print("%");
                        lastProgress = progress;
                    }
                }
            }
            else
            {
                delay(10);
            }
        }

        // Finalizar visualização do progresso
        Serial.println("\r✅ Download concluído: 100%");
        for (int i = 0; i < 2; i++)
            Serial.println();

        if (Update.end())
        {
            // ✅ CORREÇÃO GARANTIDA: Sempre atualizar a versão no LittleFS após atualização bem-sucedida

            String serverVersion = "unknown";
            bool versionObtained = false;

            // Tentar obter a versão atualizada do servidor
            HTTPClient versionHttp;
            versionHttp.begin(_versionUrl);
            versionHttp.setTimeout(5000);

            int versionCode = versionHttp.GET();
            if (versionCode == HTTP_CODE_OK)
            {
                serverVersion = versionHttp.getString();
                serverVersion.trim();
                versionObtained = true;
                LOG_INFO("🎉 Versão confirmada no servidor: %s", serverVersion.c_str());
            }
            else
            {
                // Se não conseguir obter do servidor, usar fallback seguro
                LOG_WARN("⚠️ Não foi possível obter versão do servidor, usando fallback");

                // Fallback: obter a versão mais recente do servidor
                String latestVersion = getLatestVersion();
                String currentVersion = OTAManager::getFirmwareVersion();

                if (latestVersion != "unknown" && !latestVersion.isEmpty())
                {
                    serverVersion = latestVersion; // Usar a versão mais recente que obtivemos
                    versionObtained = true;
                }
                else
                {
                    // Fallback final: adicionar sufixo de atualização com timestamp
                    serverVersion = currentVersion + ".updated";
                }
            }
            versionHttp.end();

            // ✅ ATUALIZAÇÃO OBRIGATÓRIA: Sempre salvar a versão
            OTAManager::setFirmwareVersion(serverVersion);
            LOG_INFO("💾 Versão salva no LittleFS: %s", serverVersion.c_str());

            LOG_INFO("✨ Atualização de firmware concluída com sucesso");
            _updating = false;
            return true;
        }
        else
        {
            LOG_ERROR("💥 Falha na atualização do firmware: %s",
                      Update.errorString());
            _updating = false;
            return false;
        }
    }
    else
    {
        LOG_ERROR("❌ Download do firmware falhou. Código HTTP: %d", httpCode);
        _updating = false;
        return false;
    }

    http.end();
    return false;
}

bool OTAPullUpdateManager::isUpdating() { return _updating; }

String OTAPullUpdateManager::getCurrentVersion() { return OTAManager::getFirmwareVersion(); }

String OTAPullUpdateManager::getLatestVersion()
{
    // Implementação simples - você pode melhorar isso
    if (_versionUrl.length() == 0)
    {
        return "unknown";
    }

    HTTPClient http;
    http.begin(_versionUrl);
    http.setTimeout(5000);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        String version = http.getString();
        version.trim();
        http.end();
        return version;
    }

    http.end();
    return "unknown";
}

// ============ GERENCIAMENTO DE THREAD ============

void OTAPullUpdateManager::updateTask(void *parameter)
{
    ThreadParams *params = static_cast<ThreadParams *>(parameter);
    uint32_t checkIntervalMs = params->checkIntervalMs;
    delete params; // Liberar memória alocada
    vTaskDelay(pdMS_TO_TICKS(5000));
    LOG_INFO("🔄 Thread de verificação de atualizações iniciada");

    while (_threadRunning)
    {
        if (WiFi.status() == WL_CONNECTED && !_updating)
        {
            LOG_DEBUG("Thread: Verificando atualizações...");
            checkForUpdates();
        }
        else
        {
            LOG_DEBUG(
                "⏸️ Thread: Aguardando WiFi ou conclusão de atualização...");
        }

        // Aguardar intervalo especificado
        uint32_t waitSeconds = checkIntervalMs / 1000;
        for (uint32_t i = 0; i < waitSeconds && _threadRunning; i++)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    LOG_INFO("🛑 Thread de verificação de atualizações finalizada");
    vTaskDelete(nullptr);
}

void OTAPullUpdateManager::startUpdateThread(uint16_t checkIntervalMinutes)
{
    if (_threadRunning)
    {
        LOG_WARN("Thread de atualização já está em execução");
        return;
    }

    _checkIntervalMs = checkIntervalMinutes * 60 * 1000;
    _threadRunning = true;

    // Preparar parâmetros para a thread
    ThreadParams *params = new ThreadParams();
    params->checkIntervalMs = _checkIntervalMs;

    BaseType_t result = xTaskCreate(updateTask,          // Função da task
                                    "HTTPUpdateChecker", // Nome da task
                                    4096,                // Stack size
                                    params,              // Parâmetros
                                    1,                   // Prioridade (baixa)
                                    &_updateTaskHandle   // Handle da task
    );

    if (result == pdPASS && _updateTaskHandle != nullptr)
    {
        LOG_INFO("✅ Thread de verificação criada (intervalo: %u minutos)",
                 checkIntervalMinutes);
    }
    else
    {
        LOG_ERROR("❌ Falha ao criar thread de verificação");
        _threadRunning = false;
        delete params;
    }
}

void OTAPullUpdateManager::stopUpdateThread()
{
    if (!_threadRunning)
        return;

    LOG_INFO("Parando thread de verificação de atualizações...");
    _threadRunning = false;

    if (_updateTaskHandle != nullptr)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        _updateTaskHandle = nullptr;
    }

    LOG_INFO("Thread de verificação de atualizações parada");
}

bool OTAPullUpdateManager::isThreadRunning() { return _threadRunning; }