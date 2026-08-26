#pragma once

#include <Arduino.h>


class TelegramManager
{
public:

    void begin();

    void update();


    bool isStarted();


    // --------------------------------------------------------
    // Send messages
    // --------------------------------------------------------

    bool sendMessage(
        const String& chatId,
        const String& text
    );


private:

    bool started = false;

    bool startupMessageSent = false;

    unsigned long lastCheck = 0;


    void checkMessages();

    void handleMessage(
        int messageIndex
    );


    // Commands

    void handleCommand(
        const String& chatId,
        const String& command
    );


    void sendStartMessage(
        const String& chatId
    );


    void sendAbout(
        const String& chatId
    );


    // Startup message

    void sendStartupMessage();
};


extern TelegramManager telegram;