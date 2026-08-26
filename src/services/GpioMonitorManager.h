#pragma once

#include <Arduino.h>


class GpioMonitorManager
{
public:

    void begin();

    void update();


private:

    static const int PIN_35 = 35;
    static const int PIN_39 = 39;


    bool pin35State = false;
    bool pin39State = false;


    bool pin35Initialized = false;
    bool pin39Initialized = false;


    unsigned long lastCheck = 0;


    void checkPin35();

    void checkPin39();
};


extern GpioMonitorManager gpioMonitor;