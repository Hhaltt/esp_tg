#pragma once

#include <Arduino.h>
#include <time.h>


class TimeManager
{
public:

    void begin();

    void update();


    // --------------------------------------------------------
    // STATUS
    // --------------------------------------------------------

    bool isTimeValid();


    // --------------------------------------------------------
    // FORMATTED TIME
    // --------------------------------------------------------

    String getTimeString();

    String getDateString();

    String getDateTimeString();


    // --------------------------------------------------------
    // RAW TIME
    // --------------------------------------------------------

    time_t getTimestamp();

    int getHour();

    int getMinute();

    int getSecond();


private:

    bool timeValid = false;


    // --------------------------------------------------------
    // NTP SYNCHRONIZATION
    // --------------------------------------------------------

    unsigned long lastSyncAttempt = 0;


    // Успішна пересинхронізація
    // раз на 6 годин

    static const unsigned long
        NTP_SYNC_INTERVAL =
            6UL *
            60UL *
            60UL *
            1000UL;


    // Якщо NTP не відповідає —
    // повторюємо через 1 хвилину

    static const unsigned long
        NTP_RETRY_INTERVAL =
            60UL *
            1000UL;
};


extern TimeManager timeManager;