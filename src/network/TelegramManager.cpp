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
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);
TelegramManager telegram;

void TelegramManager::begin()
{
    telegramClient.setInsecure();
    String token = config.getBotToken();
    token.trim();
    if (token.length()) bot.updateToken(token);
    Serial.println("[TG] Telegram initialized");
}

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

void TelegramManager::sendStartupMessage()
{
    if (startupMessageSent || !config.isStartupMessageEnabled()) return;
    String message = config.getStartupMessage();
    message.trim();
    if (!message.length() || !config.getChatCount()) return;
    if (sendMessage(config.getChat(0).id, message)) startupMessageSent = true;
}

void TelegramManager::checkMessages()
{
    int count = bot.getUpdates(bot.last_message_received + 1);
    while (count > 0)
    {
        for (int i = 0; i < count; i++) handleMessage(i);
        count = bot.getUpdates(bot.last_message_received + 1);
    }
}

void TelegramManager::handleMessage(int messageIndex)
{
    statistics.onTelegramMessageReceived();
    String chatId = bot.messages[messageIndex].chat_id;
    String text = bot.messages[messageIndex].text;

    // Ordinary chat messages are ignored. Only explicit commands are handled.
    if (text.startsWith("/"))
    {
        statistics.onCommandReceived();
        handleCommand(chatId, text);
    }
}

void TelegramManager::handleCommand(const String& chatId, const String& command)
{
    if (command == "/start") { statistics.onCommandExecuted(); sendStartMessage(chatId); return; }
    if (command == "/about") { statistics.onCommandExecuted(); sendAbout(chatId); return; }
    if (command == "/ping") { statistics.onCommandExecuted(); sendMessage(chatId, "🏓 Pong!"); return; }
    if (command == "/time") { statistics.onCommandExecuted(); sendMessage(chatId, "🕐 " + timeManager.getDateTimeString()); return; }
    if (command == "/status")
    {
        statistics.onCommandExecuted();
        String message;
        message += "🤖 " + config.getDeviceName();
        message += "\n\n🟢 Online\n";
        message += "🌐 IP: " + ethernet.getIP();
        message += "\n🔗 Ethernet: ";
        message += ethernet.isConnected() ? "Online" : "Offline";
        message += "\n⚡ " + String(ethernet.getLinkSpeed()) + " Mbps\n";
        message += "⏱ Uptime: " + statistics.getCurrentUptimeString();
        sendMessage(chatId, message);
        return;
    }
    statistics.onCommandError();
    sendMessage(chatId, "❓ Невідома команда:\n" + command + "\n\nСпробуй /start");
}

void TelegramManager::sendStartMessage(const String& chatId)
{
    String message = "🤖 " + config.getDeviceName() + "\n\n";
    message += "Я запущений і працюю 😎\n\n";
    message += "📊 /about - про себе\n📡 /status - статус\n🏓 /ping - перевірка\n🕐 /time - час";
    sendMessage(chatId, message);
}

void TelegramManager::sendAbout(const String& chatId)
{
    String message;
    message += "🤖 <b>" + config.getDeviceName() + "</b>\n\n";
    message += "🟢 <b>Стан:</b> Online\n";
    message += "⏱ <b>Поточний uptime:</b> " + statistics.getCurrentUptimeString() + "\n";
    message += "🕰 <b>Total uptime:</b> " + statistics.getTotalUptimeString() + "\n";
    message += "🔄 <b>Перезапусків:</b> " + String(statistics.getBootCount()) + "\n\n";
    message += "💬 <b>Повідомлень:</b>\n📥 Отримано: " + String(statistics.getMessagesReceived()) + "\n📤 Надіслано: " + String(statistics.getMessagesSent()) + "\n\n";
    message += "⚙️ <b>Команди:</b>\n📥 Отримано: " + String(statistics.getCommandsReceived()) + "\n✅ Виконано: " + String(statistics.getCommandsExecuted()) + "\n❌ Помилок: " + String(statistics.getCommandErrors()) + "\n\n";
    message += "💾 <b>Система:</b>\n🧠 Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    message += "🌐 IP: " + ethernet.getIP() + "\n";
    message += "❗ Помилок: " + String(statistics.getErrors());
    if (timeManager.isTimeValid()) message += "\n\n🕐 " + timeManager.getDateTimeString();
    sendMessage(chatId, message);
}

bool TelegramManager::sendMessage(const String& chatId, const String& text, bool silent)
{
    if (!chatId.length()) return false;
    DynamicJsonDocument payload(2048);
    payload["chat_id"] = chatId;
    payload["text"] = text;
    payload["parse_mode"] = "HTML";
    if (silent) payload["disable_notification"] = true;
    bool result = bot.sendPostMessage(payload.as<JsonObject>());
    if (result) statistics.onTelegramMessageSent();
    else
    {
        statistics.onError();
        Serial.println("[TG] Failed to send message");
    }
    return result;
}

bool TelegramManager::isStarted()
{
    return started;
}
