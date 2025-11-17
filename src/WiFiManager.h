#pragma once

#include "LogLibrary.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <WiFiMulti.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

class WiFiManager
{
public:
    static void begin();
    static void end();

    // Gerenciamento de redes
    static void addNetwork(const String &ssid, const String &password);
    static void clearNetworks();
    static void setAutoReconnect(bool enable);
    static void setConnectionTimeout(uint32_t timeoutMs);

    // Status e informações
    static bool isConnected();
    static String getSSID();
    static String getIP();
    static int getRSSI();
    static String getStatusString();

    // Controle manual
    static bool connect();
    static void disconnect();
    static void reconnect();

    // Configuração
    static void setHostname(const String &hostname);
    static void setScanMethod(wifi_scan_method_t method);
    static void setSortMethod(wifi_sort_method_t method);

private:
    static WiFiMulti _wiFiMulti;
    static TaskHandle_t _wifiMonitorTaskHandle;
    static EventGroupHandle_t _wifiEventGroup;
    static bool _autoReconnect;
    static uint32_t _connectionTimeout;
    static String _hostname;

    static void wifiMonitorTask(void *parameter);
    static bool connectInternal();
};