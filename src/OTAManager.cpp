#include "OTAManager.h"
#include "WiFiManager.h"
#include "handlers/WebAssetManager.h"
#include <Preferences.h>

// Inicialização de variáveis estáticas
OTAManager::UpdateMode OTAManager::_currentMode = OTAManager::HYBRID;
String OTAManager::_serverUrl = "";
bool OTAManager::_updateAvailable = false;
String OTAManager::_latestVersion = "";
String OTAManager::_currentVersion = FIRMWARE_VERSION;
// String OTAManager::_wifiMemoryAddress = "wifi-config";

bool OTAManager::_autoUpdateEnabled = true; // Padrão: habilitado
int OTAManager::_updateIntervalHours = 24;  // Padrão: 24 horas
unsigned long OTAManager::_lastUpdateCheck = 0;
unsigned long OTAManager::_lastConfigSave = 0;

String OTAManager::_wifiSSID = "";
String OTAManager::_wifiPassword = "";

Preferences preferences;

void OTAManager::begin(const String &serverUrl, uint16_t webPort, UpdateMode mode)
{
    init();
    _serverUrl = serverUrl;
    _currentMode = mode;

    // Sempre inicia o sistema Push (web)
    OTAPushUpdateManager::begin(webPort);

    LOG_INFO("📝 Chamando WebAssetManager::setupRoutes()");
    AsyncWebServer *server = OTAPushUpdateManager::getServer();
    if (server)
    {
        LOG_INFO("🔍 Server obtido: %p", server);
        WebAssetManager::setupRoutes(server);
    }
    else
    {
        LOG_ERROR("❌ Não foi possível obter o server do OTAPushUpdateManager");
    }

    LOG_INFO("📝 Chamando WebAssetManager::checkRequiredAssets()");
    WebAssetManager::checkRequiredAssets();

    // ✅ ADICIONAR: Configurar callbacks para o sistema Push
    OTAPushUpdateManager::setPullUpdateCallback([]() -> bool
                                                { return OTAManager::isUpdateAvailable(); });

    OTAPushUpdateManager::setPerformUpdateCallback([]()
                                                   { OTAManager::performUpdate(); });

    OTAPushUpdateManager::run(); // Inicia thread FreeRTOS

    // Se tem URL de servidor e modo não é MANUAL, inicia o sistema Pull
    if (!serverUrl.isEmpty() && mode != MANUAL)
    {
        OTAPullUpdateManager::init(serverUrl);

        if (mode == AUTOMATIC)
        {
            // Inicia thread de verificação automática a cada 5 minutos
            OTAPullUpdateManager::startUpdateThread(5);
        }
    }

    LOG_INFO("✅ OTA Manager inicializado - Modo: %s",
             mode == MANUAL ? "Manual" : mode == AUTOMATIC ? "Automático"
                                                           : "Híbrido");
}

void OTAManager::end()
{
    OTAPushUpdateManager::stop();
    OTAPullUpdateManager::stopUpdateThread();
}

void OTAManager::setWebCredentials(const String &username, const String &password)
{
    OTAPushUpdateManager::setCredentials(username, password);
}

void OTAManager::setMDNS(const String &hostname)
{
    OTAPushUpdateManager::setMDNS(hostname);
}

bool OTAManager::isUpdateAvailable()
{
    return _updateAvailable;
}

void OTAManager::performUpdate()
{
    if (_updateAvailable && _currentMode != MANUAL)
    {
        LOG_INFO("🚀 Iniciando atualização pull...");
        OTAPullUpdateManager::checkForUpdates();
    }
}

String OTAManager::getServerUrl()
{
    return _serverUrl;
}

