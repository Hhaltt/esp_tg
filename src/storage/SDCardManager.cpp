#include "SDCardManager.h"

#include <SPI.h>
#include <SD.h>


// ============================================================
// SPI PINS
// ============================================================

#define SD_SCK   14
#define SD_MOSI  17
#define SD_MISO  33
#define SD_CS    32

SDCardManager sdCard;


// ============================================================
// BEGIN
// ============================================================

void SDCardManager::begin()
{
    Serial.println("[SD] Initializing...");


    SPI.begin(
        SD_SCK,
        SD_MISO,
        SD_MOSI,
        SD_CS
    );


    if (!SD.begin(SD_CS))
    {
        Serial.println("[SD] Card mount FAILED");

        available = false;

        return;
    }


    uint8_t cardType = SD.cardType();


    if (cardType == CARD_NONE)
    {
        Serial.println("[SD] No card attached");

        available = false;

        return;
    }


    available = true;


    Serial.println("[SD] Card mounted successfully");


    Serial.print("[SD] Size: ");

    Serial.print(
        SD.cardSize() / (1024 * 1024)
    );

    Serial.println(" MB");


    Serial.print("[SD] Used: ");

    Serial.print(
        SD.usedBytes() / 1024
    );

    Serial.println(" KB");


    Serial.print("[SD] Total: ");

    Serial.print(
        SD.totalBytes() / (1024 * 1024)
    );

    Serial.println(" MB");
}


// ============================================================
// STATUS
// ============================================================

bool SDCardManager::isAvailable()
{
    return available;
}