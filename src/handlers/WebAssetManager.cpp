#include "WebAssetManager.h"
#include "../OTAManager.h"
#include "../OTAPushUpdateManager.h"
#include "InternalFunctions.h"
#include <ArduinoJson.h>

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

    server->on("/filesystem-list", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        LOG_INFO("📁 Listagem de arquivos solicitada para LittleFS");
        
        String path = "/";
        if (request->hasParam("path")) {
            path = request->getParam("path")->value();
            LOG_INFO("🔍 Path solicitado: %s", path.c_str());
        }
        
        // Garantir que o path termina com /
        if (!path.endsWith("/")) {
            path += "/";
        }
        
        File root = LittleFS.open(path);
        if (!root || !root.isDirectory()) {
            LOG_WARN("⚠️ Diretório não encontrado: %s", path.c_str());
            request->send(404, "text/plain", "Directory not found: " + path);
            return;
        }
        
        String json = "[";
        File file = root.openNextFile();
        bool first = true;
        
        while (file) {
            if (!first) json += ",";
            first = false;
            
            // Extrair apenas o nome do arquivo (sem o path completo)
            String fileName = String(file.name());
            if (fileName.startsWith(path)) {
                fileName = fileName.substring(path.length());
            }
            
            json += "{";
            json += "\"name\":\"" + fileName + "\",";
            json += "\"size\":" + String(file.size()) + ",";
            json += "\"isDirectory\":" + String(file.isDirectory() ? "true" : "false");
            json += "}";
            
            LOG_INFO("📄 Arquivo listado: %s (size: %d, dir: %s)", 
                     fileName.c_str(), file.size(), file.isDirectory() ? "true" : "false");
            
            file = root.openNextFile();
        }
        
        json += "]";
        root.close();
        
        LOG_INFO("📊 Listagem concluída: %d arquivos", first ? 0 : json.length());
        request->send(200, "application/json", json); });

    // ✅ Rota para verificar status do LittleFS
    server->on("/filesystem-status", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("🔍 Verificando status do LittleFS");
    
    String status = "LittleFS Status:\n";
    
    // Verificar montagem
    if (LittleFS.begin(false)) {
        status += "✅ Montado: Sim\n";
        
        // Verificar permissões de escrita
        String testFile = "/write_test.tmp";
        File file = LittleFS.open(testFile, "w");
        if (file) {
            file.print("test");
            file.close();
            LittleFS.remove(testFile);
            status += "✅ Permissões: Leitura/Escrita\n";
        } else {
            status += "❌ Permissões: Somente Leitura\n";
        }
        
        // Verificar espaço livre
        status += "💾 Espaço Total: " + String(LittleFS.totalBytes()) + " bytes\n";
        status += "💾 Espaço Usado: " + String(LittleFS.usedBytes()) + " bytes\n";
        
    } else {
        status += "❌ Montado: Não\n";
    }
    
    request->send(200, "text/plain", status); });

    // ✅ Rota para criar diretório no LittleFS (DEBUG)
    server->on("/filesystem-mkdir", HTTP_POST, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("📁 [WEBASSETMANAGER] Criando diretório no LittleFS");
    
    if (request->hasParam("path", true)) {
        String path = request->getParam("path", true)->value();
        LOG_INFO("🔍 [WEBASSETMANAGER] Path para criar: %s", path.c_str());
        
        // DEBUG: Listar diretório pai
        String parentDir = path.substring(0, path.lastIndexOf('/'));
        if (parentDir == "") parentDir = "/";
        LOG_INFO("🔍 [WEBASSETMANAGER] Diretório pai: %s", parentDir.c_str());
        
        File test = LittleFS.open(parentDir);
        if (test) {
            LOG_INFO("✅ [WEBASSETMANAGER] Diretório pai existe e é acessível");
            test.close();
        } else {
            LOG_ERROR("❌ [WEBASSETMANAGER] Diretório pai NÃO é acessível");
        }
        
        // Criar o diretório
        if (LittleFS.mkdir(path)) {
            LOG_INFO("✅ [WEBASSETMANAGER] Diretório criado com sucesso: %s", path.c_str());
            request->send(200, "text/plain", "Directory created: " + path);
        } else {
            LOG_ERROR("❌ [WEBASSETMANAGER] Falha ao criar diretório: %s", path.c_str());
            request->send(500, "text/plain", "Failed to create directory: " + path);
        }
    } else {
        LOG_ERROR("❌ [WEBASSETMANAGER] Parâmetro 'path' não encontrado");
        request->send(400, "text/plain", "Missing 'path' parameter");
    } });

    server->on("/force-remount", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    forceRemountLittleFS();
    request->send(200, "text/plain", "LittleFS remounted"); });
    // ✅ Rota para deletar arquivos/diretórios
    server->on("/filesystem-delete", HTTP_POST, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("🗑️ Deletando arquivo/diretório no LittleFS");
    
    if (request->hasParam("path", true)) {
        String path = request->getParam("path", true)->value();
        LOG_INFO("🔍 Path para deletar: %s", path.c_str());
        
        if (LittleFS.exists(path)) {
            if (LittleFS.remove(path)) {
                LOG_INFO("✅ Arquivo deletado: %s", path.c_str());
                request->send(200, "text/plain", "File deleted: " + path);
            } else {
                // Tenta deletar como diretório
                if (deleteRecursive(path)) {
                    LOG_INFO("✅ Diretório deletado: %s", path.c_str());
                    request->send(200, "text/plain", "Directory deleted: " + path);
                } else {
                    LOG_ERROR("❌ Falha ao deletar: %s", path.c_str());
                    request->send(500, "text/plain", "Failed to delete: " + path);
                }
            }
        } else {
            LOG_WARN("⚠️ Arquivo não encontrado: %s", path.c_str());
            request->send(404, "text/plain", "File not found: " + path);
        }
    } else {
        LOG_ERROR("❌ Parâmetro 'path' não encontrado");
        request->send(400, "text/plain", "Missing 'path' parameter");
    } });

    // ✅ Rota para download de arquivos
    server->on("/filesystem-download", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("⬇️ Download de arquivo solicitado");
    
    if (request->hasParam("path")) {
        String path = request->getParam("path")->value();
        LOG_INFO("🔍 Path para download: %s", path.c_str());
        
        if (LittleFS.exists(path)) {
            StaticFileHandler::serveFile(request, path);
        } else {
            LOG_WARN("⚠️ Arquivo não encontrado: %s", path.c_str());
            request->send(404, "text/plain", "File not found: " + path);
        }
    } else {
        LOG_ERROR("❌ Parâmetro 'path' não encontrado");
        request->send(400, "text/plain", "Missing 'path' parameter");
    } });

    // ✅ Rota para upload de arquivos
    server->on("/filesystem-upload", HTTP_POST, [](AsyncWebServerRequest *request)
               {
                   // Resposta será enviada no handler de upload
               },
               [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
               { handleFilesystemUpload(request, filename, index, data, len, final); });

    // ✅ Rota para informações do sistema em JSON (usando ArduinoJson)
    server->on("/api/system-info", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("📊 API System Info solicitada");
    
    // Cria o documento JSON
    JsonDocument doc;
    
    // Adiciona dados do sistema
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["ip"] = WiFi.localIP().toString();
    wifi["ssid"] = WiFi.SSID();
    wifi["rssi"] = WiFi.RSSI();
    wifi["mac"] = WiFi.macAddress();
    wifi["hostname"] = WiFi.getHostname();
    wifi["gateway"] = WiFi.gatewayIP().toString();
    wifi["subnet"] = WiFi.subnetMask().toString();
    wifi["dns"] = WiFi.dnsIP().toString();
    wifi["mDNS"] = OTAPushUpdateManager::getMDNSHostname()+".local";

    JsonObject hardware = doc["hardware"].to<JsonObject>();
        hardware["chipModel"] = ESP.getChipModel();
    hardware["chipCores"] = ESP.getChipCores();
    hardware["chipRevision"] = ESP.getChipRevision();
    hardware["cpuFreq"] = ESP.getCpuFreqMHz();
    hardware["heapFree"] = ESP.getFreeHeap();
    hardware["heapTotal"] = ESP.getHeapSize();
    hardware["heapMin"] = ESP.getMinFreeHeap();
    hardware["psramSize"] = ESP.getPsramSize();
    hardware["flashSize"] = ESP.getFlashChipSize();
    hardware["flashSpeed"] = ESP.getFlashChipSpeed();
    hardware["sdkVersion"] = ESP.getSdkVersion();

    JsonObject system = doc["system"].to<JsonObject>();
        system["firmwareVersion"] = String(OTAManager::getFirmwareVersion().c_str());
    system["uptime"] = millis();
    system["currentTime"] = InternalFunctions::getCurrentDateTime();
    system["resetReason"] = "fazer isso";
    system["resetInfo"] = "fazer isso";

        JsonObject filesystem = doc["filesystem"].to<JsonObject>();
    filesystem["totalBytes"] = LittleFS.totalBytes();
    filesystem["usedBytes"] = LittleFS.usedBytes();
    filesystem["freeBytes"] = LittleFS.totalBytes() - LittleFS.usedBytes();
    
    // Serializa o JSON para string
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    LOG_INFO("📤 Enviando system info: %s", jsonResponse.c_str());
    request->send(200, "application/json", jsonResponse); });

    server->on("/api/uptime", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    unsigned long milliseconds = millis();
    unsigned long seconds = milliseconds / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    String uptime = String(days) + "d " + 
                   String(hours % 24) + "h " + 
                   String(minutes % 60) + "m " + 
                   String(seconds % 60) + "s";
    
    request->send(200, "text/plain", uptime); });

    server->on("/check-updates", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("🔍 Verificação de atualizações solicitada");
    
    JsonDocument doc;
    doc["status"] = "up_to_date";
    doc["message"] = "Sistema atualizado";
    doc["current_version"] = OTAManager::getFirmwareVersion().c_str();
    doc["latest_version"] = OTAManager::getFirmwareVersion().c_str();
    doc["timestamp"] = InternalFunctions::getCurrentDateTime();
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    LOG_INFO("📤 Resposta OTA Check: %s", jsonResponse.c_str());
    request->send(200, "application/json", jsonResponse); });

    server->on("/check-updates-debug", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("🔍 DEBUG: Verificação de atualizações");
    
    // Retorna um JSON simples para teste
    String debugResponse = "{\"status\":\"up_to_date\",\"message\":\"Sistema atualizado\",\"current_version\":\"2.1.10\",\"latest_version\":\"2.1.10\"}";
    
    request->send(200, "application/json", debugResponse); });

    // -->>✅ Colocar por ultimo<<--
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

    LOG_INFO("✅ Rotas de assets configuradas - Padrões com regex ativos");
}

