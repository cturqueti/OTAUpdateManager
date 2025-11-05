#include "OTAManager.h"

// Inicialização de variáveis estáticas
OTAManager::UpdateMode OTAManager::_currentMode = OTAManager::HYBRID;
String OTAManager::_serverUrl = "";
bool OTAManager::_updateAvailable = false;
String OTAManager::_latestVersion = "";
String OTAManager::_currentVersion = FIRMWARE_VERSION;

void OTAManager::begin(const String &serverUrl, uint16_t webPort, UpdateMode mode)
{
    init();
    _serverUrl = serverUrl;
    _currentMode = mode;

    // Sempre inicia o sistema Push (web)
    OTAPushUpdateManager::begin(webPort);

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

void OTAManager::setUpdateMode(UpdateMode mode)
{
    _currentMode = mode;

    if (mode == AUTOMATIC && !_serverUrl.isEmpty())
    {
        OTAPullUpdateManager::startUpdateThread(5);
    }
    else
    {
        OTAPullUpdateManager::stopUpdateThread();
    }

    LOG_INFO("🔄 Modo OTA alterado para: %s",
             mode == MANUAL ? "Manual" : mode == AUTOMATIC ? "Automático"
                                                           : "Híbrido");
}

void OTAManager::setServerUrl(const String &serverUrl)
{
    _serverUrl = serverUrl;
    if (!serverUrl.isEmpty() && _currentMode != MANUAL)
    {
        OTAPullUpdateManager::init(serverUrl);
    }
}

void OTAManager::setWebCredentials(const String &username, const String &password)
{
    OTAPushUpdateManager::setCredentials(username, password);
}

void OTAManager::setMDNS(const String &hostname)
{
    OTAPushUpdateManager::setMDNS(hostname);
}

void OTAManager::setPullInterval(uint16_t minutes)
{
    if (_currentMode != MANUAL)
    {
        OTAPullUpdateManager::stopUpdateThread();
        OTAPullUpdateManager::startUpdateThread(minutes);
    }
}

void OTAManager::handleClient()
{
    OTAPushUpdateManager::handleClient();
}

void OTAManager::checkForUpdates()
{
    if (_serverUrl.isEmpty() || _currentMode == MANUAL)
    {
        return;
    }

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