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
    unsigned long lastSendAttempt = 0;

    struct PendingNotification
    {
        String chatId;
        String message;
    };
    static const size_t QUEUE_CAPACITY = 32;
    PendingNotification pending[QUEUE_CAPACITY];
    size_t queueHead = 0;
    size_t queueCount = 0;

    void queueNotification(const String& message);
    void processQueue();
    void checkPin35();

    void checkPin39();
};


extern GpioMonitorManager gpioMonitor;