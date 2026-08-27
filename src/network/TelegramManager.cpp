#include "TelegramManager.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include "../core/Config.h"
#include "../core/ConfigManager.h"
#include "../core/Statistics.h"
#include "../services/TimeManager.h"
#include "EthernetManager.h"


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

    String token = config.getBotToken();
    token.trim();

    if (token.length())
    {
        bot.updateToken(token);
    }

    Serial.println("[TG] Telegram initialized");
}


// ============================================================
// UPDATE
// ============================================================

void TelegramManager::update()
{
    if (!ethernet.isConnected())
    {
        started = false;
        return;
    }

    if (!started)
    {
        started = true;

        Serial.println("[TG] Telegram ready");

        sendStartupMessage();
    }

    if (millis() - lastCheck >= TELEGRAM_CHECK_INTERVAL)
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
    if (
        startupMessageSent ||
        !config.isStartupMessageEnabled()
    )
    {
        return;
    }

    String message = config.getStartupMessage();
    message.trim();

    if (
        !message.length() ||
        !config.getChatCount()
    )
    {
        return;
    }

    if (
        sendMessage(
            config.getChat(0).id,
            message
        )
    )
    {
        startupMessageSent = true;
    }
}


// ============================================================
// CHECK MESSAGES
// ============================================================

void TelegramManager::checkMessages()
{
    int count =
        bot.getUpdates(
            bot.last_message_received + 1
        );

    while (count > 0)
    {
        for (int i = 0; i < count; i++)
        {
            handleMessage(i);
        }

        count =
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

    if (text.startsWith("/"))
    {
        statistics.onCommandReceived();

        handleCommand(
            chatId,
            text
        );
    }
}


// ============================================================
// COMMANDS
// ============================================================

void TelegramManager::handleCommand(
    const String& chatId,
    const String& command
)
{
    if (command == "/start")
    {
        statistics.onCommandExecuted();

        sendStartMessage(chatId);

        return;
    }

    if (command == "/about")
    {
        statistics.onCommandExecuted();

        sendAbout(chatId);

        return;
    }

    if (command == "/ping")
    {
        statistics.onCommandExecuted();

        sendMessage(
            chatId,
            "🏓 Pong!"
        );

        return;
    }

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

    if (command == "/status")
    {
        statistics.onCommandExecuted();

        String message;

        message += "🤖 ";
        message += config.getDeviceName();

        message += "\n\n🟢 Online\n";

        message += "🌐 IP: ";
        message += ethernet.getIP();

        message += "\n🔗 Ethernet: ";
        message +=
            ethernet.isConnected()
                ? "Online"
                : "Offline";

        message += "\n⚡ ";
        message += String(
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

    statistics.onCommandError();

    sendMessage(
        chatId,
        "❓ Невідома команда:\n" +
        command +
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
    message += config.getDeviceName();

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
    message += config.getDeviceName();
    message += "</b>\n\n";

    message += "🟢 <b>Стан:</b> Online\n";

    message += "⏱ <b>Поточний uptime:</b> ";
    message += statistics.getCurrentUptimeString();
    message += "\n";

    message += "🕰 <b>Total uptime:</b> ";
    message += statistics.getTotalUptimeString();
    message += "\n";

    message += "🔄 <b>Перезапусків:</b> ";
    message += String(statistics.getBootCount());
    message += "\n\n";

    message += "💬 <b>Повідомлень:</b>\n";
    message += "📥 Отримано: ";
    message += String(statistics.getMessagesReceived());
    message += "\n";
    message += "📤 Надіслано: ";
    message += String(statistics.getMessagesSent());
    message += "\n\n";

    message += "⚙️ <b>Команди:</b>\n";
    message += "📥 Отримано: ";
    message += String(statistics.getCommandsReceived());
    message += "\n";
    message += "✅ Виконано: ";
    message += String(statistics.getCommandsExecuted());
    message += "\n";
    message += "❌ Помилок: ";
    message += String(statistics.getCommandErrors());
    message += "\n\n";

    message += "💾 <b>Система:</b>\n";
    message += "🧠 Free heap: ";
    message += String(ESP.getFreeHeap());
    message += " bytes\n";

    message += "🌐 IP: ";
    message += ethernet.getIP();
    message += "\n";

    message += "❗ Помилок: ";
    message += String(statistics.getErrors());

    if (timeManager.isTimeValid())
    {
        message += "\n\n🕐 ";
        message += timeManager.getDateTimeString();
    }

    sendMessage(chatId, message);
}


// ============================================================
// SEND MESSAGE
// ============================================================

bool TelegramManager::sendMessage(
    const String& chatId,
    const String& text,
    bool silent
)
{
    if (!chatId.length())
    {
        return false;
    }

    DynamicJsonDocument payload(2048);

    payload["chat_id"] = chatId;
    payload["text"] = text;
    payload["parse_mode"] = "HTML";

    if (silent)
    {
        payload["disable_notification"] = true;
    }

    bool result =
        bot.sendPostMessage(
            payload.as<JsonObject>()
        );

    if (result)
    {
        statistics.onTelegramMessageSent();
    }
    else
    {
        statistics.onError();
        Serial.println("[TG] Failed to send message");
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
