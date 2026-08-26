#include "ReminderManager.h"

#include <SD.h>
#include <ArduinoJson.h>

#include "../services/TimeManager.h"
#include "../network/TelegramManager.h"

static const char* REMINDERS_FILE = "/data/reminders.json";

ReminderManager reminderManager;

bool ReminderManager::begin()
{
    Serial.println("[REMINDERS] Initializing...");

    if (!load())
    {
        Serial.println("[REMINDERS] Failed to load reminders");
        return false;
    }

    Serial.printf("[REMINDERS] Loaded: %u\n", (unsigned)reminderCount);
    return true;
}

void ReminderManager::update()
{
    if (!timeManager.isTimeValid() || millis() - lastCheck < 5000)
        return;

    lastCheck = millis();

    time_t now = timeManager.getTimestamp();
    if (now <= 0)
        return;

    for (size_t i = 0; i < reminderCount;)
    {
        uint32_t id = reminders[i].id;

        if (checkReminder(reminders[i], now))
        {
            // deleteReminder() shifts the array, therefore do not increment i.
            if (!deleteReminder(id))
                Serial.printf("[REMINDERS] Failed to auto-delete #%lu\n", (unsigned long)id);

            continue;
        }

        i++;
    }
}

uint32_t ReminderManager::addReminder(Reminder reminder)
{
    if (reminderCount >= MAX_REMINDERS)
        return 0;

    reminder.id = nextId++;
    reminder.enabled = true;
    reminder.lastTriggered = 0;
    reminder.nextTrigger = 0;

    if (timeManager.isTimeValid())
        reminder.nextTrigger = calculateNextTrigger(reminder, timeManager.getTimestamp());

    reminders[reminderCount++] = reminder;

    if (!save())
    {
        reminderCount--;
        return 0;
    }

    return reminder.id;
}

bool ReminderManager::updateReminder(const Reminder& reminder)
{
    for (size_t i = 0; i < reminderCount; i++)
    {
        if (reminders[i].id != reminder.id)
            continue;

        Reminder updated = reminder;

        // Editing can change schedule, chat and notification mode.
        // Recalculate from current time and keep no stale delivery state.
        updated.nextTrigger = 0;
        updated.lastTriggered = 0;

        reminders[i] = updated;
        return save();
    }

    return false;
}

bool ReminderManager::deleteReminder(uint32_t id)
{
    for (size_t i = 0; i < reminderCount; i++)
    {
        if (reminders[i].id != id)
            continue;

        for (size_t j = i + 1; j < reminderCount; j++)
            reminders[j - 1] = reminders[j];

        reminderCount--;
        return save();
    }

    return false;
}

bool ReminderManager::setEnabled(uint32_t id, bool enabled)
{
    for (size_t i = 0; i < reminderCount; i++)
    {
        if (reminders[i].id != id)
            continue;

        reminders[i].enabled = enabled;

        if (enabled)
        {
            reminders[i].nextTrigger = 0;
            reminders[i].lastTriggered = 0;
        }

        return save();
    }

    return false;
}

size_t ReminderManager::getCount() const
{
    return reminderCount;
}

Reminder ReminderManager::getReminder(size_t index) const
{
    return index < reminderCount ? reminders[index] : Reminder();
}

bool ReminderManager::getReminderById(uint32_t id, Reminder& reminder) const
{
    for (size_t i = 0; i < reminderCount; i++)
    {
        if (reminders[i].id == id)
        {
            reminder = reminders[i];
            return true;
        }
    }

    return false;
}

bool ReminderManager::checkReminder(Reminder& reminder, time_t now)
{
    if (!reminder.enabled)
        return false;

    if (reminder.nextTrigger == 0)
    {
        reminder.nextTrigger = calculateNextTrigger(reminder, now);
        save();
        return false;
    }

    if (now < reminder.nextTrigger)
        return false;

    if (!reminder.chatId.length() || !telegram.isStarted())
        return false;

    if (!telegram.sendMessage(reminder.chatId, reminder.message, reminder.silent))
        return false;

    reminder.lastTriggered = now;

    if (reminder.type == ReminderType::ONCE)
    {
        Serial.printf("[REMINDERS] One-time reminder #%lu completed\n", (unsigned long)reminder.id);
        return true;
    }

    advanceReminder(reminder, now);
    save();
    return false;
}

static time_t makeLocal(int year, int month, int day, int hour, int minute, int second = 0)
{
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    t.tm_isdst = -1;
    return mktime(&t);
}

time_t ReminderManager::calculateScheduledTime(Reminder& reminder, time_t referenceTime)
{
    struct tm t;
    localtime_r(&referenceTime, &t);

    int year = t.tm_year + 1900;
    int month = t.tm_mon + 1;
    int day = t.tm_mday;

    if (reminder.type == ReminderType::ONCE)
    {
        year = reminder.year;
        month = reminder.month;
        day = reminder.day;
    }
    else if (reminder.type == ReminderType::MONTHLY)
        day = reminder.day;
    else if (reminder.type == ReminderType::YEARLY)
    {
        month = reminder.month;
        day = reminder.day;
    }

    return makeLocal(year, month, day, reminder.hour, reminder.minute);
}

