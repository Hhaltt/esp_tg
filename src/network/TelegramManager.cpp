#include "TelegramManager.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "../core/Config.h"
#include "../core/Statistics.h"

#include "../services/TimeManager.h"
#include "../core/ConfigManager.h"

#include "EthernetManager.h"


// ============================================================
// TELEGRAM CLIENT
// ============================================================

WiFiClientSecure telegramClient;


UniversalTelegramBot bot(
    BOT_TOKEN,
    telegramClient
);


TelegramManager telegram;


// ============================================================
// BEGIN
// ============================================================

void TelegramManager::begin()
{
    telegramClient.setInsecure();

    Serial.println(
        "[TG] Telegram initialized"
    );
}


// ============================================================
// UPDATE
// ============================================================

void TelegramManager::update()
{
    // Ethernet обов'язково потрібен

    if (!ethernet.isConnected())
    {
        started = false;

        return;
    }


    if (!started)
    {
        started = true;

        Serial.println(
            "[TG] Telegram ready"
        );


        // Стартове повідомлення

        sendStartupMessage();
    }


    if (
        millis() - lastCheck
        >= TELEGRAM_CHECK_INTERVAL
    )
    {
        lastCheck = millis();

        checkMessages();
    }
}


// ============================================================
// STARTUP MESSAGE
// ============================================================

void TelegramManager::sendStartupMessage()
{
    // Уже відправляли в цьому запуску

    if (startupMessageSent)
    {
        return;
    }


    // Вимкнено в налаштуваннях

    if (!config.isStartupMessageEnabled())
    {
        Serial.println(
            "[TG] Startup message disabled"
        );

        return;
    }


    String message =
        config.getStartupMessage();


    // Порожнє повідомлення

    message.trim();


    if (message.length() == 0)
    {
        Serial.println(
            "[TG] Startup message is empty"
        );

        return;
    }


    Serial.println(
        "[TG] Sending startup message..."
    );


    if (
        sendMessage(
            TELEGRAM_CHAT_ID,
            message
        )
    )
    {
        startupMessageSent = true;

        Serial.println(
            "[TG] Startup message sent"
        );
    }
    else
    {
        Serial.println(
            "[TG] Failed to send startup message"
        );
    }
}


// ============================================================
// CHECK MESSAGES
// ============================================================

void TelegramManager::checkMessages()
{
    int numNewMessages =
        bot.getUpdates(
            bot.last_message_received + 1
        );


    while (numNewMessages > 0)
    {
        Serial.printf(
            "[TG] %d new message(s)\n",
            numNewMessages
        );


        for (
            int i = 0;
            i < numNewMessages;
            i++
        )
        {
            handleMessage(i);
        }


        numNewMessages =
            bot.getUpdates(
                bot.last_message_received + 1
            );
    }
}


// ============================================================
// HANDLE MESSAGE
// ============================================================

void TelegramManager::handleMessage(
    int messageIndex
)
{
    statistics.onTelegramMessageReceived();


    String chatId =
        bot.messages[messageIndex].chat_id;


    String text =
        bot.messages[messageIndex].text;


    String fromName =
        bot.messages[messageIndex].from_name;


    Serial.println(
        "\n========== TELEGRAM =========="
    );


    Serial.print("From: ");
    Serial.println(fromName);


    Serial.print("Chat ID: ");
    Serial.println(chatId);


    Serial.print("Text: ");
    Serial.println(text);


    Serial.println(
        "==============================\n"
    );


    // --------------------------------------------------------
    // COMMAND
    // --------------------------------------------------------

    if (text.startsWith("/"))
    {
        statistics.onCommandReceived();

        handleCommand(
            chatId,
            text
        );
    }
    else
    {
        sendMessage(
            chatId,
            "Я поки що розумію тільки команди.\n\n"
            "Спробуй /start"
        );
    }
}


// ============================================================
// HANDLE COMMAND
// ============================================================

