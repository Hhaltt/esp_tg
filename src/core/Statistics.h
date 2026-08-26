#pragma once

#include <Arduino.h>


// ============================================================
// Дані, які переживають перезапуск
// ============================================================

struct StatisticsData
{
    uint64_t totalUptime;

    uint32_t bootCount;

    uint32_t messagesReceived;
    uint32_t messagesSent;

    uint32_t commandsReceived;
    uint32_t commandsExecuted;
    uint32_t commandErrors;

    uint32_t errors;
};


// ============================================================
// Statistics
// ============================================================

class Statistics
{
public:

    void begin();

    void update();

    void load(
        const StatisticsData& data
    );

    StatisticsData getData() const;


    // --------------------------------------------------------
    // Events
    // --------------------------------------------------------

    void onBoot();

    void onTelegramMessageReceived();

    void onTelegramMessageSent();

    void onCommandReceived();

    void onCommandExecuted();

    void onCommandError();

    void onError();


    // --------------------------------------------------------
    // Getters
    // --------------------------------------------------------

    uint32_t getBootCount() const;

    uint32_t getMessagesReceived() const;

    uint32_t getMessagesSent() const;

    uint32_t getCommandsReceived() const;

    uint32_t getCommandsExecuted() const;

    uint32_t getCommandErrors() const;

    uint32_t getErrors() const;


    // --------------------------------------------------------
    // Uptime
    // --------------------------------------------------------

    uint64_t getCurrentUptime() const;

    uint64_t getTotalUptime() const;

    String getCurrentUptimeString() const;

    String getTotalUptimeString() const;

    String formatDuration(
        uint64_t seconds
    ) const;


private:

    StatisticsData data = {};

    uint64_t sessionUptime = 0;

    unsigned long bootMillis = 0;

    unsigned long lastUpdate = 0;
};


extern Statistics statistics;