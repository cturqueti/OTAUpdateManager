#include "StaticFileHandler.h"

bool StaticFileHandler::ensureFSMounted()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("❌ ERRO CRÍTICO: Falha ao montar LittleFS");
        return false;
    }
    return true;
}

void StaticFileHandler::serveFile(AsyncWebServerRequest *request, const String &filePath, const String &defaultContentType)
{
    if (!ensureFSMounted())
    {
        request->send(500, "text/plain", "File system error");
        return;
    }
    LOG_INFO("📁 Servindo arquivo: %s", filePath.c_str());

    if (!LittleFS.exists(filePath))
    {
        LOG_WARN("⚠️  Arquivo não encontrado: %s", filePath.c_str());
        request->send(404, "text/plain", "File not found: " + filePath);
        return;
    }

    File file = LittleFS.open(filePath, "r");
    if (!file)
    {
        LOG_ERROR("❌ Erro ao abrir arquivo: %s", filePath.c_str());
        request->send(500, "text/plain", "Error opening file");
        return;
    }

    // Detecta o tipo MIME baseado na extensão
    String contentType = getContentType(filePath, defaultContentType);

    // Envia o arquivo de forma assíncrona
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, filePath, contentType);
    request->send(response);

    file.close();
    LOG_INFO("✅ Arquivo enviado: %s (%d bytes)", filePath.c_str(), file.size());
}

String StaticFileHandler::getContentType(const String &filename, const String &defaultType)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".htm"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    if (filename.endsWith(".png"))
        return "image/png";
    if (filename.endsWith(".jpg") || filename.endsWith(".jpeg"))
        return "image/jpeg";
    if (filename.endsWith(".gif"))
        return "image/gif";
    if (filename.endsWith(".ico"))
        return "image/x-icon";
    if (filename.endsWith(".json"))
        return "application/json";
    if (filename.endsWith(".txt"))
        return "text/plain";
    if (filename.endsWith(".xml"))
        return "text/xml";
    if (filename.endsWith(".pdf"))
        return "application/pdf";
    if (filename.endsWith(".zip"))
        return "application/zip";
    return defaultType;
}

bool StaticFileHandler::fileExists(const String &path)
{
    return LittleFS.exists(path);
}

size_t StaticFileHandler::getFileSize(const String &path)
{
    if (!fileExists(path))
        return 0;
    File file = LittleFS.open(path, "r");
    size_t size = file.size();
    file.close();
    return size;
}