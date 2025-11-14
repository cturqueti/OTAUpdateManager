#pragma once

#include "LogLibrary.h"
#include "OTAPullUpdateManager.h"
#include "OTAPushUpdateManager.h"
#include <ArduinoJson.h>

class OTAManager
{
public:
    enum UpdateMode
    {
        MANUAL,    // Apenas via página web
        AUTOMATIC, // Apenas pull automático
        HYBRID     // Ambos (padrão)
    };

    enum VersionComparison
    {
        VERSION_OLDER = -1, // Versão A é mais antiga que B
        VERSION_EQUAL = 0,  // Versões são iguais
        VERSION_NEWER = 1   // Versão A é mais nova que B
    };

    static void begin(const String &serverUrl = "", uint16_t webPort = 80, UpdateMode mode = HYBRID);
    static void end();
    static void setUpdateMode(UpdateMode mode);
    static void setServerUrl(const String &serverUrl);
    static String getServerUrl();
    static void setWebCredentials(const String &username, const String &password);
    static void setMDNS(const String &hostname);
    static void setPullInterval(uint16_t minutes);

    static void checkForUpdates();
    static bool isUpdateAvailable();
    static void performUpdate();

    static VersionComparison compareVersions(const String &v1, const String &v2);
    static UpdateMode getCurrentMode();
    static String getLatestVersion();
    static String getUpdateStatus();
    static String getFirmwareVersion();
    static void setFirmwareVersion(const String &version);

    static bool isAutoUpdateEnabled();
    static void setAutoUpdateEnabled(bool enabled);
    static int getUpdateInterval();
    static void setUpdateInterval(int hours);
    static String getLastUpdateCheck();

private:
    static UpdateMode _currentMode;
    static String _serverUrl;
    static bool _updateAvailable;
    static String _latestVersion;
    static String _currentVersion;

    static bool _autoUpdateEnabled;
    static int _updateIntervalHours;
    static unsigned long _lastUpdateCheck;
    static unsigned long _lastConfigSave;

    static esp_err_t init();
    static esp_err_t writeVersion(const String &version);

    static void loadConfig();
    static void saveConfig();
    static esp_err_t writeConfigToFile();
    static esp_err_t readConfigFromFile();
};