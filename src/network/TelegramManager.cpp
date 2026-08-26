#include "TelegramManager.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

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
    if (token.length()) bot = UniversalTelegramBot(token, telegramClient);
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
    TelegramChatConfig chat = config.getChat(0);
    if (sendMessage(chat.id, message)) startupMessageSent = true;
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

void TelegramManager::handleMessage(int index)
{
    statistics.onTelegramMessageReceived();
    String chatId = bot.messages[index].chat_id;
    String text = bot.messages[index].text;
    if (text.startsWith("/"))
    {
        statistics.onCommandReceived();
        handleCommand(chatId, text);
    }
    else sendMessage(chatId, "Я поки що розумію тільки команди.\n\nСпробуй /start");
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
        String m = "🤖 " + config.getDeviceName() + "\n\n🟢 Online\n";
        m += "🌐 IP: " + ethernet.getIP() + "\n";
        m += "🔗 Ethernet: " + String(ethernet.isConnected() ? "Online" : "Offline") + "\n";
        m += "⚡ " + String(ethernet.getLinkSpeed()) + " Mbps\n";
        m += "⏱ Uptime: " + statistics.getCurrentUptimeString();
        sendMessage(chatId, m); return;
    }
    statistics.onCommandError();
    sendMessage(chatId, "❓ Невідома команда:\n" + command + "\n\nСпробуй /start");
}

void TelegramManager::sendStartMessage(const String& chatId)
{
    String m = "🤖 " + config.getDeviceName() + "\n\nЯ запущений і працюю 😎\n\n";
    m += "📊 /about - про себе\n📡 /status - статус\n🏓 /ping - перевірка\n🕐 /time - час";
    sendMessage(chatId, m);
}

void TelegramManager::sendAbout(const String& chatId)
{
    String m = "🤖 <b>" + config.getDeviceName() + "</b>\n\n";
    m += "🟢 <b>Стан:</b> Online\n";
    m += "⏱ <b>Поточний uptime:</b> " + statistics.getCurrentUptimeString() + "\n";
    m += "🕰 <b>Total uptime:</b> " + statistics.getTotalUptimeString() + "\n";
    m += "🔄 <b>Перезапусків:</b> " + String(statistics.getBootCount()) + "\n\n";
    m += "💬 <b>Повідомлень:</b>\n📥 Отримано: " + String(statistics.getMessagesReceived()) + "\n📤 Надіслано: " + String(statistics.getMessagesSent()) + "\n\n";
    m += "💾 <b>Система:</b>\n🧠 Free heap: " + String(ESP.getFreeHeap()) + " bytes\n🌐 IP: " + ethernet.getIP();
    if (timeManager.isTimeValid()) m += "\n\n🕐 " + timeManager.getDateTimeString();
    sendMessage(chatId, m);
}

bool TelegramManager::sendMessage(const String& chatId, const String& text, bool silent)
{
    if (!chatId.length()) return false;
    bool result = bot.sendMessage(chatId, text, "HTML", silent);
    if (result) statistics.onTelegramMessageSent();
    else { statistics.onError(); Serial.println("[TG] Failed to send message"); }
    return result;
}

bool TelegramManager::isStarted() { return started; }
