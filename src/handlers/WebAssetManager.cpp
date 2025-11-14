#include "WebAssetManager.h"
#include "../OTAManager.h"
#include "../OTAPushUpdateManager.h"
#include "InternalFunctions.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

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

            JsonDocument doc;
            doc["success"] = false;
            doc["error"] = "Directory not found: " + path;
            doc["path"] = path;

            String jsonResponse;
            serializeJson(doc, jsonResponse);
            request->send(404, "application/json", jsonResponse);
            return;
        }
        
        JsonDocument doc;
        doc["success"] = true;
        doc["path"] = path;

        JsonArray files = doc["files"].to<JsonArray>();
        
        File file = root.openNextFile();
        int fileCount = 0;
        
        while (file) {
            JsonObject fileObj = files.add<JsonObject>();
            
            // Extrair apenas o nome do arquivo (sem o path completo)
            String fileName = String(file.name());
            if (fileName.startsWith(path)) {
                fileName = fileName.substring(path.length());
            }
            
            fileObj["name"] = fileName;
            fileObj["size"] = file.size();
            fileObj["isDirectory"] = file.isDirectory();
            fileObj["path"] = path + fileName;
            
            // Adicionar informações extras se disponíveis
            if (!file.isDirectory()) {
                fileObj["extension"] = getFileExtension(fileName);
            }
            
            LOG_INFO("📄 Arquivo listado: %s (size: %d, dir: %s)", 
                    fileName.c_str(), file.size(), file.isDirectory() ? "true" : "false");
            
            file = root.openNextFile();
            fileCount++;
        }
        
    root.close();
    
    doc["count"] = fileCount;
    doc["totalSize"] = calculateDirectorySize(path); // Função opcional
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    LOG_INFO("📊 Listagem concluída: %d arquivos", fileCount);
    request->send(200, "application/json", jsonResponse); });

    // API - HTTP GET
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

    server->on("/api/debug", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        LOG_INFO("🎯 Rota /api/debug acessada");
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"API working\"}"); });

    server->on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("📊 API Config - Obtendo configurações");
    
    JsonDocument doc;
    doc["autoUpdate"] = OTAManager::isAutoUpdateEnabled();
    doc["serverUrl"] = OTAManager::getServerUrl();
    doc["updateInterval"] = OTAManager::getUpdateInterval();
    doc["lastUpdateCheck"] = OTAManager::getLastUpdateCheck();
    doc["currentMode"] = (int)OTAManager::getCurrentMode();
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    LOG_INFO("📤 Enviando configurações: %s", jsonResponse.c_str());
    request->send(200, "application/json", jsonResponse); });

    server->on("/api/network/status", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("📡 Obtendo status da rede");
    
    JsonDocument doc;
    
    if (WiFi.status() == WL_CONNECTED) {
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = WiFi.localIP().toString();
        doc["status"] = "Conectado";
        doc["rssi"] = WiFi.RSSI();
        doc["mac"] = WiFi.macAddress();
        doc["gateway"] = WiFi.gatewayIP().toString();
        doc["subnet"] = WiFi.subnetMask().toString();
        doc["power"] = WiFi.getTxPower();

    } else {
        doc["ssid"] = "";
        doc["ip"] = "";
        doc["status"] = "Desconectado";
        doc["rssi"] = 0;
        doc["mac"] = WiFi.macAddress();
    }
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse); });

    server->on("/api/network/scan", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("🔍 Escaneando redes WiFi");
    
    int numNetworks = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();
    
    if (numNetworks == 0) {
        LOG_INFO("📶 Nenhuma rede encontrada");
        doc["message"] = "Nenhuma rede WiFi encontrada";
    } else {
        LOG_INFO("📶 Encontradas %d redes", numNetworks);
        
        for (int i = 0; i < numNetworks; i++) {
            JsonObject network = networks.add<JsonObject>();
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["encryption"] = getEncryptionType(WiFi.encryptionType(i));
            network["channel"] = WiFi.channel(i);
            network["bssid"] = WiFi.BSSIDstr(i);
            
            LOG_DEBUG("📶 Rede: %s, RSSI: %d, Encryption: %s", 
                     WiFi.SSID(i).c_str(), WiFi.RSSI(i), 
                     getEncryptionType(WiFi.encryptionType(i)).c_str());
        }
    }
    
    WiFi.scanDelete(); // Limpa a lista de scan
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse); });

    server->on("/api/filesystem-info", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    if (!request->hasParam("path")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Path parameter required\"}");
        return;
    }
    
    String path = request->getParam("path")->value();
    LOG_INFO("📄 Obtendo informações do arquivo: %s", path.c_str());
    
    JsonDocument doc;
    
    // Verifica se o arquivo/pasta existe
    if (!LittleFS.exists(path)) {
        doc["success"] = false;
        doc["error"] = "File or directory not found: " + path;
        doc["path"] = path;
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(404, "application/json", jsonResponse);
        return;
    }
    
    File file = LittleFS.open(path, "r");
    if (!file) {
        doc["success"] = false;
        doc["error"] = "Cannot open: " + path;
        doc["path"] = path;
        
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(500, "application/json", jsonResponse);
        return;
    }
    
    // Informações básicas
    doc["success"] = true;
    doc["path"] = path;
    doc["name"] = path.substring(path.lastIndexOf('/') + 1);
    doc["isDirectory"] = file.isDirectory();
    doc["size"] = file.size();

