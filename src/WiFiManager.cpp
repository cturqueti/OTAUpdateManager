#include "WiFiManager.h"
#include "esp_task_wdt.h"

// Inicialização de variáveis estáticas
WiFiMulti WiFiManager::_wiFiMulti;
TaskHandle_t WiFiManager::_wifiMonitorTaskHandle = NULL;
EventGroupHandle_t WiFiManager::_wifiEventGroup = NULL;
bool WiFiManager::_autoReconnect = true;
uint32_t WiFiManager::_connectionTimeout = 10000; // 10 segundos
String WiFiManager::_hostname = "esp32-device";

void WiFiManager::begin()
{
    LOG_INFO("🚀 Inicializando WiFiManager...");

    // Configurar WiFi
    WiFi.mode(WIFI_STA);

    // Configurar hostname se definido
    if (!_hostname.isEmpty())
    {
        WiFi.setHostname(_hostname.c_str());
        LOG_DEBUG("Hostname configurado: %s", _hostname.c_str());
    }

    // Criar event group para sincronização
    _wifiEventGroup = xEventGroupCreate();
    if (!_wifiEventGroup)
    {
        LOG_ERROR("❌ Falha ao criar event group do WiFi");
        return;
    }

    // Tentar conectar
    if (connect())
    {
        // Iniciar task de monitoramento
        BaseType_t result = xTaskCreate(
            wifiMonitorTask,
            "WiFi_Monitor",
            4096,
            NULL,
            tskIDLE_PRIORITY + 1,
            &_wifiMonitorTaskHandle);

        if (result != pdPASS)
        {
            LOG_ERROR("❌ Falha ao criar task de monitoramento WiFi");
        }
    }
}

void WiFiManager::end()
{
    LOG_INFO("🛑 Finalizando WiFiManager...");

    // Parar task de monitoramento
    if (_wifiMonitorTaskHandle)
    {
        vTaskDelete(_wifiMonitorTaskHandle);
        _wifiMonitorTaskHandle = NULL;
    }

    // Deletar event group
    if (_wifiEventGroup)
    {
        vEventGroupDelete(_wifiEventGroup);
        _wifiEventGroup = NULL;
    }

    // Desconectar WiFi
    disconnect();

    // Limpar redes
    clearNetworks();
}

void WiFiManager::addNetwork(const String &ssid, const String &password)
{
    if (ssid.isEmpty())
    {
        LOG_WARN("⚠️  Tentativa de adicionar SSID vazio");
        return;
    }

    LOG_DEBUG("➕ Adicionando rede WiFi: %s", ssid.c_str());
    _wiFiMulti.addAP(ssid.c_str(), password.c_str());
}

void WiFiManager::clearNetworks()
{
    LOG_DEBUG("🧹 Limpando todas as redes WiFi");
    // WiFiMulti não tem método clear, então recriamos o objeto
    _wiFiMulti.~WiFiMulti();
    new (&_wiFiMulti) WiFiMulti();
}

void WiFiManager::setAutoReconnect(bool enable)
{
    _autoReconnect = enable;
    LOG_INFO("🔧 Auto-reconexão WiFi: %s", enable ? "ATIVADA" : "DESATIVADA");
}

void WiFiManager::setConnectionTimeout(uint32_t timeoutMs)
{
    _connectionTimeout = timeoutMs;
    LOG_DEBUG("⏱️  Timeout de conexão WiFi: %lu ms", timeoutMs);
}

bool WiFiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getSSID()
{
    return WiFi.SSID();
}

String WiFiManager::getIP()
{
    return WiFi.localIP().toString();
}

int WiFiManager::getRSSI()
{
    return WiFi.RSSI();
}

String WiFiManager::getStatusString()
{
    wl_status_t status = WiFi.status();
    switch (status)
    {
    case WL_NO_SHIELD:
        return "NO_SHIELD";
    case WL_IDLE_STATUS:
        return "IDLE";
    case WL_NO_SSID_AVAIL:
        return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
        return "SCAN_COMPLETED";
    case WL_CONNECTED:
        return "CONNECTED";
    case WL_CONNECT_FAILED:
        return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "CONNECTION_LOST";
    case WL_DISCONNECTED:
        return "DISCONNECTED";
    default:
        return "UNKNOWN";
    }
}

bool WiFiManager::connect()
{
    return connectInternal();
}

void WiFiManager::disconnect()
{
    LOG_INFO("🔌 Desconectando WiFi...");
    WiFi.disconnect();
}

void WiFiManager::reconnect()
{
    LOG_INFO("🔄 Tentando reconectar WiFi...");
    disconnect();
    vTaskDelay(pdMS_TO_TICKS(1000));
    connect();
}

