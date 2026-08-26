#include "Statistics.h"

#include "Config.h"


Statistics statistics;


// ============================================================
// BEGIN
// ============================================================

void Statistics::begin()
{
    bootMillis = millis();

    sessionUptime = 0;

    lastUpdate = millis();

    Serial.println("[STATS] Started");
}


// ============================================================
// LOAD
// ============================================================

void Statistics::load(
    const StatisticsData& loadedData
)
{
    data = loadedData;

    Serial.println("[STATS] Persistent statistics loaded");
}


// ============================================================
// UPDATE
// ============================================================

void Statistics::update()
{
    if (
        millis() - lastUpdate
        >= STATISTICS_UPDATE_INTERVAL
    )
    {
        lastUpdate = millis();

        sessionUptime =
            (millis() - bootMillis) / 1000ULL;
    }
}


// ============================================================
// GET PERSISTENT DATA
// ============================================================

StatisticsData Statistics::getData() const
{
    StatisticsData result = data;

    // Важливо:
    // додаємо поточний uptime,
    // але data.totalUptime поки не змінюємо.
    //
    // Так при повторному checkpoint не буде
    // постійного накопичення одного й того ж uptime.

    result.totalUptime =
        data.totalUptime
        +
        getCurrentUptime();

    return result;
}


// ============================================================
// EVENTS
// ============================================================

void Statistics::onBoot()
{
    data.bootCount++;
}


void Statistics::onTelegramMessageReceived()
{
    data.messagesReceived++;
}


void Statistics::onTelegramMessageSent()
{
    data.messagesSent++;
}


void Statistics::onCommandReceived()
{
    data.commandsReceived++;
}


void Statistics::onCommandExecuted()
{
    data.commandsExecuted++;
}


void Statistics::onCommandError()
{
    data.commandErrors++;
}


void Statistics::onError()
{
    data.errors++;
}


// ============================================================
// GETTERS
// ============================================================

uint32_t Statistics::getBootCount() const
{
    return data.bootCount;
}


uint32_t Statistics::getMessagesReceived() const
{
    return data.messagesReceived;
}


uint32_t Statistics::getMessagesSent() const
{
    return data.messagesSent;
}


uint32_t Statistics::getCommandsReceived() const
{
    return data.commandsReceived;
}


uint32_t Statistics::getCommandsExecuted() const
{
    return data.commandsExecuted;
}


uint32_t Statistics::getCommandErrors() const
{
    return data.commandErrors;
}


uint32_t Statistics::getErrors() const
{
    return data.errors;
}


// ============================================================
// UPTIME
// ============================================================

uint64_t Statistics::getCurrentUptime() const
{
    return
        (millis() - bootMillis)
        / 1000ULL;
}


uint64_t Statistics::getTotalUptime() const
{
    return
        data.totalUptime
        +
        getCurrentUptime();
}


String Statistics::getCurrentUptimeString() const
{
    return formatDuration(
        getCurrentUptime()
    );
}


String Statistics::getTotalUptimeString() const
{
    return formatDuration(
        getTotalUptime()
    );
}


// ============================================================
// FORMAT DURATION
// ============================================================

String Statistics::formatDuration(
    uint64_t totalSeconds
) const
{
    uint64_t days =
        totalSeconds / 86400;

    totalSeconds %= 86400;


    uint64_t hours =
        totalSeconds / 3600;

    totalSeconds %= 3600;


    uint64_t minutes =
        totalSeconds / 60;

    uint64_t seconds =
        totalSeconds % 60;


    char buffer[64];


    snprintf(
        buffer,
        sizeof(buffer),
        "%llu д %02llu:%02llu:%02llu",
        days,
        hours,
        minutes,
        seconds
    );


    return String(buffer);
}