#include "WebAssetManager.h"
#include "../OTAPushUpdateManager.h"

void WebAssetManager::setupRoutes(AsyncWebServer *server)
{
    if (!server)
    {
        LOG_ERROR("❌ AsyncWebServer é nulo no setupRoutes");
        return;
    }

    LOG_INFO("🔄 WebAssetManager::setupRoutes() - INICIANDO");
    LOG_INFO("🔍 Ponteiro do server: %p", server);

    // ✅ Rotas para assets usando ESPAsyncWebServer
    server->on("^\\/assets\\/css\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String filename = request->pathArg(0);
        LOG_INFO("📁 CSS genérico solicitado: %s", filename.c_str());
        StaticFileHandler::serveFile(request, "/web/css/" + filename, "text/css"); });

    server->on("^\\/assets\\/js\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String filename = request->pathArg(0);
        LOG_INFO("📁 JS genérico solicitado: %s", filename.c_str());
        StaticFileHandler::serveFile(request, "/web/js/" + filename, "application/javascript"); });

    server->on("^\\/assets\\/icons\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String filename = request->pathArg(0);
        LOG_INFO("📁 Ícone solicitado: %s", filename.c_str());
        String fullPath = "/web/icons/" + filename;
        StaticFileHandler::serveFile(request, fullPath); });

    server->on("^\\/assets\\/images\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String filename = request->pathArg(0);
        LOG_INFO("📁 Imagem solicitada: %s", filename.c_str());
        String fullPath = "/web/images/" + filename;
        StaticFileHandler::serveFile(request, fullPath); });

    // ✅ Rota catch-all para outros assets
    server->on("^\\/assets\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String assetPath = request->pathArg(0);
        LOG_INFO("📁 Asset genérico solicitado: %s", assetPath.c_str());
        
        String fullPath = "/web/" + assetPath;
        if (StaticFileHandler::fileExists(fullPath)) {
            StaticFileHandler::serveFile(request, fullPath);
        } else {
            LOG_WARN("⚠️ Asset não encontrado: %s", fullPath.c_str());
            request->send(404, "text/plain", "Asset not found: " + assetPath);
        } });

    LOG_INFO("✅ Rotas de assets configuradas - Padrões com regex ativos");
}

void WebAssetManager::setupRoutes_bkp(AsyncWebServer *server)
{
    // Backup mantido para referência
    if (!server)
    {
        LOG_ERROR("❌ WebServer é nulo no setupRoutes_bkp");
        return;
    }
    LOG_INFO("🔄 WebAssetManager::setupRoutes_bkp() - INICIANDO");

    // ... código do backup mantido
}

void WebAssetManager::checkRequiredAssets()
{
    const char *requiredFiles[] = {
        "/web/icons/favicon.png",
        "/web/index.html",
        "/web/css/styles.css",
        "/web/js/scripts.js"};

    LOG_INFO("🔍 Verificando assets web...");
    for (const char *file : requiredFiles)
    {
        if (StaticFileHandler::fileExists(file))
        {
            size_t size = StaticFileHandler::getFileSize(file);
            LOG_INFO("✅ %s (%d bytes)", file, size);
        }
        else
        {
            LOG_WARN("⚠️  Arquivo não encontrado: %s", file);
        }
    }
}