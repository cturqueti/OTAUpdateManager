#pragma once

#include "StaticFileHandler.h"
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

class WebAssetManager
{
public:
    static void setupRoutes(AsyncWebServer *server);
    static void setupRoutes_bkp(AsyncWebServer *server);
    static void checkRequiredAssets();

private:
    static void checkAdditionalHTMLFiles();
    static void forceRemountLittleFS();
    static void handleFilesystemUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
    static bool deleteRecursive(String path);
    static String getEncryptionType(wifi_auth_mode_t encryptionType);
};