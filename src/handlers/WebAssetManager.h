#pragma once

#include "StaticFileHandler.h"
#include <ESPAsyncWebServer.h>

class WebAssetManager
{
public:
    static void setupRoutes(AsyncWebServer *server);
    static void setupRoutes_bkp(AsyncWebServer *server);
    static void checkRequiredAssets();
};