#include <Arduino.h>

#include <esp_task_wdt.h>
#include <esp_idf_version.h>

#include "core/Config.h"
#include "core/Statistics.h"
#include "core/ConfigManager.h"

#include "network/EthernetManager.h"
#include "network/WebServerManager.h"
#include "network/TelegramManager.h"

#include "services/TimeManager.h"
#include "services/ReminderManager.h"
#include "services/GpioMonitorManager.h"

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

    if (result == ESP_OK)
        Serial.println("[WDT] Task watchdog initialized");
    else if (result == ESP_ERR_INVALID_STATE)
        Serial.println("[WDT] Task watchdog already initialized");
    else
        Serial.printf("[WDT] Init error: %d\n", (int)result);

    result = esp_task_wdt_add(NULL);

    if (result == ESP_OK)
        Serial.printf("[WDT] Main loop watched, timeout: %lu s\n", (unsigned long)WATCHDOG_TIMEOUT_SECONDS);
    else if (result != ESP_ERR_INVALID_STATE)
        Serial.printf("[WDT] Add task error: %d\n", (int)result);
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
    gpioMonitor.begin();

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

    if (
        sdCard.isAvailable()
        && millis() - lastStatisticsSave >= STATISTICS_SAVE_INTERVAL
    )
    {
        lastStatisticsSave = millis();
        Serial.println("[SYSTEM] Statistics checkpoint");
        storage.saveStatistics(statistics.getData());
    }

    esp_task_wdt_reset();
    delay(1);
}
