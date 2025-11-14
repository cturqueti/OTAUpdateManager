#include "OTAManager.h"
#include "handlers/WebAssetManager.h"
#include <Preferences.h>

// Inicialização de variáveis estáticas
OTAManager::UpdateMode OTAManager::_currentMode = OTAManager::HYBRID;
String OTAManager::_serverUrl = "";
bool OTAManager::_updateAvailable = false;
String OTAManager::_latestVersion = "";
String OTAManager::_currentVersion = FIRMWARE_VERSION;

// ✅ NOVAS VARIÁVEIS INICIALIZADAS
bool OTAManager::_autoUpdateEnabled = true; // Padrão: habilitado
int OTAManager::_updateIntervalHours = 24;  // Padrão: 24 horas
unsigned long OTAManager::_lastUpdateCheck = 0;
unsigned long OTAManager::_lastConfigSave = 0;

// ✅ Preferences para armazenamento persistente
Preferences preferences;

void OTAManager::begin(const String &serverUrl, uint16_t webPort, UpdateMode mode)
{
    init();
    _serverUrl = serverUrl;
    _currentMode = mode;

    // ✅ CARREGA CONFIGURAÇÕES SALVAS
    loadConfig();

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

    // ✅ Configurar callbacks para o sistema Push
    OTAPushUpdateManager::setPullUpdateCallback([]() -> bool
                                                { return OTAManager::isUpdateAvailable(); });

    OTAPushUpdateManager::setPerformUpdateCallback([]()
                                                   { OTAManager::performUpdate(); });

    OTAPushUpdateManager::run(); // Inicia thread FreeRTOS

    // Se tem URL de servidor e modo não é MANUAL, inicia o sistema Pull
    if (!serverUrl.isEmpty() && mode != MANUAL)
    {
        OTAPullUpdateManager::init(serverUrl);

        // ✅ VERIFICA SE AUTO UPDATE ESTÁ HABILITADO
        if (mode == AUTOMATIC && _autoUpdateEnabled)
        {
            // Inicia thread de verificação automática com intervalo configurado
            OTAPullUpdateManager::startUpdateThread(_updateIntervalHours * 60); // Converte horas para minutos
        }
    }

    LOG_INFO("✅ OTA Manager inicializado - Modo: %s, AutoUpdate: %s, Intervalo: %d horas",
             mode == MANUAL ? "Manual" : mode == AUTOMATIC ? "Automático"
                                                           : "Híbrido",
             _autoUpdateEnabled ? "Ativado" : "Desativado",
             _updateIntervalHours);
}

// ✅ IMPLEMENTAÇÃO DAS NOVAS FUNÇÕES

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
    DynamicJsonDocument doc(512);
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
    DynamicJsonDocument doc(512);
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

// ✅ ATUALIZAÇÃO DA FUNÇÃO setUpdateMode PARA CONSIDERAR AUTO UPDATE

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

// ✅ ATUALIZAÇÃO DA FUNÇÃO setPullInterval PARA USAR CONFIGURAÇÃO

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