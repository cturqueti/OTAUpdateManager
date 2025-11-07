#pragma once

#include "LogLibrary.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

class StaticFileHandler
{
public:
    static void serveFile(AsyncWebServerRequest *server, const String &filePath, const String &defaultContentType = "application/octet-stream");
    static String getContentType(const String &filename, const String &defaultType);
    static bool fileExists(const String &path);
    static size_t getFileSize(const String &path);

private:
    static bool ensureFSMounted();
};