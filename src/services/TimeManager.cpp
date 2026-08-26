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
    // --------------------------------------------------------
    // ETHERNET REQUIRED
    // --------------------------------------------------------

    if (!ethernet.isConnected())
    {
        return;
    }


    // --------------------------------------------------------
    // DETERMINE INTERVAL
    // --------------------------------------------------------

    unsigned long interval;


    if (timeValid)
    {
        // Час вже отриманий —
        // синхронізуємо раз на 6 годин

        interval =
            NTP_SYNC_INTERVAL;
    }
    else
    {
        // Час ще не отриманий —
        // повторюємо спробу раз на хвилину

        interval =
            NTP_RETRY_INTERVAL;
    }


    // --------------------------------------------------------
    // CHECK TIME
    // --------------------------------------------------------

    if (
        lastSyncAttempt != 0 &&
        millis() - lastSyncAttempt < interval
    )
    {
        return;
    }


    // Запам'ятовуємо час спроби

    lastSyncAttempt =
        millis();


    // --------------------------------------------------------
    // START NTP
    // --------------------------------------------------------

    Serial.println(
        "[TIME] Synchronizing with NTP..."
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
        bool wasValid =
            timeValid;


        timeValid = true;


        if (wasValid)
        {
            Serial.println(
                "[TIME] NTP resynchronized"
            );
        }
        else
        {
            Serial.println(
                "[TIME] NTP synchronized"
            );
        }


        Serial.println(
            getDateTimeString()
        );
    }
    else
    {
        Serial.println(
            "[TIME] Failed to synchronize NTP"
        );
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