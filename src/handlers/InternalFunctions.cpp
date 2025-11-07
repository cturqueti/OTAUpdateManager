#include "InternalFunctions.h"

String InternalFunctions::getCurrentDateTime()
{
    // Usa o tempo interno do sistema (que deve estar sincronizado com NTP)
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    // Mapeamento de meses em português
    const char *months_Pt_BR[] = {"Jan", "Fev", "Mar", "Abr", "Mai", "Jun",
                                  "Jul", "Ago", "Set", "Out", "Nov", "Dez"};

    // Extrair componentes da data
    int day = timeinfo.tm_mday;
    int month = timeinfo.tm_mon; // 0-11
    int year = timeinfo.tm_year + 1900;
    int hour = timeinfo.tm_hour;
    int min = timeinfo.tm_min;
    int sec = timeinfo.tm_sec;

    // Formatar no estilo DD/MMM/AAAA HH:MM:SS
    char timeStr[64];
    snprintf(timeStr, sizeof(timeStr), "%02d/%03s/%04d %02d:%02d:%02d",
             day, months_Pt_BR[month], year, hour, min, sec);

    return String(timeStr);
}