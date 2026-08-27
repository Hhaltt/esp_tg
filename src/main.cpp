#include <Arduino.h>


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


// ============================================================
// STATISTICS CHECKPOINT
// ============================================================

unsigned long lastStatisticsSave = 0;


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(2000);


    Serial.println();
    Serial.println();

    Serial.println(
        "========================================"
    );

    Serial.println(
        DEVICE_NAME
    );

    Serial.println(
        "========================================"
    );


    // --------------------------------------------------------
    // Statistics
    // --------------------------------------------------------

    statistics.begin();


    // --------------------------------------------------------
    // Ethernet
    // --------------------------------------------------------

    ethernet.begin();


    // --------------------------------------------------------
    // SD CARD
    // --------------------------------------------------------

    sdCard.begin();


    // --------------------------------------------------------
    // STORAGE
    // --------------------------------------------------------

    if (storage.begin())
    {
        StatisticsData loadedData;


        storage.loadStatistics(
            loadedData
        );


        statistics.load(
            loadedData
        );


        // Цей запуск вже рахується

        statistics.onBoot();


        // Одразу зберігаємо boot count.
        //
        // Це всього один запис на запуск.

        storage.saveStatistics(
            statistics.getData()
        );
    }
    else
    {
        Serial.println(
            "[SYSTEM] Storage unavailable"
        );

        // Emily продовжує працювати
        // навіть без SD.
    }


    // --------------------------------------------------------
    // CONFIG
    // --------------------------------------------------------

    config.begin();


    // --------------------------------------------------------
    // reminder
    // --------------------------------------------------------

    reminderManager.begin();


    // --------------------------------------------------------
    // Time
    // --------------------------------------------------------

    timeManager.begin();


    // --------------------------------------------------------
    // Telegram
    // --------------------------------------------------------

    telegram.begin();


    // --------------------------------------------------------
    // WEB
    // --------------------------------------------------------

    webServerManager.begin();


    // --------------------------------------------------------
    // GPIO MONITOR
    // --------------------------------------------------------

    gpioMonitor.begin();


    // --------------------------------------------------------
    // RX / TX LEDS
    // --------------------------------------------------------

    ledActivity.begin();


    Serial.println(
        "[SYSTEM] Initialization complete"
    );
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    ethernet.update();

    timeManager.update();

    telegram.update();

    gpioMonitor.update();

    webServerManager.update();

    statistics.update();

    reminderManager.update();

    ledActivity.update();


    // --------------------------------------------------------
    // STATISTICS CHECKPOINT
    // --------------------------------------------------------

    if (
        sdCard.isAvailable()
        &&
        millis() - lastStatisticsSave
        >= STATISTICS_SAVE_INTERVAL
    )
    {
        lastStatisticsSave =
            millis();


        Serial.println(
            "[SYSTEM] Statistics checkpoint"
        );


        storage.saveStatistics(
            statistics.getData()
        );
    }


    delay(1);
}