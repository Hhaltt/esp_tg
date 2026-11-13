#include "StorageManager.h"

#include <SD.h>
#include <string.h>

#include <ArduinoJson.h>

#include "SDCardManager.h"


// ============================================================
// FILES
// ============================================================

static const char* DATA_DIR =
    "/data";

static const char* CONFIG_FILE =
    "/data/config.json";

static const char* REMINDERS_FILE =
    "/data/reminders.json";

static const char* STATS_A_FILE =
    "/data/statistics_a.dat";

static const char* STATS_B_FILE =
    "/data/statistics_b.dat";


// ============================================================
// STATISTICS FILE FORMAT
// ============================================================

// Простий header:
//
// MAGIC
// VERSION
// SEQUENCE
// DATA
// CRC32

static const uint32_t STATS_MAGIC =
    0x454D4C59;  // "EMLY"

static const uint16_t STATS_VERSION =
    1;


struct __attribute__((packed)) StatisticsFile
{
    uint32_t magic;

    uint16_t version;

    uint16_t reserved;

    uint32_t sequence;

    StatisticsData data;

    uint32_t crc;
};


// ============================================================
// GLOBAL OBJECT
// ============================================================

StorageManager storage;


// ============================================================
// BEGIN
// ============================================================

bool StorageManager::begin()
{
    ;


    if (!sdCard.isAvailable())
    {
        ;

        return false;
    }


    // --------------------------------------------------------
    // DATA DIRECTORY
    // --------------------------------------------------------

    if (!ensureDirectory(DATA_DIR))
    {
        ;

        return false;
    }


    // --------------------------------------------------------
    // DEFAULT CONFIG
    // --------------------------------------------------------

    const char* defaultConfig =
    "{\n"
    "  \"system\": {\n"
    "    \"deviceName\": \"Emily\",\n"
    "    \"timezone\": \"Europe/Kyiv\"\n"
    "  },\n"
    "\n"
    "  \"weather\": {\n"
    "    \"enabled\": false,\n"
    "    \"apiKey\": \"\",\n"
    "    \"city\": \"\",\n"
    "    \"hour\": 7,\n"
    "    \"minute\": 0\n"
    "  }\n"
    "}\n";


    if (
        !ensureFile(
            CONFIG_FILE,
            defaultConfig
        )
    )
    {
        return false;
    }


    // --------------------------------------------------------
    // DEFAULT REMINDERS
    // --------------------------------------------------------

    if (
        !ensureFile(
            REMINDERS_FILE,
            "{\n"
"  \"nextId\": 1,\n"
"  \"reminders\": []\n"
"}\n"
        )
    )
    {
        return false;
    }


    ready = true;


    ;

    return true;
}


// ============================================================
// ENSURE DIRECTORY
// ============================================================

bool StorageManager::ensureDirectory(
    const char* path
)
{
    // Спочатку перевіряємо, чи вже існує директорія.
    // Не викликаємо SD.open() для неіснуючого шляху.

    if (SD.exists(path))
    {
        File dir = SD.open(path, FILE_READ);

        if (!dir)
        {
            ;

            return false;
        }

        bool isDirectory =
            dir.isDirectory();

        dir.close();

        if (!isDirectory)
        {
            ;

            return false;
        }

        ;

        return true;
    }


    // Директорії немає — створюємо.

    if (!SD.mkdir(path))
    {
        ;

        return false;
    }


    ;

    return true;
}


// ============================================================
// ENSURE FILE
// ============================================================

bool StorageManager::ensureFile(
    const char* path,
    const char* defaultContent
)
{
    if (SD.exists(path))
    {
        ;

        return true;
    }


    File file = SD.open(
        path,
        FILE_WRITE
    );


    if (!file)
    {
        ;

        return false;
    }


    size_t expected = strlen(defaultContent);
    size_t written = file.print(defaultContent);

    file.flush();
    file.close();

    if (written != expected)
    {
        SD.remove(path);
        ;
        return false;
    }


    ;

    return true;
}


// ============================================================
// LOAD STATISTICS
// ============================================================

bool StorageManager::loadStatistics(
    StatisticsData& result
)
{
    memset(
        &result,
        0,
        sizeof(result)
    );


    if (!ready)
    {
        ;

        return false;
    }


    StatisticsData dataA = {};
    StatisticsData dataB = {};

    uint32_t sequenceA = 0;
    uint32_t sequenceB = 0;


    bool validA =
        readStatisticsSlot(
            STATS_A_FILE,
            dataA,
            sequenceA
        );


    bool validB =
        readStatisticsSlot(
            STATS_B_FILE,
            dataB,
            sequenceB
        );


    ;


    ;

    if (validA)
    {
        ;
    }

    ;


    ;

    if (validB)
    {
        ;
    }

    ;


    // --------------------------------------------------------
    // No valid statistics
    // --------------------------------------------------------

    if (!validA && !validB)
    {
        ;

        ;


        activeSlot = 0;

        activeSequence = 0;


        // Створюємо перший checkpoint.
        //
        // Після цього:
        //
        // A = sequence 1

        return saveStatistics(result);
    }


    // --------------------------------------------------------
    // Only A
    // --------------------------------------------------------

    if (validA && !validB)
    {
        result = dataA;

        activeSlot = 'A';

        activeSequence = sequenceA;

        ;

        return true;
    }


    // --------------------------------------------------------
    // Only B
    // --------------------------------------------------------

    if (!validA && validB)
    {
        result = dataB;

        activeSlot = 'B';

        activeSequence = sequenceB;

        ;

        return true;
    }


    // --------------------------------------------------------
    // Both valid
    // --------------------------------------------------------

    if (sequenceA >= sequenceB)
    {
        result = dataA;

        activeSlot = 'A';

        activeSequence = sequenceA;

        ;
    }
    else
    {
        result = dataB;

        activeSlot = 'B';

        activeSequence = sequenceB;

        ;
    }


    return true;
}


