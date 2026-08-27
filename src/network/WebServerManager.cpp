#include "WebServerManager.h"

#include <ETH.h>
#include <ArduinoJson.h>

#include "../core/ConfigManager.h"
#include "../services/ReminderManager.h"
#include "TelegramManager.h"

WebServerManager webServerManager;

static void sendJson(WebServer& s, int code, bool ok, const String& message)
{
    DynamicJsonDocument doc(256);
    doc["ok"] = ok;
    doc["message"] = message;

    String out;
    serializeJson(doc, out);
    s.send(code, "application/json; charset=utf-8", out);
}

static bool readReminder(WebServer& s, Reminder& r, bool edit)
{
    String msg = s.arg("message");
    String chat = s.arg("chatId");
    String type = s.arg("type");
    String tm = s.arg("time");

    msg.trim();
    chat.trim();

    if (!msg.length() || !config.hasChat(chat) || tm.length() != 5)
        return false;

    r.message = msg;
    r.chatId = chat;
    r.silent = s.hasArg("silent");
    r.enabled = edit ? s.hasArg("enabled") : true;
    r.hour = tm.substring(0, 2).toInt();
    r.minute = tm.substring(3, 5).toInt();

    if (r.hour > 23 || r.minute > 59)
        return false;

    if (type == "daily") r.type = ReminderType::DAILY;
    else if (type == "weekly") r.type = ReminderType::WEEKLY;
    else if (type == "monthly") r.type = ReminderType::MONTHLY;
    else if (type == "yearly") r.type = ReminderType::YEARLY;
    else r.type = ReminderType::ONCE;

    if (r.type == ReminderType::ONCE)
    {
        String d = s.arg("onceDate");
        if (d.length() != 10) return false;
        r.year = d.substring(0, 4).toInt();
        r.month = d.substring(5, 7).toInt();
        r.day = d.substring(8, 10).toInt();
        return r.year >= 2000 && r.month >= 1 && r.month <= 12 && r.day >= 1 && r.day <= 31;
    }

    if (r.type == ReminderType::WEEKLY)
    {
        r.weekday = s.arg("weekday").toInt();
        return r.weekday <= 6;
    }

    if (r.type == ReminderType::MONTHLY)
    {
        r.day = s.arg("monthlyDay").toInt();
        return r.day >= 1 && r.day <= 31;
    }

    if (r.type == ReminderType::YEARLY)
    {
        r.day = s.arg("yearlyDay").toInt();
        r.month = s.arg("yearlyMonth").toInt();
        return r.day >= 1 && r.day <= 31 && r.month >= 1 && r.month <= 12;
    }

    return true;
}

void WebServerManager::begin()
{
    server.on("/", HTTP_GET, [this]() {
        server.sendHeader("Location", "/sd/");
        server.send(303);
    });

    server.on("/settings", HTTP_GET, [this]() {
        server.sendHeader("Location", "/sd/settings.html");
        server.send(303);
    });

    server.on("/reminders", HTTP_GET, [this]() {
        server.sendHeader("Location", "/sd/reminders.html");
        server.send(303);
    });

    // Keep secrets out of normal settings JSON. The web UI only shows masked inputs.
    server.on("/api/secrets", HTTP_GET, [this]() {
        sendJson(server, 410, false, "Deprecated endpoint");
    });

    server.on("/settings/save", HTTP_POST, [this]() { handleSaveSettings(); });
    server.on("/settings/chat/add", HTTP_POST, [this]() { handleAddChat(); });
    server.on("/settings/chat/edit", HTTP_POST, [this]() { handleUpdateChat(); });
    server.on("/settings/chat/delete", HTTP_POST, [this]() { handleDeleteChat(); });

    server.on("/reminders/add", HTTP_POST, [this]() { handleAddReminder(); });
    server.on("/reminders/edit", HTTP_POST, [this]() { handleUpdateReminder(); });
    server.on("/reminders/clone", HTTP_POST, [this]() { handleCloneReminder(); });
    server.on("/reminders/delete", HTTP_POST, [this]() { handleDeleteReminder(); });
    server.on("/reminders/toggle", HTTP_POST, [this]() { handleToggleReminder(); });

    // Legacy endpoint.
    server.on("/api/telegram/send", HTTP_POST, [this]() { handleHubTelegramSend(); });
    // Stable external API.
    server.on("/api/v1/telegram/send", HTTP_POST, [this]() { handleHubTelegramSend(); });

    server.onNotFound([this]() { handleNotFound(); });
    server.begin();

    Serial.printf("[WEB] Started: http://%s\n", ETH.localIP().toString().c_str());
}

void WebServerManager::update()
{
    server.handleClient();
}

