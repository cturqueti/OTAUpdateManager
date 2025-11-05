#pragma once

/**
 * @file OTAPullUpdateManager.h
 * @brief Sistema de gerenciamento de atualizações OTA (Over-The-Air) via HTTP
 *
 * Esta classe fornece funcionalidades para verificar e baixar atualizações
 * de firmware remotamente, com suporte a execução em thread separada.
 */

#include "ESPmDNS.h"
#include "LogLibrary.h"
#include <HTTPClient.h>
#include <LittleFS.h>
#include <Update.h>

class OTAPullUpdateManager
{
public:
    // ============ INTERFACE PÚBLICA ============

    /**
     * @brief Inicializa o gerenciador com URL do servidor
     * @param serverUrl URL do servidor de atualizações
     */
    static void init(const String &serverUrl);

    /**
     * @brief Inicializa o gerenciador com configurações customizadas
     * @param serverUrl URL do servidor de atualizações
     * @param port Porta do servidor
     * @param versionPath Caminho para endpoint de versão
     * @param firmwarePath Caminho para endpoint do firmware
     */
    static void init(const String &serverUrl, uint16_t port,
                     const String &versionPath = "/version",
                     const String &firmwarePath = "/firmware");

    // Configurações
    static void setServerPort(uint16_t port);
    static void setVersionPath(const String &path);
    static void setFirmwarePath(const String &path);

    // Controle de atualizações
    static void checkForUpdates();
    static bool isUpdating();
    static String getCurrentVersion();
    static String getLatestVersion();
    static void setCurrentVersion(const String &version);

    // Gerenciamento de thread
    static void startUpdateThread(uint16_t checkIntervalMinutes = 1);
    static void stopUpdateThread();
    static bool isThreadRunning();

private:
    // ============ VARIÁVEIS DE ESTADO ============
    static String _firmwareUrl; ///< URL completa para download do firmware
    static String _versionUrl;  ///< URL completa para verificação de versão

    static bool _updating;       ///< Flag indicando se atualização está em progresso
    static uint16_t _serverPort; ///< Porta do servidor de atualizações
    static String _versionPath;  ///< Caminho do endpoint de versão
    static String _firmwarePath; ///< Caminho do endpoint do firmware

    // ============ GERENCIAMENTO DE THREAD ============
    static TaskHandle_t _updateTaskHandle; ///< Handle da task FreeRTOS
    static bool _threadRunning;            ///< Flag de controle da thread
    static uint32_t
        _checkIntervalMs; ///< Intervalo de verificação em milissegundos

    // ============ MÉTODOS PRIVADOS ============

    /**
     * @brief Constrói URLs completas a partir da URL base
     * @param serverUrl URL base do servidor
     * @return true se URLs foram construídas com sucesso
     */
    static bool buildUrls(const String &serverUrl);

    /**
     * @brief Verifica se há nova versão disponível
     * @return true se nova versão está disponível
     */
    static bool checkVersion();

    /**
     * @brief Download e instalação do firmware
     * @return true se atualização foi bem-sucedida
     */
    static bool downloadFirmware();

    /**
     * @brief Carrega versão armazenada no sistema de arquivos
     * @return true se versão foi carregada com sucesso
     */
    static bool loadStoredVersion();

    /**
     * @brief Função executada pela thread de verificação
     * @param parameter Parâmetros da thread
     */
    static void updateTask(void *parameter);

    /**
     * @brief Estrutura para passagem de parâmetros para a thread
     */
    struct ThreadParams
    {
        uint32_t checkIntervalMs; ///< Intervalo de verificação em ms
    };
};