time_t ReminderManager::calculateNextTrigger(Reminder& reminder, time_t fromTime)
{
    if (reminder.type == ReminderType::ONCE)
    {
        time_t target = makeLocal(reminder.year, reminder.month, reminder.day, reminder.hour, reminder.minute);
        return target >= fromTime ? target : 0;
    }

    struct tm t;
    localtime_r(&fromTime, &t);

    if (reminder.type == ReminderType::DAILY)
    {
        time_t target = makeLocal(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, reminder.hour, reminder.minute);
        if (target < fromTime)
            target += 86400;
        return target;
    }

    if (reminder.type == ReminderType::WEEKLY)
    {
        int delta = (reminder.weekday - t.tm_wday + 7) % 7;
        time_t target = makeLocal(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, reminder.hour, reminder.minute) + delta * 86400;
        if (target < fromTime)
            target += 7 * 86400;
        return target;
    }

    if (reminder.type == ReminderType::MONTHLY)
    {
        int year = t.tm_year + 1900;
        int month = t.tm_mon + 1;
        time_t target = makeLocal(year, month, reminder.day, reminder.hour, reminder.minute);

        if (target < fromTime)
        {
            month++;
            if (month > 12)
            {
                month = 1;
                year++;
            }
            target = makeLocal(year, month, reminder.day, reminder.hour, reminder.minute);
        }

        return target;
    }

    if (reminder.type == ReminderType::YEARLY)
    {
        int year = t.tm_year + 1900;
        time_t target = makeLocal(year, reminder.month, reminder.day, reminder.hour, reminder.minute);
        if (target < fromTime)
            target = makeLocal(year + 1, reminder.month, reminder.day, reminder.hour, reminder.minute);
        return target;
    }

    return 0;
}

void ReminderManager::advanceReminder(Reminder& reminder, time_t now)
{
    reminder.nextTrigger = calculateNextTrigger(reminder, now + 1);
}

const char* ReminderManager::typeToString(ReminderType type) const
{
    switch (type)
    {
        case ReminderType::DAILY: return "daily";
        case ReminderType::WEEKLY: return "weekly";
        case ReminderType::MONTHLY: return "monthly";
        case ReminderType::YEARLY: return "yearly";
        default: return "once";
    }
}

ReminderType ReminderManager::stringToType(const String& type) const
{
    if (type == "daily") return ReminderType::DAILY;
    if (type == "weekly") return ReminderType::WEEKLY;
    if (type == "monthly") return ReminderType::MONTHLY;
    if (type == "yearly") return ReminderType::YEARLY;
    return ReminderType::ONCE;
}

bool ReminderManager::load()
{
    if (!SD.exists(REMINDERS_FILE))
    {
        JsonDocument doc;
        doc["nextId"] = 1;
        doc["reminders"].to<JsonArray>();

        File file = SD.open(REMINDERS_FILE, FILE_WRITE);
        if (!file)
            return false;

        serializeJson(doc, file);
        file.close();
        return true;
    }

    File file = SD.open(REMINDERS_FILE, FILE_READ);
    if (!file)
        return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
        return false;

    nextId = doc["nextId"] | 1;
    reminderCount = 0;

    for (JsonObject object : doc["reminders"].as<JsonArray>())
    {
        if (reminderCount >= MAX_REMINDERS)
            break;

        Reminder& reminder = reminders[reminderCount];
        reminder.id = object["id"] | 0;
        reminder.enabled = object["enabled"] | true;
        reminder.type = stringToType(String(object["type"] | "once"));
        reminder.year = object["year"] | 0;
        reminder.month = object["month"] | 0;
        reminder.day = object["day"] | 0;
        reminder.weekday = object["weekday"] | 0;
        reminder.hour = object["hour"] | 0;
        reminder.minute = object["minute"] | 0;
        reminder.message = String(object["message"] | "");
        reminder.chatId = String(object["chatId"] | "");
        reminder.silent = object["silent"] | false;
        reminder.lastTriggered = object["lastTriggered"] | 0;
        reminder.nextTrigger = object["nextTrigger"] | 0;
        reminderCount++;
    }

    return true;
}

bool ReminderManager::save()
{
    JsonDocument doc;
    doc["nextId"] = nextId;

    JsonArray array = doc["reminders"].to<JsonArray>();

    for (size_t i = 0; i < reminderCount; i++)
    {
        Reminder& reminder = reminders[i];
        JsonObject object = array.add<JsonObject>();
        object["id"] = reminder.id;
        object["enabled"] = reminder.enabled;
        object["type"] = typeToString(reminder.type);
        object["year"] = reminder.year;
        object["month"] = reminder.month;
        object["day"] = reminder.day;
        object["weekday"] = reminder.weekday;
        object["hour"] = reminder.hour;
        object["minute"] = reminder.minute;
        object["message"] = reminder.message;
        object["chatId"] = reminder.chatId;
        object["silent"] = reminder.silent;
        object["lastTriggered"] = reminder.lastTriggered;
        object["nextTrigger"] = reminder.nextTrigger;
    }

    const char* tempFile = "/data/reminders.tmp";

    if (SD.exists(tempFile))
        SD.remove(tempFile);

    File file = SD.open(tempFile, FILE_WRITE);
    if (!file)
        return false;

    size_t written = serializeJsonPretty(doc, file);
    file.flush();
    file.close();

    if (!written)
    {
        SD.remove(tempFile);
        return false;
    }

    if (SD.exists(REMINDERS_FILE) && !SD.remove(REMINDERS_FILE))
        return false;

    return SD.rename(tempFile, REMINDERS_FILE);
}