void WebServerManager::handleSaveSettings()
{
    String oldToken = config.getBotToken();
    String oldApiKey = config.getHubApiKey();

    config.setDeviceName(server.arg("deviceName"));
    config.setTimezone(server.arg("timezone"));
    config.setNtpServer(server.arg("ntpServer"));

    // Empty masked fields mean "leave the existing secret unchanged".
    String botToken = server.arg("botToken");
    String hubApiKey = server.arg("hubApiKey");
    botToken.trim();
    hubApiKey.trim();

    if (botToken.length()) config.setBotToken(botToken);
    if (hubApiKey.length()) config.setHubApiKey(hubApiKey);

    config.setStartupMessageEnabled(server.hasArg("startupMessageEnabled"));
    config.setStartupMessage(server.arg("startupMessage"));
    config.setGpio35Enabled(server.hasArg("gpio35Enabled"));
    config.setGpio35HighMessage(server.arg("gpio35HighMessage"));
    config.setGpio35LowMessage(server.arg("gpio35LowMessage"));
    config.setGpio39Enabled(server.hasArg("gpio39Enabled"));
    config.setGpio39HighMessage(server.arg("gpio39HighMessage"));
    config.setGpio39LowMessage(server.arg("gpio39LowMessage"));

    if (!config.save())
    {
        sendJson(server, 500, false, "Save failed");
        return;
    }

    if (config.getBotToken() != oldToken && !telegram.reloadToken())
    {
        sendJson(server, 500, false, "Settings saved, but Telegram token reload failed");
        return;
    }

    sendJson(server, 200, true,
             config.getHubApiKey() != oldApiKey ? "Settings and API key updated" : "Settings saved");
}

void WebServerManager::handleAddChat()
{
    String id = server.arg("id");
    String name = server.arg("name");
    id.trim();
    name.trim();

    if (!id.length() || !name.length() || !config.addChat(id, name) || !config.save())
    {
        sendJson(server, 400, false, "Failed to add chat");
        return;
    }

    sendJson(server, 200, true, "Chat added");
}

void WebServerManager::handleUpdateChat()
{
    size_t i = (size_t)server.arg("index").toInt();
    String id = server.arg("id");
    String name = server.arg("name");
    id.trim();
    name.trim();

    if (!id.length() || !name.length() || i >= config.getChatCount() ||
        !config.updateChat(i, id, name) || !config.save())
    {
        sendJson(server, 400, false, "Failed to update chat");
        return;
    }

    sendJson(server, 200, true, "Chat updated");
}

void WebServerManager::handleDeleteChat()
{
    size_t i = (size_t)server.arg("index").toInt();

    if (i >= config.getChatCount() || !config.deleteChat(i) || !config.save())
    {
        sendJson(server, 400, false, "Failed to delete chat");
        return;
    }

    sendJson(server, 200, true, "Chat deleted");
}

void WebServerManager::handleAddReminder()
{
    Reminder r;
    if (!readReminder(server, r, false))
    {
        sendJson(server, 400, false, "Invalid reminder");
        return;
    }

    if (!reminderManager.addReminder(r))
    {
        sendJson(server, 500, false, "Failed to add reminder");
        return;
    }

    sendJson(server, 200, true, "Reminder added");
}

void WebServerManager::handleUpdateReminder()
{
    uint32_t id = (uint32_t)server.arg("id").toInt();
    Reminder r;

    if (!id || !reminderManager.getReminderById(id, r) || !readReminder(server, r, true))
    {
        sendJson(server, 400, false, "Invalid reminder");
        return;
    }

    if (!reminderManager.updateReminder(r))
    {
        sendJson(server, 500, false, "Failed to update reminder");
        return;
    }

    sendJson(server, 200, true, "Reminder updated");
}

void WebServerManager::handleCloneReminder()
{
    uint32_t id = (uint32_t)server.arg("id").toInt();
    Reminder r;

    if (!id || !reminderManager.getReminderById(id, r))
    {
        sendJson(server, 404, false, "Reminder not found");
        return;
    }

    r.id = 0;
    r.lastTriggered = 0;
    r.nextTrigger = 0;

    if (!reminderManager.addReminder(r))
    {
        sendJson(server, 500, false, "Failed to clone reminder");
        return;
    }

    sendJson(server, 200, true, "Reminder cloned");
}

void WebServerManager::handleDeleteReminder()
{
    uint32_t id = (uint32_t)server.arg("id").toInt();

    if (!id || !reminderManager.deleteReminder(id))
    {
        sendJson(server, 404, false, "Reminder not found");
        return;
    }

    sendJson(server, 200, true, "Reminder deleted");
}

void WebServerManager::handleToggleReminder()
{
    uint32_t id = (uint32_t)server.arg("id").toInt();
    bool enabled = server.arg("enabled") == "1" || server.arg("enabled") == "true";

    if (!id || !reminderManager.setEnabled(id, enabled))
    {
        sendJson(server, 400, false, "Failed to update reminder");
        return;
    }

    sendJson(server, 200, true, "Reminder updated");
}

void WebServerManager::handleHubTelegramSend()
{
    String apiKey = server.arg("apiKey");
    if (!apiKey.length()) apiKey = server.header("X-API-Key");

    String chatId = server.arg("chatId");
    String text = server.arg("text");
    bool silent = server.arg("silent") == "1" || server.arg("silent") == "true";

    if (!apiKey.length() || apiKey != config.getHubApiKey())
    {
        sendJson(server, 403, false, "Invalid API key");
        return;
    }

    if (!config.hasChat(chatId))
    {
        sendJson(server, 400, false, "Unknown chat");
        return;
    }

    if (!text.length())
    {
        sendJson(server, 400, false, "Message text is empty");
        return;
    }

    if (!telegram.sendMessage(chatId, text, silent))
    {
        sendJson(server, 502, false, "Telegram send failed");
        return;
    }

    sendJson(server, 200, true, "Message sent");
}

void WebServerManager::handleNotFound()
{
    sendJson(server, 404, false, "Not found");
}
