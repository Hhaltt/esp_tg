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

static void beginWatchdog()
{
#if ESP_IDF_VERSION_MAJOR >= 5
    esp_task_wdt_config_t configWdt = {};
    configWdt.timeout_ms = WATCHDOG_TIMEOUT_SECONDS * 1000;
    configWdt.idle_core_mask = 0;
    configWdt.trigger_panic = true;
    esp_err_t result = esp_task_wdt_init(&configWdt);
#else
    esp_err_t result = esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true);
#endif
    if (result == ESP_OK) Serial.println("[WDT] Task watchdog initialized");
    else if (result == ESP_ERR_INVALID_STATE) Serial.println("[WDT] Task watchdog already initialized");
    else Serial.printf("[WDT] Init error: %d\n", (int)result);

    result = esp_task_wdt_add(NULL);
    if (result == ESP_OK) Serial.printf("[WDT] Main loop watched, timeout: %lu s\n", (unsigned long)WATCHDOG_TIMEOUT_SECONDS);
    else if (result != ESP_ERR_INVALID_STATE) Serial.printf("[WDT] Add task error: %d\n", (int)result);
}

static void setupSdWebExperiment()
{
    WebServer& server = webServerManager.rawServer();

    // URL /sd/ is only a web prefix. The physical SD path starts at /www/.
    // Do not use serveStatic here: it appends the URL path to the SD path.
    server.on("/sd/", HTTP_GET, [&server]()
    {
        if (!sdCard.isAvailable())
        {
            server.send(503, "text/plain", "SD card unavailable");
            return;
        }

        if (!SD.exists("/www/index.html"))
        {
            Serial.println("[WEB] Missing SD frontend: /www/index.html");
            server.send(404, "text/plain", "Missing /www/index.html on SD card");
            return;
        }

        File file = SD.open("/www/index.html", FILE_READ);
        server.streamFile(file, "text/html; charset=utf-8");
        file.close();
    });

    server.on("/sd/style.css", HTTP_GET, [&server]()
    {
        if (!sdCard.isAvailable() || !SD.exists("/www/style.css"))
        {
            server.send(404, "text/plain", "Missing /www/style.css on SD card");
            return;
        }

        File file = SD.open("/www/style.css", FILE_READ);
        server.streamFile(file, "text/css; charset=utf-8");
        file.close();
    });

    server.on("/sd/app.js", HTTP_GET, [&server]()
    {
        if (!sdCard.isAvailable() || !SD.exists("/www/app.js"))
        {
            server.send(404, "text/plain", "Missing /www/app.js on SD card");
            return;
        }

        File file = SD.open("/www/app.js", FILE_READ);
        server.streamFile(file, "application/javascript; charset=utf-8");
        file.close();
    });

    server.on("/sd", HTTP_GET, [&server]()
    {
        server.sendHeader("Location", "/sd/");
        server.send(303);
    });

    server.on("/api/status", HTTP_GET, [&server]()
    {
        uint64_t total = 0;
        uint64_t used = 0;

        if (sdCard.isAvailable())
        {
            total = SD.totalBytes();
            used = SD.usedBytes();
        }

        String json = "{";
        json += "\"deviceName\":\"" + config.getDeviceName() + "\",";
        json += "\"linkUp\":" + String(ETH.linkUp() ? "true" : "false") + ",";
        json += "\"ip\":\"" + ETH.localIP().toString() + "\",";
        json += "\"gateway\":\"" + ETH.gatewayIP().toString() + "\",";
        json += "\"linkSpeed\":" + String(ETH.linkSpeed()) + ",";
        json += "\"fullDuplex\":" + String(ETH.fullDuplex() ? "true" : "false") + ",";
        json += "\"time\":\"" + timeManager.getDateTimeString() + "\",";
        json += "\"totalUptime\":\"" + statistics.getTotalUptimeString() + "\",";
        json += "\"currentUptime\":\"" + statistics.getCurrentUptimeString() + "\",";
        json += "\"bootCount\":" + String(statistics.getBootCount()) + ",";
        json += "\"errors\":" + String(statistics.getErrors()) + ",";
        json += "\"messagesReceived\":" + String(statistics.getMessagesReceived()) + ",";
        json += "\"messagesSent\":" + String(statistics.getMessagesSent()) + ",";
        json += "\"commandsReceived\":" + String(statistics.getCommandsReceived()) + ",";
        json += "\"commandsExecuted\":" + String(statistics.getCommandsExecuted()) + ",";
        json += "\"commandErrors\":" + String(statistics.getCommandErrors()) + ",";
        json += "\"chatCount\":" + String(config.getChatCount()) + ",";
        json += "\"reminderCount\":" + String(reminderManager.getCount()) + ",";
        json += "\"sdAvailable\":" + String(sdCard.isAvailable() ? "true" : "false") + ",";
        json += "\"sdTotal\":" + String((uint32_t)total) + ",";
        json += "\"sdUsed\":" + String((uint32_t)used);
        json += "}";

        server.send(200, "application/json; charset=utf-8", json);
    });

    Serial.println("[WEB] SD frontend experiment: /sd/");
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println();
    Serial.println("========================================");
    Serial.println(DEVICE_NAME);
    Serial.println("========================================");

    beginWatchdog();
    statistics.begin();
    ethernet.begin();
    sdCard.begin();

    if (storage.begin())
    {
        StatisticsData loadedData;
        storage.loadStatistics(loadedData);
        statistics.load(loadedData);
        statistics.onBoot();
        storage.saveStatistics(statistics.getData());
    }
    else
    {
        Serial.println("[SYSTEM] Storage unavailable");
    }

    config.begin();
    reminderManager.begin();
    timeManager.begin();
    telegram.begin();
    webServerManager.begin();
    setupSdWebExperiment();
    gpioMonitor.begin();
    ledActivity.begin();

    Serial.println("[SYSTEM] Initialization complete");
    esp_task_wdt_reset();
}

void loop()
{
    esp_task_wdt_reset();
    ethernet.update();
    esp_task_wdt_reset();

    timeManager.update();
    telegram.update();
    esp_task_wdt_reset();

    gpioMonitor.update();
    webServerManager.update();
    statistics.update();
    reminderManager.update();
    ledActivity.update();

    if (sdCard.isAvailable() && millis() - lastStatisticsSave >= STATISTICS_SAVE_INTERVAL)
    {
        lastStatisticsSave = millis();
        Serial.println("[SYSTEM] Statistics checkpoint");
        storage.saveStatistics(statistics.getData());
    }

    esp_task_wdt_reset();
    delay(1);
}
