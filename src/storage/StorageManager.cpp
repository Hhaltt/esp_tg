#include "StorageManager.h"

#include <SD.h>

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
    Serial.println(
        "[STORAGE] Initializing..."
    );


    if (!sdCard.isAvailable())
    {
        Serial.println(
            "[STORAGE] SD card unavailable"
        );

        return false;
    }


    // --------------------------------------------------------
    // DATA DIRECTORY
    // --------------------------------------------------------

    if (!ensureDirectory(DATA_DIR))
    {
        Serial.println(
            "[STORAGE] Failed to create data directory"
        );

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


    Serial.println(
        "[STORAGE] Ready"
    );

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
            Serial.printf(
                "[STORAGE] Failed to open: %s\n",
                path
            );

            return false;
        }

        bool isDirectory =
            dir.isDirectory();

        dir.close();

        if (!isDirectory)
        {
            Serial.printf(
                "[STORAGE] ERROR: %s exists but is not a directory\n",
                path
            );

            return false;
        }

        Serial.printf(
            "[STORAGE] Directory exists: %s\n",
            path
        );

        return true;
    }


    // Директорії немає — створюємо.

    if (!SD.mkdir(path))
    {
        Serial.printf(
            "[STORAGE] Failed to create directory: %s\n",
            path
        );

        return false;
    }


    Serial.printf(
        "[STORAGE] Directory created: %s\n",
        path
    );

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
        Serial.printf(
            "[STORAGE] File exists: %s\n",
            path
        );

        return true;
    }


    File file = SD.open(
        path,
        FILE_WRITE
    );


    if (!file)
    {
        Serial.printf(
            "[STORAGE] Failed to create: %s\n",
            path
        );

        return false;
    }


    size_t written =
        file.print(defaultContent);


    file.close();


    if (written == 0)
    {
        Serial.printf(
            "[STORAGE] Failed to write default data: %s\n",
            path
        );

        return false;
    }


    Serial.printf(
        "[STORAGE] Created: %s\n",
        path
    );

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
        Serial.println(
            "[STORAGE] Not ready"
        );

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


    Serial.println(
        "[STORAGE] Statistics status:"
    );


    Serial.printf(
        "[STORAGE] A: %s",
        validA ? "VALID" : "INVALID"
    );

    if (validA)
    {
        Serial.printf(
            " | seq=%lu",
            (unsigned long)sequenceA
        );
    }

    Serial.println();


    Serial.printf(
        "[STORAGE] B: %s",
        validB ? "VALID" : "INVALID"
    );

    if (validB)
    {
        Serial.printf(
            " | seq=%lu",
            (unsigned long)sequenceB
        );
    }

    Serial.println();


    // --------------------------------------------------------
    // No valid statistics
    // --------------------------------------------------------

    if (!validA && !validB)
    {
        Serial.println(
            "[STORAGE] No valid statistics found"
        );

        Serial.println(
            "[STORAGE] Creating initial statistics"
        );


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

        Serial.println(
            "[STORAGE] Using slot A"
        );

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

        Serial.println(
            "[STORAGE] Using slot B"
        );

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

        Serial.println(
            "[STORAGE] Using newest slot A"
        );
    }
    else
    {
        result = dataB;

        activeSlot = 'B';

        activeSequence = sequenceB;

        Serial.println(
            "[STORAGE] Using newest slot B"
        );
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


    Serial.printf(
        "[STORAGE] Saving statistics: slot %c, seq=%lu\n",
        newActiveSlot,
        (unsigned long)newSequence
    );


    bool success =
        writeStatisticsSlot(
            targetFile,
            data,
            newSequence
        );


    if (!success)
    {
        Serial.println(
            "[STORAGE] Statistics save FAILED"
        );

        return false;
    }


    // Тільки ПІСЛЯ успішного запису
    // переключаємо активний слот.

    activeSlot =
        newActiveSlot;

    activeSequence =
        newSequence;


    Serial.println(
        "[STORAGE] Statistics saved OK"
    );

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
        Serial.printf(
            "[STORAGE] Invalid file size: %s\n",
            path
        );

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
        Serial.printf(
            "[STORAGE] Invalid magic: %s\n",
            path
        );

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
        Serial.printf(
            "[STORAGE] Invalid version: %s\n",
            path
        );

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
        Serial.printf(
            "[STORAGE] CRC error: %s\n",
            path
        );

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
        Serial.println(
            "[STORAGE] Failed to open temp file"
        );

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
        Serial.println(
            "[STORAGE] Failed to write complete statistics record"
        );

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
        Serial.println(
            "[STORAGE] Written data verification FAILED"
        );

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
        Serial.printf(
            "[STORAGE] Failed to remove old file: %s\n",
            path
        );

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
    Serial.println(
        "[STORAGE] Failed to activate new statistics file"
    );

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