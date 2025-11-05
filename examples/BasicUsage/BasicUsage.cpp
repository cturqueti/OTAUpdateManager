/**
 * @file BasicUsage.ino
 * @brief Exemplo básico de uso da biblioteca OTAUpdateManager
 */

#include <OTAManager.h>

void setup()
{
    Serial.begin(115200);

    // Inicializa WiFi primeiro
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Conectando WiFi...");
    }

    // Inicializa OTA Manager
    OTAManager::begin(
        "http://meuserver.com", // URL do servidor (opcional)
        80,                     // Porta web
        OTAManager::HYBRID      // Modo: HYBRID, AUTOMATIC ou MANUAL
    );

    // Configurações opcionais
    OTAManager::setWebCredentials("admin", "password");
    OTAManager::setMDNS("esp32-device");
    OTAManager::setPullInterval(10); // Verificar a cada 10 minutos

    Serial.println("✅ Sistema OTA inicializado!");
    Serial.println(OTAManager::getUpdateStatus());
}

void loop()
{
    OTAManager::handleClient(); // Necessário se não usar thread FreeRTOS
    delay(100);
}