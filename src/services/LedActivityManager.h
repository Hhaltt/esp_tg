#pragma once

#include <Arduino.h>


class LedActivityManager
{
public:

    void begin();

    void update();


    void onRx();

    void onTx();


private:

    bool rxActive = false;

    bool txActive = false;


    unsigned long rxStartedAt = 0;

    unsigned long txStartedAt = 0;


    uint64_t lastMessagesReceived = 0;

    uint64_t lastMessagesSent = 0;
};


extern LedActivityManager ledActivity;