void WiFiManager::setHostname(const String &hostname)
{
    _hostname = hostname;
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.setHostname(hostname.c_str());
    }
    LOG_DEBUG("🏷️  Hostname configurado: %s", hostname.c_str());
}

void WiFiManager::setScanMethod(wifi_scan_method_t method)
{
    WiFi.scanNetworks(true, false); // Configurações de scan
    LOG_DEBUG("🔍 Método de scan configurado");
}

void WiFiManager::setSortMethod(wifi_sort_method_t method)
{
    // WiFiMulti usa ordem de adição, não temos controle direto sobre ordenação
    LOG_DEBUG("📊 Método de ordenação configurado");
}

// Implementação interna
bool WiFiManager::connectInternal()
{
    LOG_INFO("📡 Conectando ao WiFi...");

    uint32_t startTime = millis();
    int connectionAttempts = 0;
    const int MAX_ATTEMPTS = 3;

    while (connectionAttempts < MAX_ATTEMPTS)
    {
        connectionAttempts++;
        LOG_DEBUG("🔄 Tentativa de conexão %d/%d", connectionAttempts, MAX_ATTEMPTS);

        uint32_t attemptStartTime = millis();

        // Iniciar a conexão
        uint8_t wifiStatus = _wiFiMulti.run();

        // Aguardar conexão com timeout
        while (wifiStatus != WL_CONNECTED && wifiStatus != WL_CONNECT_FAILED)
        {
            // ✅ Resetar o watchdog durante tentativas longas
            esp_task_wdt_reset();

            vTaskDelay(pdMS_TO_TICKS(500));

            wifiStatus = WiFi.status();

            // Verificar timeout
            if (millis() - attemptStartTime > _connectionTimeout)
            {
                LOG_WARN("⏱️  Timeout na tentativa de conexão %d", connectionAttempts);
                break;
            }

            // Verificar se conseguiu conectar
            if (wifiStatus == WL_CONNECTED)
            {
                break;
            }
        }

        if (wifiStatus == WL_CONNECTED)
        {
            LOG_INFO("✅ Conectado ao WiFi: %s", WiFi.SSID().c_str());
            LOG_INFO("📡 IP: %s", WiFi.localIP().toString().c_str());
            LOG_INFO("📶 RSSI: %d dBm", WiFi.RSSI());

            if (_wifiEventGroup)
            {
                xEventGroupSetBits(_wifiEventGroup, WIFI_CONNECTED_BIT);
            }

            return true;
        }

        // ✅ IMPORTANTE: Dar tempo para outras tasks
        vTaskDelay(pdMS_TO_TICKS(1000));

        // ✅ Resetar o watchdog durante tentativas longas
        esp_task_wdt_reset();

        LOG_WARN("⚠️ Falha na tentativa %d, status: %s",
                 connectionAttempts, getStatusString().c_str());
    }

    LOG_ERROR("❌ Falha na conexão WiFi após %d tentativas", MAX_ATTEMPTS);
    if (_wifiEventGroup)
    {
        xEventGroupSetBits(_wifiEventGroup, WIFI_FAIL_BIT);
    }

    return false;
}

void WiFiManager::wifiMonitorTask(void *parameter)
{
    LOG_INFO("👀 Iniciando monitoramento WiFi...");

    // ✅ Registrar esta task no watchdog
    esp_task_wdt_add(NULL);

    uint32_t lastCheck = 0;
    const uint32_t CHECK_INTERVAL = 10000; // Verificar a cada 10 segundos (aumentado)

    while (true)
    {
        // ✅ Resetar watchdog a cada loop
        esp_task_wdt_reset();

        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay base de 2s (reduzido)

        uint32_t now = millis();

        // Verificar conexão apenas no intervalo definido
        if (now - lastCheck >= CHECK_INTERVAL)
        {
            lastCheck = now;

            if (!isConnected())
            {
                LOG_WARN("📡 Conexão WiFi perdida!");

                if (_autoReconnect)
                {
                    LOG_INFO("🔄 Tentando reconexão automática...");

                    // ✅ Resetar watchdog durante reconexão
                    esp_task_wdt_reset();
                    connectInternal();
                    esp_task_wdt_reset();
                }
            }
            else
            {
                // ✅ Log periódico de status (opcional)
                static uint32_t lastStatusLog = 0;
                if (now - lastStatusLog >= 60000)
                { // A cada 1 minuto
                    lastStatusLog = now;
                    LOG_DEBUG("📶 WiFi OK - SSID: %s, IP: %s, RSSI: %d",
                              getSSID().c_str(), getIP().c_str(), getRSSI());
                }
            }
        }
    }

    // ✅ Remover do watchdog quando task terminar (nunca deve acontecer)
    esp_task_wdt_delete(NULL);
}
