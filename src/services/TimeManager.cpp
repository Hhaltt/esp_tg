#include "TimeManager.h"

#include <time.h>

#include "../core/Config.h"
#include "../network/EthernetManager.h"


TimeManager timeManager;


// ============================================================
// BEGIN
// ============================================================

void TimeManager::begin()
{
    Serial.println(
        "[TIME] Waiting for Ethernet..."
    );
}


// ============================================================
// UPDATE
// ============================================================

void TimeManager::update()
{
    if (!ethernet.isConnected())
    {
        return;
    }


    // Якщо час ще не налаштований —
    // пробуємо отримати NTP

    if (!timeValid)
    {
        Serial.println(
            "[TIME] Starting NTP..."
        );


        configTime(
            GMT_OFFSET_SEC,
            DAYLIGHT_OFFSET_SEC,
            NTP_SERVER_1,
            NTP_SERVER_2
        );


        struct tm timeinfo;


        if (
            getLocalTime(
                &timeinfo,
                5000
            )
        )
        {
            timeValid = true;


            Serial.println(
                "[TIME] NTP synchronized"
            );

            Serial.println(
                getDateTimeString()
            );
        }
        else
        {
            Serial.println(
                "[TIME] Failed to get NTP time"
            );
        }
    }
}


// ============================================================
// STATUS
// ============================================================

bool TimeManager::isTimeValid()
{
    return timeValid;
}


// ============================================================
// FORMATTED TIME
// ============================================================

String TimeManager::getTimeString()
{
    if (!timeValid)
    {
        return "Time not synchronized";
    }


    struct tm timeinfo;


    if (!getLocalTime(&timeinfo))
    {
        return "Time error";
    }


    char buffer[16];


    strftime(
        buffer,
        sizeof(buffer),
        "%H:%M:%S",
        &timeinfo
    );


    return String(buffer);
}


String TimeManager::getDateString()
{
    if (!timeValid)
    {
        return "Date not synchronized";
    }


    struct tm timeinfo;


    if (!getLocalTime(&timeinfo))
    {
        return "Date error";
    }


    char buffer[32];


    strftime(
        buffer,
        sizeof(buffer),
        "%d.%m.%Y",
        &timeinfo
    );


    return String(buffer);
}


String TimeManager::getDateTimeString()
{
    if (!timeValid)
    {
        return "Time not synchronized";
    }


    return
        getDateString()
        +
        " "
        +
        getTimeString();
}


// ============================================================
// RAW TIME
// ============================================================

time_t TimeManager::getTimestamp()
{
    return time(nullptr);
}


int TimeManager::getHour()
{
    struct tm timeinfo;


    if (!getLocalTime(&timeinfo))
    {
        return -1;
    }


    return timeinfo.tm_hour;
}


int TimeManager::getMinute()
{
    struct tm timeinfo;


    if (!getLocalTime(&timeinfo))
    {
        return -1;
    }


    return timeinfo.tm_min;
}


int TimeManager::getSecond()
{
    struct tm timeinfo;


    if (!getLocalTime(&timeinfo))
    {
        return -1;
    }


    return timeinfo.tm_sec;
}