void OTAManager::setServerUrl(const String &serverUrl)
{
    _serverUrl = serverUrl;
    saveConfig(); // Salva a configuração

    // Reinicia o sistema pull se necessário
    if (_autoUpdateEnabled && _currentMode != MANUAL && !serverUrl.isEmpty())
    {
        OTAPullUpdateManager::init(serverUrl);
        if (_currentMode == AUTOMATIC)
        {
            OTAPullUpdateManager::startUpdateThread(_updateIntervalHours * 60);
        }
    }

    LOG_INFO("🔧 URL do servidor atualizado: %s", serverUrl.c_str());
}

OTAManager::VersionComparison OTAManager::compareVersions(const String &v1, const String &v2)
{
    // Remove possíveis "v" no início
    String version1 = v1;
    String version2 = v2;

    if (version1.startsWith("v") || version1.startsWith("V"))
    {
        version1 = version1.substring(1);
    }
    if (version2.startsWith("v") || version2.startsWith("V"))
    {
        version2 = version2.substring(1);
    }

    // Arrays para armazenar as partes das versões (major.minor.patch)
    int parts1[3] = {0, 0, 0};
    int parts2[3] = {0, 0, 0};

    // Parse da primeira versão
    int count1 = 0;
    int start1 = 0;
    for (int i = 0; i <= version1.length() && count1 < 3; i++)
    {
        if (i == version1.length() || version1[i] == '.')
        {
            parts1[count1] = version1.substring(start1, i).toInt();
            start1 = i + 1;
            count1++;
        }
    }

    // Parse da segunda versão
    int count2 = 0;
    int start2 = 0;
    for (int i = 0; i <= version2.length() && count2 < 3; i++)
    {
        if (i == version2.length() || version2[i] == '.')
        {
            parts2[count2] = version2.substring(start2, i).toInt();
            start2 = i + 1;
            count2++;
        }
    }

    // Comparação semântica: major -> minor -> patch
    for (int i = 0; i < 3; i++)
    {
        if (parts1[i] > parts2[i])
            return VERSION_NEWER;
        if (parts1[i] < parts2[i])
            return VERSION_OLDER;
    }

    return VERSION_EQUAL;
}

OTAManager::UpdateMode OTAManager::getCurrentMode()
{
    return _currentMode;
}

String OTAManager::getLatestVersion()
{
    return _latestVersion;
}

String OTAManager::getUpdateStatus()
{
    String currentVersion = OTAPullUpdateManager::getCurrentVersion();

    String status = "OTA - Modo: ";
    status += (_currentMode == MANUAL ? "Manual" : _currentMode == AUTOMATIC ? "Automático"
                                                                             : "Híbrido");
    status += " | Versão: v" + currentVersion;

    if (!_serverUrl.isEmpty())
    {
        status += " | Servidor: " + _serverUrl;

        if (_updateAvailable && !_latestVersion.isEmpty())
        {
            VersionComparison comp = compareVersions(_latestVersion, currentVersion);
            String statusText;

            switch (comp)
            {
            case VERSION_NEWER:
                statusText = "ATUALIZAR ↗ v" + _latestVersion;
                break;
            case VERSION_OLDER:
                statusText = "REGREDIR ↘ v" + _latestVersion;
                break;
            case VERSION_EQUAL:
                statusText = "IGUAL ≡ v" + _latestVersion;
                break;
            }

            status += " | Status: " + statusText;
        }
        else
        {
            status += " | Status: Atualizado ✓";
        }
    }

    return status;
}

bool OTAManager::isAutoUpdateEnabled()
{
    return _autoUpdateEnabled;
}

void OTAManager::setAutoUpdateEnabled(bool enabled)
{
    if (_autoUpdateEnabled != enabled)
    {
        _autoUpdateEnabled = enabled;
        saveConfig();

        LOG_INFO("🔧 Auto Update %s", enabled ? "ATIVADO" : "DESATIVADO");

        // Se foi ativado e estamos no modo AUTOMATIC, reinicia a thread
        if (enabled && _currentMode == AUTOMATIC && !_serverUrl.isEmpty())
        {
            OTAPullUpdateManager::startUpdateThread(_updateIntervalHours * 60);
        }
        else if (!enabled)
        {
            // Se foi desativado, para a thread de verificação
            OTAPullUpdateManager::stopUpdateThread();
        }
    }
}