// Informações de tempo (se disponíveis)
#ifdef ESP32
    struct tm *timeinfo;
    time_t lastWrite = file.getLastWrite();
    if (lastWrite > 0) {
        timeinfo = localtime(&lastWrite);
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
        doc["modified"] = String(timeStr);
        doc["lastModified"] = lastWrite;
    }
#endif
    
    // Para arquivos, obter extensão
    if (!file.isDirectory()) {
        String fileName = doc["name"].as<String>();
        int dotIndex = fileName.lastIndexOf('.');
        if (dotIndex > 0 && dotIndex < fileName.length() - 1) {
            doc["extension"] = fileName.substring(dotIndex + 1);
        } else {
            doc["extension"] = "";
        }
        
        // Tipo MIME básico
        doc["mimeType"] = getMimeType(doc["extension"].as<String>());
    }
    
    // Para diretórios, contar arquivos
    if (file.isDirectory()) {
        int fileCount = 0;
        int dirCount = 0;
        size_t totalSize = 0;
        
        File entry = file.openNextFile();
        while (entry) {
            if (entry.isDirectory()) {
                dirCount++;
            } else {
                fileCount++;
                totalSize += entry.size();
            }
            entry = file.openNextFile();
        }
        
        doc["fileCount"] = fileCount;
        doc["dirCount"] = dirCount;
        doc["totalSize"] = totalSize;
        doc["totalItems"] = fileCount + dirCount;
    }
    
    file.close();
    
    doc["fsInfo"] = getFileSystemInfo();
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    LOG_DEBUG("📊 Informações do arquivo: %s", jsonResponse.c_str());
    request->send(200, "application/json", jsonResponse); });

    // -------

    // API - HTTP POST
    server->on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
    if (index == 0) {
        LOG_INFO("📝 API Config - Salvando configurações");
    }
    
    // Processa os dados JSON recebidos
    String body = String((char*)data, len);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        LOG_ERROR("❌ Erro ao parsear JSON: %s", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    // Atualiza as configurações
    if (doc["autoUpdate"].is<bool>()) {
        bool autoUpdate = doc["autoUpdate"];
        OTAManager::setAutoUpdateEnabled(autoUpdate);
        LOG_INFO("🔧 Auto Update: %s", autoUpdate ? "true" : "false");
    }
    
    if (doc["serverUrl"].is<String>()) {
        String serverUrl = doc["serverUrl"].as<String>();
        OTAManager::setServerUrl(serverUrl);
        LOG_INFO("🔧 Server URL: %s", serverUrl.c_str());
    }
    
    if (doc["updateInterval"].is<int>()) {
        int interval = doc["updateInterval"];
        OTAManager::setUpdateInterval(interval);
        LOG_INFO("🔧 Update Interval: %d horas", interval);
    }
    
    if (index + len == total) {
        JsonDocument responseDoc;
        responseDoc["status"] = "success";
        responseDoc["message"] = "Configurações salvas com sucesso";
        responseDoc["autoUpdate"] = OTAManager::isAutoUpdateEnabled();
        responseDoc["updateInterval"] = OTAManager::getUpdateInterval();
        responseDoc["lastUpdateCheck"] = OTAManager::getLastUpdateCheck();
        
        String jsonResponse;
        serializeJson(responseDoc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
        
        LOG_INFO("✅ Configurações salvas com sucesso");
    } });

    server->on("/api/network/connect", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
    if (index == 0) {
        LOG_INFO("📶 Conectando à rede WiFi");
    }
    
    String body = String((char*)data, len);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        LOG_ERROR("❌ Erro ao parsear JSON: %s", error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();
    
    if (ssid.isEmpty()) {
        request->send(400, "application/json", "{\"error\":\"SSID is required\"}");
        return;
    }
    
    // Salvar credenciais na preferências (não na LittleFS)
    Preferences preferences;
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    
    LOG_INFO("🔑 Credenciais salvas - Conectando à: %s", ssid.c_str());
    
    // Tentar conectar
    WiFi.begin(ssid.c_str(), password.c_str());
    
    JsonDocument responseDoc;
    responseDoc["status"] = "connecting";
    responseDoc["message"] = "Conectando à rede " + ssid;
    responseDoc["ssid"] = ssid;
    
    String jsonResponse;
    serializeJson(responseDoc, jsonResponse);
    request->send(200, "application/json", jsonResponse); });

    server->on("/api/network/clear", HTTP_POST, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("🗑️ Limpando configuração WiFi");
    
    // Limpar credenciais salvas
    Preferences preferences;
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    
    // Desconectar
    WiFi.disconnect(true);
    delay(1000);
    
    // Iniciar modo AP
    WiFi.softAP("ESP32-OTA", "");
    
    request->send(200, "application/json", "{\"status\":\"cleared\", \"message\":\"WiFi config cleared\"}"); });

    // -------

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

    server->on("/check-updates", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    LOG_INFO("🔍 Verificação de atualizações solicitada");
    OTAManager::checkForUpdates();

    LOG_INFO("Versão atual: %s\tVersão do servidor: %s", OTAManager::getFirmwareVersion().c_str(), OTAManager::getLatestVersion().c_str());
    
    JsonDocument doc;
    doc["status"] = "up_to_date";
    doc["message"] = "Sistema atualizado";
    doc["current_version"] = OTAManager::getFirmwareVersion().c_str();
    doc["latest_version"] = OTAManager::getLatestVersion().c_str();
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

    // ✅ API para salvar configurações (CORRIGIDA)

    // -->>✅ Colocar por ultimo<<--
    server->on("^\\/(.+)$", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String filename = request->pathArg(0);
        LOG_INFO("📁 Arquivo solicitado: %s", filename.c_str());

            if (filename.startsWith("api/")) {
        LOG_WARN("⚠️ Rota API não encontrada: %s", filename.c_str());
        request->send(404, "text/plain", "API endpoint not found: " + filename);
        return;
    }
        
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

String WebAssetManager::getEncryptionType(wifi_auth_mode_t encryptionType)
{
    switch (encryptionType)
    {
    case WIFI_AUTH_OPEN:
        return "OPEN";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK:
        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2/WPA3";
    case WIFI_AUTH_WAPI_PSK:
        return "WAPI";
    default:
        return "UNKNOWN";
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

String WebAssetManager::getFileExtension(const String &filename)
{
    int dotIndex = filename.lastIndexOf('.');
    if (dotIndex == -1 || dotIndex == filename.length() - 1)
    {
        return "";
    }
    return filename.substring(dotIndex + 1);
}

size_t WebAssetManager::calculateDirectorySize(const String &path)
{
    size_t totalSize = 0;
    File root = LittleFS.open(path);
    if (!root || !root.isDirectory())
        return 0;

    File file = root.openNextFile();
    while (file)
    {
        if (!file.isDirectory())
        {
            totalSize += file.size();
        }
        file = root.openNextFile();
    }
    root.close();
    return totalSize;
}

String WebAssetManager::getMimeType(const String &extension)
{
    if (extension == "txt" || extension == "log")
        return "text/plain";
    if (extension == "html" || extension == "htm")
        return "text/html";
    if (extension == "css")
        return "text/css";
    if (extension == "js")
        return "application/javascript";
    if (extension == "json")
        return "application/json";
    if (extension == "xml")
        return "application/xml";
    if (extension == "pdf")
        return "application/pdf";
    if (extension == "zip")
        return "application/zip";
    if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    if (extension == "png")
        return "image/png";
    if (extension == "gif")
        return "image/gif";
    if (extension == "bmp")
        return "image/bmp";
    if (extension == "ico")
        return "image/x-icon";
    if (extension == "svg")
        return "image/svg+xml";
    if (extension == "mp3")
        return "audio/mpeg";
    if (extension == "wav")
        return "audio/wav";
    if (extension == "mp4")
        return "video/mp4";
    if (extension == "avi")
        return "video/x-msvideo";
    return "application/octet-stream";
}

String WebAssetManager::getFileSystemInfo()
{
    JsonDocument fsDoc;

    // Método compatível com LittleFS - calcular uso manualmente
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();

    fsDoc["totalBytes"] = totalBytes;
    fsDoc["usedBytes"] = usedBytes;
    fsDoc["freeBytes"] = totalBytes - usedBytes;

    if (totalBytes > 0)
    {
        fsDoc["percentUsed"] = (usedBytes * 100) / totalBytes;
    }
    else
    {
        fsDoc["percentUsed"] = 0;
    }

    String fsInfoStr;
    serializeJson(fsDoc, fsInfoStr);
    return fsInfoStr;
}

// void WebAssetManager::addFileSystemInfo(JsonDocument &doc)
// {
//     size_t totalBytes = LittleFS.totalBytes();
//     size_t usedBytes = LittleFS.usedBytes();
//     size_t freeBytes = totalBytes - usedBytes;

//     doc["fsTotalBytes"] = totalBytes;
//     doc["fsUsedBytes"] = usedBytes;
//     doc["fsFreeBytes"] = freeBytes;

//     if (totalBytes > 0)
//     {
//         doc["fsPercentUsed"] = (usedBytes * 100) / totalBytes;
//     }
//     else
//     {
//         doc["fsPercentUsed"] = 0;
//     }
// }