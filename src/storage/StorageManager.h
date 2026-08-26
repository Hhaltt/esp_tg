#pragma once

#include <Arduino.h>

#include "../core/Statistics.h"


class StorageManager
{
public:

    bool begin();


    // --------------------------------------------------------
    // Statistics
    // --------------------------------------------------------

    bool loadStatistics(
        StatisticsData& data
    );

    bool saveStatistics(
        const StatisticsData& data
    );


private:

    bool ready = false;


    // --------------------------------------------------------
    // Internal
    // --------------------------------------------------------

    bool ensureDirectory(
        const char* path
    );

    bool ensureFile(
        const char* path,
        const char* defaultContent
    );


    // --------------------------------------------------------
    // Statistics slots
    // --------------------------------------------------------

    bool readStatisticsSlot(
        const char* path,
        StatisticsData& data,
        uint32_t& sequence
    );

    bool writeStatisticsSlot(
        const char* path,
        const StatisticsData& data,
        uint32_t sequence
    );

    uint32_t calculateCRC32(
        const uint8_t* data,
        size_t length
    );


    // Який слот зараз останній валідний

    char activeSlot = 0;

    uint32_t activeSequence = 0;
};


extern StorageManager storage;