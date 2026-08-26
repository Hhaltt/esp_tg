#pragma once

#include <Arduino.h>
#include <time.h>

enum class ReminderType { ONCE, DAILY, WEEKLY, MONTHLY, YEARLY };

struct Reminder
{
    uint32_t id = 0;
    bool enabled = true;
    ReminderType type = ReminderType::ONCE;
    int year = 0;
    int month = 0;
    int day = 0;
    int weekday = 0;
    int hour = 0;
    int minute = 0;
    String message;
    String chatId;
    bool silent = false;
    time_t lastTriggered = 0;
    time_t nextTrigger = 0;
};

class ReminderManager
{
public:
    bool begin();
    void update();
    uint32_t addReminder(Reminder reminder);
    bool updateReminder(const Reminder& reminder);
    bool deleteReminder(uint32_t id);
    bool setEnabled(uint32_t id, bool enabled);
    size_t getCount() const;
    Reminder getReminder(size_t index) const;
    bool getReminderById(uint32_t id, Reminder& reminder) const;
    bool load();
    bool save();

private:
    static const size_t MAX_REMINDERS = 256;
    Reminder reminders[MAX_REMINDERS];
    size_t reminderCount = 0;
    uint32_t nextId = 1;
    unsigned long lastCheck = 0;
    time_t calculateNextTrigger(Reminder& reminder, time_t fromTime);
    time_t calculateScheduledTime(Reminder& reminder, time_t referenceTime);
    void checkReminder(Reminder& reminder, time_t now);
    void advanceReminder(Reminder& reminder, time_t now);
    const char* typeToString(ReminderType type) const;
    ReminderType stringToType(const String& type) const;
};

extern ReminderManager reminderManager;
