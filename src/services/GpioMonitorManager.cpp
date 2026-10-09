#include "GpioMonitorManager.h"

#include "../core/Config.h"
#include "../core/ConfigManager.h"

#include "../network/TelegramManager.h"


GpioMonitorManager gpioMonitor;


// ============================================================
// BEGIN
// ============================================================

void GpioMonitorManager::begin()
{
    pinMode(
        PIN_35,
        INPUT
    );


    pinMode(
        PIN_39,
        INPUT
    );


    Serial.println(
        "[GPIO] Monitor initialized"
    );
}


// ============================================================
// UPDATE
// ============================================================

void GpioMonitorManager::update()
{
    if (
        millis() - lastCheck < 100
    )
    {
        return;
    }


    lastCheck = millis();


    checkPin35();
    checkPin39();
    processQueue();
}


void GpioMonitorManager::queueNotification(const String& message)
{
    for (size_t i = 0; i < config.getChatCount(); ++i)
    {
        if (queueCount >= QUEUE_CAPACITY)
        {
            Serial.println("[GPIO] Notification queue full; dropping message");
            return;
        }
        TelegramChatConfig chat = config.getChat(i);
        size_t tail = (queueHead + queueCount) % QUEUE_CAPACITY;
        pending[tail].chatId = chat.id;
        pending[tail].message = message;
        ++queueCount;
    }
}

void GpioMonitorManager::processQueue()
{
    if (!queueCount || millis() - lastSendAttempt < 250) return;
    lastSendAttempt = millis();

    PendingNotification item = pending[queueHead];
    queueHead = (queueHead + 1) % QUEUE_CAPACITY;
    --queueCount;

    telegram.sendMessage(item.chatId, item.message);
}

// ============================================================
// GPIO 35
// ============================================================

void GpioMonitorManager::checkPin35()
{
    if (
        !config.isGpio35Enabled()
    )
    {
        pin35Initialized = false;

        return;
    }


    bool state =
        digitalRead(PIN_35);


    // --------------------------------------------------------
    // FIRST READ
    // --------------------------------------------------------

    if (!pin35Initialized)
    {
        pin35State = state;

        pin35Initialized = true;


        Serial.print(
            "[GPIO] GPIO35 initial state: "
        );

        Serial.println(
            state
                ? "HIGH"
                : "LOW"
        );

        return;
    }


    // --------------------------------------------------------
    // NO CHANGE
    // --------------------------------------------------------

    if (state == pin35State)
    {
        return;
    }


    pin35State = state;


    Serial.print(
        "[GPIO] GPIO35 changed to "
    );

    Serial.println(
        state
            ? "HIGH"
            : "LOW"
    );


    // --------------------------------------------------------
    // SEND
    // --------------------------------------------------------

    String message;


    if (state)
    {
        message =
            config.getGpio35HighMessage();
    }
    else
    {
        message =
            config.getGpio35LowMessage();
    }


    message.trim();


    if (
        message.length() == 0
    )
    {
        return;
    }


    queueNotification(message);
}


// ============================================================
// GPIO 39
// ============================================================

void GpioMonitorManager::checkPin39()
{
    if (
        !config.isGpio39Enabled()
    )
    {
        pin39Initialized = false;

        return;
    }


    bool state =
        digitalRead(PIN_39);


    // --------------------------------------------------------
    // FIRST READ
    // --------------------------------------------------------

    if (!pin39Initialized)
    {
        pin39State = state;

        pin39Initialized = true;


        Serial.print(
            "[GPIO] GPIO39 initial state: "
        );

        Serial.println(
            state
                ? "HIGH"
                : "LOW"
        );

        return;
    }


    // --------------------------------------------------------
    // NO CHANGE
    // --------------------------------------------------------

    if (state == pin39State)
    {
        return;
    }


    pin39State = state;


    Serial.print(
        "[GPIO] GPIO39 changed to "
    );

    Serial.println(
        state
            ? "HIGH"
            : "LOW"
    );


    // --------------------------------------------------------
    // SEND
    // --------------------------------------------------------

    String message;


    if (state)
    {
        message =
            config.getGpio39HighMessage();
    }
    else
    {
        message =
            config.getGpio39LowMessage();
    }


    message.trim();


    if (
        message.length() == 0
    )
    {
        return;
    }


    queueNotification(message);
}