// ============================================================
// SAVE STATISTICS
// ============================================================

bool StorageManager::saveStatistics(
    const StatisticsData& data
)
{
    if (!ready)
    {
        return false;
    }


    uint32_t newSequence =
        activeSequence + 1;


    // --------------------------------------------------------
    // Чергуємо A / B
    // --------------------------------------------------------

    const char* targetFile;

    char newActiveSlot;


    if (activeSlot == 'A')
    {
        targetFile = STATS_B_FILE;

        newActiveSlot = 'B';
    }
    else
    {
        targetFile = STATS_A_FILE;

        newActiveSlot = 'A';
    }


    ;


    bool success =
        writeStatisticsSlot(
            targetFile,
            data,
            newSequence
        );


    if (!success)
    {
        ;

        return false;
    }


    // Тільки ПІСЛЯ успішного запису
    // переключаємо активний слот.

    activeSlot =
        newActiveSlot;

    activeSequence =
        newSequence;


    ;

    return true;
}


// ============================================================
// READ STATISTICS SLOT
// ============================================================

bool StorageManager::readStatisticsSlot(
    const char* path,
    StatisticsData& data,
    uint32_t& sequence
)
{
    if (!SD.exists(path))
    {
        return false;
    }


    File file = SD.open(
        path,
        FILE_READ
    );


    if (!file)
    {
        return false;
    }


    // Файл повинен мати точно потрібний розмір

    if (
        file.size()
        != sizeof(StatisticsFile)
    )
    {
        ;

        file.close();

        return false;
    }


    StatisticsFile record;


    size_t bytesRead =
        file.read(
            (uint8_t*)&record,
            sizeof(record)
        );


    file.close();


    if (bytesRead != sizeof(record))
    {
        return false;
    }


    // --------------------------------------------------------
    // MAGIC
    // --------------------------------------------------------

    if (
        record.magic
        != STATS_MAGIC
    )
    {
        ;

        return false;
    }


    // --------------------------------------------------------
    // VERSION
    // --------------------------------------------------------

    if (
        record.version
        != STATS_VERSION
    )
    {
        ;

        return false;
    }


    // --------------------------------------------------------
    // CRC
    // --------------------------------------------------------

    uint32_t calculatedCRC =
        calculateCRC32(
            (const uint8_t*)&record,
            offsetof(
                StatisticsFile,
                crc
            )
        );


    if (
        calculatedCRC
        != record.crc
    )
    {
        ;

        return false;
    }


    data =
        record.data;

    sequence =
        record.sequence;


    return true;
}


// ============================================================
// WRITE STATISTICS SLOT
// ============================================================

bool StorageManager::writeStatisticsSlot(
    const char* path,
    const StatisticsData& data,
    uint32_t sequence
)
{
    StatisticsFile record = {};


    record.magic =
        STATS_MAGIC;

    record.version =
        STATS_VERSION;

    record.reserved =
        0;

    record.sequence =
        sequence;

    record.data =
        data;


    record.crc =
        calculateCRC32(
            (const uint8_t*)&record,
            offsetof(
                StatisticsFile,
                crc
            )
        );


    // --------------------------------------------------------
    // Спочатку пишемо тимчасовий файл
    // --------------------------------------------------------

    String tempPath =
        String(path) + ".tmp";


    if (SD.exists(tempPath.c_str()))
{
    SD.remove(
        tempPath.c_str()
    );
}


    File file =
        SD.open(
            tempPath.c_str(),
            FILE_WRITE
        );


    if (!file)
    {
        ;

        return false;
    }


    size_t written =
        file.write(
            (const uint8_t*)&record,
            sizeof(record)
        );


    file.flush();

    file.close();


    if (
        written
        != sizeof(record)
    )
    {
        ;

        if (SD.exists(tempPath.c_str()))
{
    SD.remove(
        tempPath.c_str()
    );
}

        return false;
    }


    // --------------------------------------------------------
    // Перевіряємо записаний файл
    // --------------------------------------------------------

    StatisticsData verifyData;

    uint32_t verifySequence;


    if (
        !readStatisticsSlot(
            tempPath.c_str(),
            verifyData,
            verifySequence
        )
    )
    {
        ;

    if (SD.exists(tempPath.c_str()))
{
    SD.remove(
        tempPath.c_str()
    );
}

        return false;
    }


    // --------------------------------------------------------
    // Тільки після перевірки
    // замінюємо основний файл
    // --------------------------------------------------------

   if (SD.exists(path))
{
    if (!SD.remove(path))
    {
        ;

        return false;
    }
}


if (
    !SD.rename(
        tempPath.c_str(),
        path
    )
)
{
    ;

    return false;
}

    return true;
}


// ============================================================
// CRC32
// ============================================================

uint32_t StorageManager::calculateCRC32(
    const uint8_t* buffer,
    size_t length
)
{
    uint32_t crc =
        0xFFFFFFFF;


    while (length--)
    {
        crc ^=
            *buffer++;


        for (
            uint8_t i = 0;
            i < 8;
            i++
        )
        {
            if (crc & 1)
            {
                crc =
                    (crc >> 1)
                    ^
                    0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }


    return ~crc;
}