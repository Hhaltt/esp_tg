#pragma once

#include <Arduino.h>


class TimeManager
{
public:

    void begin();

    void update();


    bool isTimeValid();


    // --------------------------------------------------------
    // Time
    // --------------------------------------------------------

    String getTimeString();

    String getDateString();

    String getDateTimeString();


    // --------------------------------------------------------
    // Raw time
    // --------------------------------------------------------

    time_t getTimestamp();

    int getHour();

    int getMinute();

    int getSecond();


private:

    bool timeValid = false;

    unsigned long lastCheck = 0;
};


extern TimeManager timeManager;