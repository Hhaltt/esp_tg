#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_idf_version.h>
#include <SD.h>
#include <ETH.h>
#include "core/Config.h"
#include "core/Statistics.h"
#include "core/ConfigManager.h"
#include "network/EthernetManager.h"
#include "network/WebServerManager.h"
#include "network/TelegramManager.h"
#include "services/TimeManager.h"
#include "services/ReminderManager.h"
#include "services/GpioMonitorManager.h"
#include "services/LedActivityManager.h"
#include "storage/SDCardManager.h"
#include "storage/StorageManager.h"

unsigned long lastStatisticsSave = 0;
static const uint32_t WATCHDOG_TIMEOUT_SECONDS = 15;

static String esc(String v)
{
    v.replace("\\", "\\\\");
    v.replace("\"", "\\\"");
    v.replace("\n", "\\n");
    v.replace("\r", "\\r");
    return v;
}

static void beginWatchdog()
{
#if ESP_IDF_VERSION_MAJOR >= 5
    esp_task_wdt_config_t w = {};
    w.timeout_ms = WATCHDOG_TIMEOUT_SECONDS * 1000;
    w.idle_core_mask = 0;
    w.trigger_panic = true;
    esp_task_wdt_init(&w);
#else
    esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true);
#endif
    esp_task_wdt_add(NULL);
    Serial.printf("[WDT] Main loop watched, timeout: %lu s\n", (unsigned long)WATCHDOG_TIMEOUT_SECONDS);
}

static void streamSd(WebServer& s, const char* path, const char* type)
{
    if (!sdCard.isAvailable() || !SD.exists(path))
    {
        s.send(404, "text/plain", String("Missing ") + path);
        return;
    }

    File f = SD.open(path, FILE_READ);
    s.streamFile(f, type);
    f.close();
}

static String statusJson()
{
    String j = "{";
    j += "\"deviceName\":\"" + esc(config.getDeviceName()) + "\",";
    j += "\"network\":{\"connected\":" + String(ETH.linkUp() ? "true" : "false");
    j += ",\"ip\":\"" + ETH.localIP().toString() + "\"";
    j += ",\"gateway\":\"" + ETH.gatewayIP().toString() + "\"";
    j += ",\"speed\":" + String(ETH.linkSpeed());
    j += ",\"fullDuplex\":" + String(ETH.fullDuplex() ? "true" : "false") + "},";
    j += "\"storage\":{\"available\":" + String(sdCard.isAvailable() ? "true" : "false");
    j += ",\"size\":" + String(sdCard.isAvailable() ? SD.cardSize() : 0);
    j += ",\"used\":" + String(sdCard.isAvailable() ? SD.usedBytes() : 0) + "},";
    j += "\"system\":{\"time\":\"" + esc(timeManager.getDateTimeString()) + "\"";
    j += ",\"totalUptime\":\"" + statistics.getTotalUptimeString() + "\"";
    j += ",\"currentUptime\":\"" + statistics.getCurrentUptimeString() + "\"";
    j += ",\"bootCount\":" + String(statistics.getBootCount());
    j += ",\"errors\":" + String(statistics.getErrors()) + "},";
    j += "\"telegram\":{\"messagesReceived\":" + String(statistics.getMessagesReceived());
    j += ",\"messagesSent\":" + String(statistics.getMessagesSent()) + "},";
    j += "\"commands\":{\"received\":" + String(statistics.getCommandsReceived());
    j += ",\"executed\":" + String(statistics.getCommandsExecuted());
    j += ",\"errors\":" + String(statistics.getCommandErrors()) + "},";
    j += "\"chatCount\":" + String(config.getChatCount()) + ",";
    j += "\"reminderCount\":" + String(reminderManager.getCount()) + ",";
    j += "\"commandRouteCount\":" + String(config.getCommandRouteCount());
    j += "}";
    return j;
}

static String settingsJson()
{
    String j = "{\"deviceName\":\"" + esc(config.getDeviceName()) + "\",\"timezone\":\"" + esc(config.getTimezone()) + "\",\"ntpServer\":\"" + esc(config.getNtpServer()) + "\",\"startupMessageEnabled\":" + String(config.isStartupMessageEnabled() ? "true" : "false") + ",\"startupMessage\":\"" + esc(config.getStartupMessage()) + "\",\"chats\":[";
    for (size_t i = 0; i < config.getChatCount(); i++)
    {
        if (i) j += ",";
        auto c = config.getChat(i);
        j += "{\"index\":" + String(i) + ",\"id\":\"" + esc(c.id) + "\",\"name\":\"" + esc(c.name) + "\"}";
    }
    return j + "]}";
}

