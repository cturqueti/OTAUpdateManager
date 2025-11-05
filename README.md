# OTA Update Manager for ESP32

Biblioteca completa para atualizações OTA (Over-The-Air) no ESP32 com interface web moderna e sistema de pull automático.

## 🚀 Características

- **🔄 Dual Mode**: Push (web) + Pull (HTTP automático)
- **🌐 Interface Web**: Design responsivo com temas dark/light
- **📱 Multi-threading**: Execução em background com FreeRTOS
- **🔒 Segurança**: Autenticação básica HTTP
- **📊 Logs**: Sistema de logging integrado
- **🔄 Versionamento**: Comparação semântica de versões

## 📦 Instalação

### PlatformIO
```ini
lib_deps = 
    cturqueti/OTAUpdateManager