int OTAManager::getUpdateInterval()
{
    return _updateIntervalHours;
}

void OTAManager::setUpdateInterval(int hours)
{
    if (hours < 1)
        hours = 1; // Mínimo 1 hora
    if (hours > 168)
        hours = 168; // Máximo 1 semana

    if (_updateIntervalHours != hours)
    {
        _updateIntervalHours = hours;
        saveConfig();

        LOG_INFO("🔧 Intervalo de atualização alterado para: %d horas", hours);

        // Se auto update está ativado e no modo AUTOMATIC, reinicia a thread com novo intervalo
        if (_autoUpdateEnabled && _currentMode == AUTOMATIC && !_serverUrl.isEmpty())
        {
            OTAPullUpdateManager::stopUpdateThread();
            OTAPullUpdateManager::startUpdateThread(_updateIntervalHours * 60);
        }
    }
}

String OTAManager::getLastUpdateCheck()
{
    if (_lastUpdateCheck == 0)
    {
        return "Nunca";
    }

    unsigned long secondsAgo = (millis() - _lastUpdateCheck) / 1000;

    if (secondsAgo < 60)
    {
        return String(secondsAgo) + " segundos atrás";
    }
    else if (secondsAgo < 3600)
    {
        return String(secondsAgo / 60) + " minutos atrás";
    }
    else if (secondsAgo < 86400)
    {
        return String(secondsAgo / 3600) + " horas atrás";
    }
    else
    {
        return String(secondsAgo / 86400) + " dias atrás";
    }
}

void OTAManager::setWifiCredentials(const String &wifiSSID, const String &wifiPassword)
{
    _wifiSSID = wifiSSID;
    _wifiPassword = wifiPassword;
}

void OTAManager::loadConfig()
{
    if (!LittleFS.begin(true))
    {
        LOG_ERROR("❌ Falha ao montar LittleFS para carregar configurações");
        return;
    }

    if (!LittleFS.exists("/ota_config.json"))
    {
        LOG_WARN("⚠️ Nenhuma configuração salva encontrada, usando padrões");
        LittleFS.end();
        return;
    }

    File file = LittleFS.open("/ota_config.json", "r");
    if (!file)
    {
        LOG_ERROR("❌ Falha ao abrir arquivo de configuração");
        LittleFS.end();
        return;
    }

    String jsonStr = file.readString();
    file.close();
    LittleFS.end();

    // Parse do JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error)
    {
        LOG_ERROR("❌ Erro ao parsear JSON de configuração: %s", error.c_str());
        return;
    }

    // Carrega valores
    _autoUpdateEnabled = doc["autoUpdate"] | true;
    _updateIntervalHours = doc["updateInterval"] | 24;
    _lastUpdateCheck = doc["lastUpdateCheck"] | 0;

    LOG_INFO("✅ Configurações carregadas - AutoUpdate: %s, Intervalo: %d horas",
             _autoUpdateEnabled ? "true" : "false", _updateIntervalHours);
}

void OTAManager::saveConfig()
{
    // Debounce: não salva mais frequentemente que a cada 2 segundos
    if (millis() - _lastConfigSave < 2000)
    {
        return;
    }

    if (!LittleFS.begin(true))
    {
        LOG_ERROR("❌ Falha ao montar LittleFS para salvar configurações");
        return;
    }

    File file = LittleFS.open("/ota_config.json", "w");
    if (!file)
    {
        LOG_ERROR("❌ Falha ao criar arquivo de configuração");
        LittleFS.end();
        return;
    }

    // Cria JSON
    JsonDocument doc;
    doc["autoUpdate"] = _autoUpdateEnabled;
    doc["updateInterval"] = _updateIntervalHours;
    doc["lastUpdateCheck"] = _lastUpdateCheck;

    // Serializa para arquivo
    if (serializeJson(doc, file) == 0)
    {
        LOG_ERROR("❌ Falha ao escrever configurações no arquivo");
    }
    else
    {
        LOG_INFO("💾 Configurações salvas - AutoUpdate: %s, Intervalo: %d horas",
                 _autoUpdateEnabled ? "true" : "false", _updateIntervalHours);
    }

    file.close();
    LittleFS.end();
    _lastConfigSave = millis();
}

