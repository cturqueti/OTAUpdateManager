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

    // ✅ Rota para a página principal
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        LOG_INFO("📁 Página principal solicitada");
        StaticFileHandler::serveFile(request, "/web/index.html", "text/html"); });

    // ✅ Rota para outros arquivos HTML
    server->on("^\\/([a-zA-Z0-9_-]+\\.html)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String filename = request->pathArg(0);
        LOG_INFO("📁 HTML solicitado: %s", filename.c_str());
        StaticFileHandler::serveFile(request, "/web/" + filename, "text/html"); });

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

    // ✅ Rota catch-all para outros arquivos na raiz web
    server->on("^\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String filename = request->pathArg(0);
        LOG_INFO("📁 Arquivo solicitado: %s", filename.c_str());
        
        // Verifica se é um arquivo com extensão conhecida
        if (filename.endsWith(".html") || filename.endsWith(".css") || 
            filename.endsWith(".js") || filename.endsWith(".png") ||
            filename.endsWith(".jpg") || filename.endsWith(".jpeg") ||
            filename.endsWith(".gif") || filename.endsWith(".ico") ||
            filename.endsWith(".json") || filename.endsWith(".txt")) {
            
            String fullPath = "/web/" + filename;
            if (StaticFileHandler::fileExists(fullPath)) {
                StaticFileHandler::serveFile(request, fullPath);
            } else {
                LOG_WARN("⚠️ Arquivo não encontrado: %s", fullPath.c_str());
                request->send(404, "text/plain", "File not found: " + filename);
            }
        } else {
            LOG_WARN("⚠️ Tipo de arquivo não suportado: %s", filename.c_str());
            request->send(404, "text/plain", "File type not supported: " + filename);
        } });

    // ADICIONAR esta rota no seu WebServer:
    server->on("/filesystem-list", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    String path = "/";
    if (request->hasParam("path")) {
        path = request->getParam("path")->value();
    }
    
    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) {
        request->send(404, "text/plain", "Directory not found");
        return;
    }
    
    String json = "[";
    File file = root.openNextFile();
    bool first = true;
    
    while (file) {
        if (!first) json += ",";
        first = false;
        
        json += "{";
        json += "\"name\":\"" + String(file.name()) + "\",";
        json += "\"size\":" + String(file.size()) + ",";
        json += "\"isDirectory\":" + String(file.isDirectory() ? "true" : "false");
        json += "}";
        
        file = root.openNextFile();
    }
    
    json += "]";
    request->send(200, "application/json", json);
    root.close(); });

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

    // Verifica também arquivos HTML adicionais
    checkAdditionalHTMLFiles();
}

void WebAssetManager::checkAdditionalHTMLFiles()
{
    LOG_INFO("🔍 Verificando arquivos HTML adicionais...");

    // Lista de arquivos HTML que você pode ter
    const char *htmlFiles[] = {
        "/web/update.html",
        "/web/system.html",
        "/web/filesystem.html",
        "/web/config.html"};

    for (const char *file : htmlFiles)
    {
        if (StaticFileHandler::fileExists(file))
        {
            size_t size = StaticFileHandler::getFileSize(file);
            LOG_INFO("✅ %s (%d bytes)", file, size);
        }
    }
}