void TelegramManager::handleCommand(
    const String& chatId,
    const String& command
)
{
    // --------------------------------------------------------
    // /start
    // --------------------------------------------------------

    if (command == "/start")
    {
        statistics.onCommandExecuted();

        sendStartMessage(chatId);

        return;
    }


    // --------------------------------------------------------
    // /about
    // --------------------------------------------------------

    if (command == "/about")
    {
        statistics.onCommandExecuted();

        sendAbout(chatId);

        return;
    }


    // --------------------------------------------------------
    // /ping
    // --------------------------------------------------------

    if (command == "/ping")
    {
        statistics.onCommandExecuted();

        sendMessage(
            chatId,
            "🏓 Pong!"
        );

        return;
    }


    // --------------------------------------------------------
    // /time
    // --------------------------------------------------------

    if (command == "/time")
    {
        statistics.onCommandExecuted();

        sendMessage(
            chatId,
            "🕐 " +
            timeManager.getDateTimeString()
        );

        return;
    }


    // --------------------------------------------------------
    // /status
    // --------------------------------------------------------

    if (command == "/status")
    {
        statistics.onCommandExecuted();


        String message;


        message += "🤖 ";
        message += DEVICE_NAME;

        message += "\n\n";


        message += "🟢 Online\n";


        message += "🌐 IP: ";
        message += ethernet.getIP();

        message += "\n";


        message += "🔗 Ethernet: ";

        if (ethernet.isConnected())
        {
            message += "Online";
        }
        else
        {
            message += "Offline";
        }


        message += "\n";


        message += "⚡ ";
        message +=
            String(
                ethernet.getLinkSpeed()
            );

        message += " Mbps\n";


        message += "⏱ Uptime: ";
        message +=
            statistics.getCurrentUptimeString();


        sendMessage(
            chatId,
            message
        );

        return;
    }


    // --------------------------------------------------------
    // UNKNOWN COMMAND
    // --------------------------------------------------------

    statistics.onCommandError();


    sendMessage(
        chatId,
        "❓ Невідома команда:\n"
        + command
        +
        "\n\nСпробуй /start"
    );
}


// ============================================================
// START MESSAGE
// ============================================================

void TelegramManager::sendStartMessage(
    const String& chatId
)
{
    String message;


    message += "🤖 ";
    message += DEVICE_NAME;

    message += "\n\n";


    message += "Я запущений і працюю 😎\n\n";


    message += "📊 /about - про себе\n";

    message += "📡 /status - статус\n";

    message += "🏓 /ping - перевірка\n";

    message += "🕐 /time - час";


    sendMessage(
        chatId,
        message
    );
}


// ============================================================
// ABOUT
// ============================================================

void TelegramManager::sendAbout(
    const String& chatId
)
{
    String message;


    message += "🤖 <b>";
    message += DEVICE_NAME;
    message += "</b>\n\n";


    // --------------------------------------------------------
    // Status
    // --------------------------------------------------------

    message += "🟢 <b>Стан:</b> Online\n";


    // --------------------------------------------------------
    // Uptime
    // --------------------------------------------------------

    message += "⏱ <b>Поточний uptime:</b> ";
    message +=
        statistics.getCurrentUptimeString();

    message += "\n";

    message += "🕰 <b>Total uptime:</b> ";
    message +=
        statistics.getTotalUptimeString();

    message += "\n";


    // --------------------------------------------------------
    // Boot
    // --------------------------------------------------------

    message += "🔄 <b>Перезапусків:</b> ";
    message +=
        String(
            statistics.getBootCount()
        );

    message += "\n\n";


    // --------------------------------------------------------
    // Messages
    // --------------------------------------------------------

    message += "💬 <b>Повідомлень:</b>\n";


    message += "📥 Отримано: ";
    message +=
        String(
            statistics.getMessagesReceived()
        );

    message += "\n";


    message += "📤 Надіслано: ";
    message +=
        String(
            statistics.getMessagesSent()
        );

    message += "\n\n";


    // --------------------------------------------------------
    // Commands
    // --------------------------------------------------------

    message += "⚙️ <b>Команди:</b>\n";


    message += "📥 Отримано: ";
    message +=
        String(
            statistics.getCommandsReceived()
        );

    message += "\n";


    message += "✅ Виконано: ";
    message +=
        String(
            statistics.getCommandsExecuted()
        );

    message += "\n";


    message += "❌ Помилок: ";
    message +=
        String(
            statistics.getCommandErrors()
        );

    message += "\n\n";


    // --------------------------------------------------------
    // System
    // --------------------------------------------------------

    message += "💾 <b>Система:</b>\n";


    message += "🧠 Free heap: ";
    message +=
        String(
            ESP.getFreeHeap()
        );

    message += " bytes\n";


    message += "🌐 IP: ";
    message +=
        ethernet.getIP();


    // --------------------------------------------------------
    // Time
    // --------------------------------------------------------

    if (timeManager.isTimeValid())
    {
        message += "\n\n🕐 ";
        message +=
            timeManager.getDateTimeString();
    }


    sendMessage(
        chatId,
        message
    );
}


// ============================================================
// SEND MESSAGE
// ============================================================

bool TelegramManager::sendMessage(
    const String& chatId,
    const String& text
)
{
    bool result =
        bot.sendMessage(
            chatId,
            text,
            "HTML"
        );


    if (result)
    {
        statistics.onTelegramMessageSent();
    }
    else
    {
        statistics.onError();

        Serial.println(
            "[TG] Failed to send message"
        );
    }


    return result;
}


// ============================================================
// STATUS
// ============================================================

bool TelegramManager::isStarted()
{
    return started;
}