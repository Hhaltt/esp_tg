#pragma once

#include <Arduino.h>

class TelegramManager
{
public:
    void begin();
    void update();
    bool isStarted();

    bool sendMessage(
        const String& chatId,
        const String& text,
        bool silent = false
    );

private:
    bool started = false;
    bool startupMessageSent = false;
    unsigned long lastCheck = 0;

    void checkMessages();
    void handleMessage(int messageIndex);
    void handleCommand(const String& chatId, const String& command);
    void sendStartMessage(const String& chatId);
    void sendAbout(const String& chatId);
    void sendStartupMessage();
};

extern TelegramManager telegram;
