#pragma once

#include <Arduino.h>
#include <esp_timer.h>


class LedActivityManager
{
public:

    void begin();

    void update();

    void onRx();

    void onTx();


private:

    esp_timer_handle_t rxTimer = nullptr;
    esp_timer_handle_t txTimer = nullptr;


    uint64_t lastMessagesReceived = 0;
    uint64_t lastMessagesSent = 0;


    static void rxTimerCallback(
        void* arg
    );

    static void txTimerCallback(
        void* arg
    );
};


extern LedActivityManager ledActivity;
