#pragma once

/**
 * @file OTAPushUpdateManager.h
 * @brief Sistema de atualização OTA via upload web local
 *
 * Fornece uma interface web para upload de firmware via browser
 * Complementa o OTAPullUpdateManager
 */

#include "LogLibrary.h"

#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <Update.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class OTAPushUpdateManager
{
public:
    // ============ INTERFACE PÚBLICA ============

    /**
     * @brief Inicializa o servidor web para OTA
     * @param port Porta do servidor web (padrão: 80)
     */
    static void begin(uint16_t port = 80);

    /**
     * @brief Inicia thread FreeRTOS para processamento automático
     * @param stackSize Tamanho da stack da thread (padrão: 8192)
     * @param priority Prioridade da thread (padrão: 1)
     */
    static void run(uint32_t stackSize = 8192, UBaseType_t priority = 1);

    /**
     * @brief Para a thread FreeRTOS
     */
    static void stop();

    /**
     * @brief Configura mDNS para acesso fácil
     * @param hostname Nome para acesso via mDNS (ex: "esp32-ota")
     */
    static void setMDNS(const String &hostname);

    /**
     * @brief Define credenciais de acesso
     * @param username Usuário para autenticação
     * @param password Senha para autenticação
     */
    static void setCredentials(const String &username, const String &password);

    /**
     * @brief Retorna se está atualizando
     */
    static bool isUpdating();

    /**
     * @brief Retorna se o servidor está ativo
     */
    static bool isRunning();

    /**
     * @brief Retorna URL de acesso
     */
    static String getAccessURL();

    /**
     * @brief Atualiza o NTP client (deve ser chamado no loop)
     */
    static void updateTime();

    /**
     * @brief Obtém data/hora atual formatada do ESP32
     */
    static String getCurrentDateTime();

    /**
     * @brief Obtém timestamp atual
     */
    static unsigned long getCurrentTimestamp();

    /**
     * @brief Formata tempo em string legível
     */
    static String formatTime(unsigned long rawTime);

    static void setPullUpdateCallback(bool (*callback)());
    static void setPerformUpdateCallback(void (*callback)());
    static String getPullUpdateStatus();

    static AsyncWebServer *getServer() { return _server; }

private:
    // ============ VARIÁVEIS DE ESTADO ============
    static AsyncWebServer *_server;
    static bool _updating;
    static String _username;
    static String _password;
    static bool _authenticated;
    static bool _running;
    static String _mdnsHostname;

    static bool (*_pullUpdateAvailableCallback)();
    static void (*_performUpdateCallback)();

    // ============ FREERTOS ============
    static TaskHandle_t _webPageTaskHandle;
    static bool _taskRunning;
    static uint32_t _taskStackSize;
    static UBaseType_t _taskPriority;

    // ============ NTP CLIENT ============
    static WiFiUDP *_ntpUDP;
    static NTPClient *_timeClient;
    static bool _timeSynced;

    // ============ MÉTODOS PRIVADOS ============
    // Handlers principais
    static void handleRoot(AsyncWebServerRequest *request);
    static void handleUpdate(AsyncWebServerRequest *request);
    static void handleSystemInfo(AsyncWebServerRequest *request);
    static void handleToggleTheme(AsyncWebServerRequest *request);
    static void handleFilesystem(AsyncWebServerRequest *request);

    // Handlers de upload
    static void handleDoUpload(AsyncWebServerRequest *request, const String &filename,
                               size_t index, uint8_t *data, size_t len, bool final);
    static void handleFilesystemUpload(AsyncWebServerRequest *request, const String &filename,
                                       size_t index, uint8_t *data, size_t len, bool final);

    // Handlers de API
    static void handleCheckUpdates(AsyncWebServerRequest *request);
    static void handlePerformUpdate(AsyncWebServerRequest *request);
    static void handleFilesystemDelete(AsyncWebServerRequest *request);
    static void handleFilesystemMkdir(AsyncWebServerRequest *request);
    static void handleFilesystemDownload(AsyncWebServerRequest *request);

    // Handlers de teste
    static void handleTestCSS(AsyncWebServerRequest *request);
    static void handleTestJS(AsyncWebServerRequest *request);

    // Handlers auxiliares
    static void handleNotFound(AsyncWebServerRequest *request);

    // Autenticação
    static bool checkAuthentication(AsyncWebServerRequest *request);

    // Thread FreeRTOS
    static void taskFunction(void *parameter);
    static void stopTask();

    // Templates e conteúdo
    static String resetReason(esp_reset_reason_t reset);
    static String getSystemInfoContent();
    static String getFullSystemInfoContent();
    static String processTemplate(const String &templateHTML, const String &title, const String &content, AsyncWebServerRequest *request);
    static String formatUptime(unsigned long milliseconds);
    static String formatBuildDate();

    // Version management
    static String extractVersionFromBinary(const uint8_t *data, size_t length);
    static bool isValidVersion(const String &version);

    // Filesystem management
    static String getFilesystemContent(const String &currentPath);
    static String generateBreadcrumb(const String &currentPath);
    static String generateFileList(const String &currentPath);
    static String formatFileSize(size_t bytes);
    static String getFileExtension(const String &filename);

    // Filesystem utilities
    static bool isLittleFSMounted();
    static String getDirectoryPath(const String &fullPath);
    static bool createDirectories(const String &path);
    static String getUploadPathFromRequest(AsyncWebServerRequest *request);
    static bool deleteDirectoryRecursive(const String &path);

    // Static file serving
    static void serveStaticFile(AsyncWebServerRequest *request, const String &path, const String &defaultContentType = "application/octet-stream");
    static String getContentType(const String &filename, const String &defaultType);
    static bool initLittleFS();
};