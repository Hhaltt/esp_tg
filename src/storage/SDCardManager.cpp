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
    ;


    SPI.begin(
        SD_SCK,
        SD_MISO,
        SD_MOSI,
        SD_CS
    );


    if (!SD.begin(SD_CS))
    {
        ;

        available = false;

        return;
    }


    uint8_t cardType = SD.cardType();


    if (cardType == CARD_NONE)
    {
        ;

        available = false;

        return;
    }


    available = true;


    ;


    ;

    ;

    ;


    ;

    ;

    ;


    ;

    ;

    ;
}


// ============================================================
// STATUS
// ============================================================

bool SDCardManager::isAvailable()
{
    return available;
}