static String routesJson()
{
    String j = "{\"items\":[";
    for (size_t i = 0; i < config.getCommandRouteCount(); i++)
    {
        if (i) j += ",";
        auto r = config.getCommandRoute(i);
        j += "{\"index\":" + String(i) + ",\"phrase\":\"" + esc(r.phrase) + "\",\"url\":\"" + esc(r.url) + "\",\"command\":\"" + esc(r.command) + "\"}";
    }
    return j + "]}";
}

static String chatsJson()
{
    String j = "{\"items\":[";
    for (size_t i = 0; i < config.getChatCount(); i++)
    {
        if (i) j += ",";
        auto c = config.getChat(i);
        j += "{\"id\":\"" + esc(c.id) + "\",\"name\":\"" + esc(c.name) + "\"}";
    }
    return j + "]}";
}

static String remindersJson()
{
    String j = "{\"count\":" + String(reminderManager.getCount()) + ",\"items\":[";
    for (size_t i = 0; i < reminderManager.getCount(); i++)
    {
        if (i) j += ",";
        auto r = reminderManager.getReminder(i);
        j += "{\"id\":" + String(r.id) + ",\"message\":\"" + esc(r.message) + "\",\"chatId\":\"" + esc(r.chatId) + "\",\"type\":" + String((int)r.type) + ",\"year\":" + String(r.year) + ",\"month\":" + String(r.month) + ",\"day\":" + String(r.day) + ",\"weekday\":" + String(r.weekday) + ",\"hour\":" + String(r.hour) + ",\"minute\":" + String(r.minute) + ",\"enabled\":" + String(r.enabled ? "true" : "false") + ",\"silent\":" + String(r.silent ? "true" : "false") + "}";
    }
    return j + "]}";
}

static void setupSd()
{
    WebServer& s = webServerManager.rawServer();

    s.on("/sd", HTTP_GET, [&s]() { s.sendHeader("Location", "/sd/"); s.send(303); });
    s.on("/sd/", HTTP_GET, [&s]() { streamSd(s, "/www/index.html", "text/html; charset=utf-8"); });
    s.on("/sd/index.html", HTTP_GET, [&s]() { streamSd(s, "/www/index.html", "text/html; charset=utf-8"); });
    s.on("/sd/settings.html", HTTP_GET, [&s]() { streamSd(s, "/www/settings.html", "text/html; charset=utf-8"); });
    s.on("/sd/reminders.html", HTTP_GET, [&s]() { streamSd(s, "/www/reminders.html", "text/html; charset=utf-8"); });
    s.on("/sd/commands.html", HTTP_GET, [&s]() { streamSd(s, "/www/commands.html", "text/html; charset=utf-8"); });
    s.on("/sd/style.css", HTTP_GET, [&s]() { streamSd(s, "/www/style.css", "text/css; charset=utf-8"); });
    s.on("/sd/app.js", HTTP_GET, [&s]() { streamSd(s, "/www/app.js", "application/javascript; charset=utf-8"); });

    s.on("/api/status", HTTP_GET, [&s]() { s.send(200, "application/json", statusJson()); });
    s.on("/api/settings", HTTP_GET, [&s]() { s.send(200, "application/json", settingsJson()); });
    s.on("/api/reminders", HTTP_GET, [&s]() { s.send(200, "application/json", remindersJson()); });
    s.on("/api/commands", HTTP_GET, [&s]() { s.send(200, "application/json", routesJson()); });
    s.on("/api/v1/status", HTTP_GET, [&s]() { s.send(200, "application/json", statusJson()); });
    s.on("/api/v1/chats", HTTP_GET, [&s]() { s.send(200, "application/json", chatsJson()); });
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    beginWatchdog();
    statistics.begin();
    ethernet.begin();
    sdCard.begin();

    if (storage.begin())
    {
        StatisticsData d;
        storage.loadStatistics(d);
        statistics.load(d);
        statistics.onBoot();
        storage.saveStatistics(statistics.getData());
    }

    config.begin();
    reminderManager.begin();
    timeManager.begin();
    telegram.begin();
    setupSd();
    webServerManager.begin();
    gpioMonitor.begin();
    ledActivity.begin();
    Serial.println("[SYSTEM] Initialization complete");
}

void loop()
{
    esp_task_wdt_reset();
    ethernet.update();
    timeManager.update();
    telegram.update();
    gpioMonitor.update();
    webServerManager.update();
    statistics.update();
    reminderManager.update();
    ledActivity.update();

    if (sdCard.isAvailable() && millis() - lastStatisticsSave >= STATISTICS_SAVE_INTERVAL)
    {
        lastStatisticsSave = millis();
        storage.saveStatistics(statistics.getData());
    }

    esp_task_wdt_reset();
    delay(1);
}