void OTAManager::checkForUpdates()
{
    if (_serverUrl.isEmpty() || _currentMode == MANUAL)
    {
        return;
    }

    // ✅ REGISTRA O MOMENTO DA VERIFICAÇÃO
    _lastUpdateCheck = millis();
    saveConfig(); // Salva para persistência

    _latestVersion = OTAPullUpdateManager::getLatestVersion();
    String currentVersion = OTAPullUpdateManager::getCurrentVersion();

    // Verifica se a versão do servidor é válida
    if (_latestVersion == "unknown" || _latestVersion.isEmpty())
    {
        _updateAvailable = false;
        LOG_WARN("⚠️  Versão do servidor inválida: %s", _latestVersion.c_str());
        return;
    }

    // Usa comparação semântica em vez de simples string comparison
    VersionComparison comparison = compareVersions(_latestVersion, currentVersion);

    _updateAvailable = (comparison == VERSION_NEWER);

    if (_updateAvailable)
    {
        LOG_INFO("🎯 ATUALIZAÇÃO DISPONÍVEL! Servidor: v%s (Atual: v%s) ↗",
                 _latestVersion.c_str(), currentVersion.c_str());
    }
    else
    {
        switch (comparison)
        {
        case VERSION_EQUAL:
            LOG_DEBUG("📋 Versões iguais: v%s ≡ v%s",
                      currentVersion.c_str(), _latestVersion.c_str());
            break;
        case VERSION_OLDER:
            LOG_WARN("⚠️  Versão do servidor é MAIS ANTIGA: v%s ↘ v%s",
                     _latestVersion.c_str(), currentVersion.c_str());
            break;
        case VERSION_NEWER:
            // Já tratado acima, mas mantido para completude
            break;
        }
    }
}

String OTAManager::getFirmwareVersion()
{
    return _currentVersion;
}

void OTAManager::setFirmwareVersion(const String &version)
{
    if (writeVersion(version) != ESP_OK)
    {
        LOG_ERROR("Falha ao atualizar versão salva");
        return;
    }
    _currentVersion = version;
    LOG_INFO("✅ Versão do firmware atualizada para: %s", version.c_str());
}

esp_err_t OTAManager::init()
{
    if (!WiFiManager::isConnected())
    {
        // Adicionar redes WiFi se disponíveis
        if (!_wifiSSID.isEmpty())
        {
            WiFiManager::addNetwork(_wifiSSID, _wifiPassword);
        }

        // Iniciar WiFiManager
        WiFiManager::begin();

        // Aguardar conexão por um tempo
        int timeout = 30; // 30 segundos
        while (!WiFiManager::isConnected() && timeout > 0)
        {
            delay(1000);
            timeout--;
            Serial.print(".");
        }

        if (!WiFiManager::isConnected())
        {
            LOG_ERROR("❌ Timeout na conexão WiFi");
            return ESP_ERR_TIMEOUT;
        }
        WiFiManager::setAutoReconnect(true);
    }

    if (!LittleFS.begin(true))
    {
        LOG_ERROR("Falha ao montar LittleFS");
        return ESP_ERR_FLASH_NOT_INITIALISED;
    }

    if (!LittleFS.exists("/ota_version.txt"))
    {
        LOG_WARN("Nenhuma versão armazenada encontrada, usando padrão");
        LittleFS.end();
        return ESP_ERR_NOT_FOUND;
    }

    File file = LittleFS.open("/ota_version.txt", "r");
    if (!file)
    {
        LOG_ERROR("Falha ao abrir arquivo de versão");
        LittleFS.end();
        return ESP_ERR_NOT_SUPPORTED;
    }
    String fileVersion = file.readString();
    file.close();
    LittleFS.end();

    VersionComparison result = compareVersions(FIRMWARE_VERSION, fileVersion);

    LOG_INFO("Versão armazenada: %s", fileVersion.c_str());

    if (result == VERSION_NEWER)
    {
        LOG_INFO("Nova versão disponível, atualizando versão salva...");
        return writeVersion(FIRMWARE_VERSION);
    }
    else if (result == VERSION_OLDER)
    {
        LOG_INFO("Versão no LittleFS é mais nova, mantendo...");
        _currentVersion = fileVersion;
        return ESP_OK;
    }
    else
    {
        LOG_INFO("Versão armazenada igual ao atual");
        _currentVersion = fileVersion;
        return ESP_OK;
    }
}