void WebAssetManager::forceRemountLittleFS()
{
    LOG_INFO("🔄 Forçando remontagem do LittleFS...");
    LittleFS.end();
    delay(100);

    if (LittleFS.begin(true))
    {
        LOG_INFO("✅ LittleFS remontado com sucesso");
    }
    else
    {
        LOG_ERROR("❌ Falha na remontagem do LittleFS");
    }
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

// ✅ Função para upload de arquivos
void WebAssetManager::handleFilesystemUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    static File uploadFile;
    static String uploadPath;

    if (index == 0)
    {
        LOG_INFO("📤 Iniciando upload de arquivo: %s", filename.c_str());

        // Obtém o path do parâmetro da URL
        if (request->hasParam("path"))
        {
            uploadPath = request->getParam("path")->value();
        }
        else
        {
            uploadPath = "/";
        }

        String fullPath = uploadPath + "/" + filename;
        LOG_INFO("📁 Salvando em: %s", fullPath.c_str());

        uploadFile = LittleFS.open(fullPath, "w");
        if (!uploadFile)
        {
            LOG_ERROR("❌ Falha ao criar arquivo: %s", fullPath.c_str());
            request->send(500, "text/plain", "Failed to create file");
            return;
        }
    }

    if (uploadFile)
    {
        if (uploadFile.write(data, len) != len)
        {
            LOG_ERROR("❌ Erro na escrita do arquivo");
        }
    }

    if (final)
    {
        if (uploadFile)
        {
            uploadFile.close();
            LOG_INFO("✅ Upload finalizado: %s (%u bytes)", filename.c_str(), index + len);
            request->send(200, "text/plain", "Upload successful: " + filename);
        }
        else
        {
            request->send(500, "text/plain", "Upload failed");
        }
    }
}

// ✅ Função auxiliar para deletar recursivamente
bool WebAssetManager::deleteRecursive(String path)
{
    File file = LittleFS.open(path);
    if (!file)
    {
        return false;
    }

    if (!file.isDirectory())
    {
        file.close();
        return LittleFS.remove(path);
    }

    file.rewindDirectory();
    while (File entry = file.openNextFile())
    {
        String entryPath = path + "/" + entry.name();
        if (entry.isDirectory())
        {
            if (!deleteRecursive(entryPath))
            {
                file.close();
                return false;
            }
        }
        else
        {
            if (!LittleFS.remove(entryPath))
            {
                file.close();
                return false;
            }
        }
        entry.close();
    }
    file.close();

    return LittleFS.rmdir(path);
}