esp_err_t OTAManager::writeVersion(const String &version)
{
    if (!LittleFS.begin(true))
    {
        LOG_ERROR("Falha ao montar LittleFS");
        return ESP_ERR_FLASH_NOT_INITIALISED;
    }

    File file = LittleFS.open("/ota_version.txt", "w");
    if (!file)
    {
        LOG_ERROR("Falha ao criar arquivo de versão");
        LittleFS.end();
        return ESP_ERR_NOT_SUPPORTED;
    }

    file.print(version);
    file.close();
    LittleFS.end();

    LOG_INFO("Versão atual salva: %s", version.c_str());
    _currentVersion = version;
    return ESP_OK;
}

void OTAManager::setUpdateMode(UpdateMode mode)
{
    _currentMode = mode;

    if (mode == AUTOMATIC && _autoUpdateEnabled && !_serverUrl.isEmpty())
    {
        OTAPullUpdateManager::startUpdateThread(_updateIntervalHours * 60);
    }
    else
    {
        OTAPullUpdateManager::stopUpdateThread();
    }

    LOG_INFO("🔄 Modo OTA alterado para: %s (AutoUpdate: %s)",
             mode == MANUAL ? "Manual" : mode == AUTOMATIC ? "Automático"
                                                           : "Híbrido",
             _autoUpdateEnabled ? "Ativado" : "Desativado");
}

void OTAManager::setPullInterval(uint16_t minutes)
{
    if (_currentMode != MANUAL)
    {
        // Converte minutos para horas e atualiza configuração
        int hours = minutes / 60;
        if (hours < 1)
            hours = 1;

        setUpdateInterval(hours);
    }
}

// esp_err_t OTAManager::initWifi()
// {
//     if (_wifiSSID.isEmpty())
//     {
//         Preferences preferences;
//         preferences.begin(_wifiMemoryAddress.c_str(), true);
//         _wifiSSID = preferences.getString("ssid", "");
//         _wifiPassword = preferences.getString("password", "");
//         preferences.end();
//     }

//     if (!_wifiSSID.isEmpty())
//     {
//         LOG_DEBUG("SSID: %s,\t Senha: %s", _wifiSSID.c_str(), _wifiPassword.c_str());
//         LOG_INFO("Conectando ao WiFi: %s", _wifiSSID.c_str());

//         WiFi.begin(_wifiSSID.c_str(), _wifiPassword.c_str());
//         while (WiFi.status() != WL_CONNECTED)
//         {
//             delay(500);

//             if (_wifiRetryCount < 0)
//             {
//                 LOG_ERROR("❌ Falha ao conectar ao WiFi");
//                 return ESP_ERR_WIFI_BASE;
//             }

//             _wifiRetryCount--;
//             Serial.print(".");
//         }
//         LOG_INFO("Conectado ao WiFi: %s", WiFi.SSID().c_str());
//     }
//     return ESP_OK;
// }
