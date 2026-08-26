#include "ReminderManager.h"

#include <SD.h>
#include <ArduinoJson.h>

#include "../services/TimeManager.h"
#include "../network/TelegramManager.h"


// ============================================================
// CONFIG
// ============================================================

static const char* REMINDERS_FILE =
    "/data/reminders.json";


// ============================================================
// GLOBAL
// ============================================================

ReminderManager reminderManager;


// ============================================================
// BEGIN
// ============================================================

bool ReminderManager::begin()
{
    Serial.println(
        "[REMINDERS] Initializing..."
    );


    if (!load())
    {
        Serial.println(
            "[REMINDERS] Failed to load reminders"
        );

        return false;
    }


    Serial.print(
        "[REMINDERS] Loaded: "
    );

    Serial.println(
        reminderCount
    );


    return true;
}


// ============================================================
// UPDATE
// ============================================================

void ReminderManager::update()
{
    if (!timeManager.isTimeValid())
    {
        return;
    }


    // Не треба перевіряти десятки разів на секунду

    if (
        millis() - lastCheck <
        5000
    )
    {
        return;
    }


    lastCheck =
        millis();


    time_t now =
        timeManager.getTimestamp();


    if (now <= 0)
    {
        return;
    }


    for (
        size_t i = 0;
        i < reminderCount;
        i++
    )
    {
        checkReminder(
            reminders[i],
            now
        );
    }
}


// ============================================================
// CHECK REMINDER
// ============================================================

void ReminderManager::checkReminder(
    Reminder& reminder,
    time_t now
)
{
    if (!reminder.enabled)
    {
        return;
    }


    // --------------------------------------------------------
    // FIRST CALCULATION
    // --------------------------------------------------------

    if (reminder.nextTrigger == 0)
    {
        reminder.nextTrigger =
            calculateNextTrigger(
                reminder,
                now
            );


        Serial.printf(
            "[REMINDERS] #%lu initialized | now=%lld | next=%lld\n",
            (unsigned long)reminder.id,
            (long long)now,
            (long long)reminder.nextTrigger
        );


        save();

        return;
    }


    // --------------------------------------------------------
    // NOT YET
    // --------------------------------------------------------

    if (now < reminder.nextTrigger)
    {
        return;
    }


    // --------------------------------------------------------
    // TRIGGER
    // --------------------------------------------------------

    Serial.printf(
        "[REMINDERS] #%lu DUE | now=%lld | next=%lld | chatId='%s'\n",
        (unsigned long)reminder.id,
        (long long)now,
        (long long)reminder.nextTrigger,
        reminder.chatId.c_str()
    );


    if (reminder.chatId.length() == 0)
    {
        Serial.println(
            "[REMINDERS] ERROR: Empty chat ID"
        );

        return;
    }


    if (!telegram.isStarted())
    {
        Serial.println(
            "[REMINDERS] Telegram not ready"
        );

        return;
    }


    Serial.println(
        "[REMINDERS] Sending Telegram..."
    );


    bool sent =
        telegram.sendMessage(
            reminder.chatId,
            reminder.message
        );


    if (!sent)
    {
        Serial.println(
            "[REMINDERS] Telegram send FAILED, will retry"
        );

        return;
    }


    Serial.println(
        "[REMINDERS] Telegram sent OK"
    );


    // --------------------------------------------------------
    // SAVE DELIVERY TIME
    // --------------------------------------------------------

    reminder.lastTriggered = now;


    // --------------------------------------------------------
    // ONE-TIME REMINDER
    // --------------------------------------------------------

    if (reminder.type == ReminderType::ONCE)
    {
        reminder.enabled = false;

        reminder.nextTrigger = 0;


        Serial.println(
            "[REMINDERS] One-time reminder completed"
        );
    }
    else
    {
        advanceReminder(
            reminder,
            now
        );
    }


    if (!save())
    {
        Serial.println(
            "[REMINDERS] ERROR: Failed to save state"
        );
    }
}


// ============================================================
// ADVANCE
// ============================================================

void ReminderManager::advanceReminder(
    Reminder& reminder,
    time_t now
)
{
    // Беремо саме запланований час,
    // а не час фактичної відправки

    time_t base =
        reminder.nextTrigger;


    if (base <= 0)
    {
        base = now;
    }


    time_t next =
        calculateNextTrigger(
            reminder,
            base + 1
        );


    // Якщо Emily була вимкнена дуже довго,
    // calculateNextTrigger може повернути минулий час.
    // Просуваємо до першого майбутнього циклу.

    while (
        next <= now
    )
    {
        next =
            calculateNextTrigger(
                reminder,
                next + 1
            );
    }


    reminder.nextTrigger =
        next;


    Serial.print(
        "[REMINDERS] Next trigger: "
    );

    Serial.println(
        (long long)next
    );
}


// ============================================================
// CALCULATE NEXT TRIGGER
// ============================================================

time_t ReminderManager::calculateNextTrigger(
    Reminder& reminder,
    time_t fromTime
)
{
    struct tm reference;


    if (
        localtime_r(
            &fromTime,
            &reference
        ) == nullptr
    )
    {
        return 0;
    }


    // --------------------------------------------------------
    // ONCE
    // --------------------------------------------------------

    if (
        reminder.type ==
        ReminderType::ONCE
    )
    {
        struct tm target = {};

        target.tm_year =
            reminder.year - 1900;

        target.tm_mon =
            reminder.month - 1;

        target.tm_mday =
            reminder.day;

        target.tm_hour =
            reminder.hour;

        target.tm_min =
            reminder.minute;

        target.tm_sec = 0;

        target.tm_isdst = -1;


        return mktime(
            &target
        );
    }


    // --------------------------------------------------------
    // DAILY
    // --------------------------------------------------------

    if (
        reminder.type ==
        ReminderType::DAILY
    )
    {
        struct tm target =
            reference;


        target.tm_hour =
            reminder.hour;

        target.tm_min =
            reminder.minute;

        target.tm_sec = 0;

        target.tm_isdst = -1;


        time_t result =
            mktime(
                &target
            );


        if (
            result <
            fromTime
        )
        {
            target.tm_mday++;

            result =
                mktime(
                    &target
                );
        }


        return result;
    }


    // --------------------------------------------------------
    // WEEKLY
    // --------------------------------------------------------

    if (
        reminder.type ==
        ReminderType::WEEKLY
    )
    {
        struct tm target =
            reference;


        int daysToAdd =
            reminder.weekday -
            reference.tm_wday;


        if (
            daysToAdd < 0
        )
        {
            daysToAdd += 7;
        }


        target.tm_mday +=
            daysToAdd;

        target.tm_hour =
            reminder.hour;

        target.tm_min =
            reminder.minute;

        target.tm_sec = 0;

        target.tm_isdst = -1;


        time_t result =
            mktime(
                &target
            );


        if (
            result <
            fromTime
        )
        {
            target.tm_mday += 7;

            result =
                mktime(
                    &target
                );
        }


        return result;
    }


    // --------------------------------------------------------
    // MONTHLY
    // --------------------------------------------------------

    if (
        reminder.type ==
        ReminderType::MONTHLY
    )
    {
        struct tm target =
            reference;


        target.tm_mday =
            reminder.day;

        target.tm_hour =
            reminder.hour;

        target.tm_min =
            reminder.minute;

        target.tm_sec = 0;

        target.tm_isdst = -1;


        time_t result =
            mktime(
                &target
            );


        if (
            result <
            fromTime
        )
        {
            target.tm_mon++;

            target.tm_mday =
                reminder.day;

            result =
                mktime(
                    &target
                );
        }


        return result;
    }


    // --------------------------------------------------------
    // YEARLY
    // --------------------------------------------------------

    if (
        reminder.type ==
        ReminderType::YEARLY
    )
    {
        struct tm target =
            reference;


        target.tm_mon =
            reminder.month - 1;

        target.tm_mday =
            reminder.day;

        target.tm_hour =
            reminder.hour;

        target.tm_min =
            reminder.minute;

        target.tm_sec = 0;

        target.tm_isdst = -1;


        time_t result =
            mktime(
                &target
            );


        if (
            result <
            fromTime
        )
        {
            target.tm_year++;

            target.tm_mon =
                reminder.month - 1;

            target.tm_mday =
                reminder.day;

            result =
                mktime(
                    &target
                );
        }


        return result;
    }


    return 0;
}


// ============================================================
// ADD
// ============================================================

uint32_t ReminderManager::addReminder(
    Reminder reminder
)
{
    if (
        reminderCount >=
        MAX_REMINDERS
    )
    {
        return 0;
    }


    reminder.id =
        nextId++;


    reminder.nextTrigger = 0;

    reminder.lastTriggered = 0;


    reminders[reminderCount] =
        reminder;


    reminderCount++;


    if (!save())
    {
        Serial.println(
            "[REMINDERS] Failed to save new reminder"
        );
    }


    return reminder.id;
}


// ============================================================
// UPDATE REMINDER
// ============================================================

bool ReminderManager::updateReminder(
    const Reminder& reminder
)
{
    for (
        size_t i = 0;
        i < reminderCount;
        i++
    )
    {
        if (
            reminders[i].id ==
            reminder.id
        )
        {
            Reminder updated =
                reminder;


            // При зміні нагадування
            // перераховуємо наступний запуск

            updated.nextTrigger = 0;


            reminders[i] =
                updated;


            return save();
        }
    }


    return false;
}


// ============================================================
// DELETE
// ============================================================

// ============================================================
// DELETE REMINDER
// ============================================================

bool ReminderManager::deleteReminder(
    uint32_t id
)
{
    for (
        size_t i = 0;
        i < reminderCount;
        i++
    )
    {
        if (
            reminders[i].id == id
        )
        {
            for (
                size_t j = i;
                j < reminderCount - 1;
                j++
            )
            {
                reminders[j] =
                    reminders[j + 1];
            }


            reminderCount--;


            return save();
        }
    }


    return false;
}


// ============================================================
// ENABLE / DISABLE
// ============================================================

bool ReminderManager::setEnabled(
    uint32_t id,
    bool enabled
)
{
    for (
        size_t i = 0;
        i < reminderCount;
        i++
    )
    {
        if (
            reminders[i].id ==
            id
        )
        {
           reminders[i].enabled = enabled;


if (enabled)
{
    // Перераховуємо наступний запуск.
    // Не використовуємо старий час,
    // який міг залишитися в минулому.

    reminders[i].nextTrigger =
        calculateNextTrigger(
            reminders[i],
            timeManager.getTimestamp()
        );
}


return save();
        }
    }


    return false;
}


// ============================================================
// GET COUNT
// ============================================================

size_t ReminderManager::getCount() const
{
    return reminderCount;
}


// ============================================================
// GET BY INDEX
// ============================================================

Reminder ReminderManager::getReminder(
    size_t index
) const
{
    if (
        index >=
        reminderCount
    )
    {
        return Reminder();
    }


    return reminders[index];
}


// ============================================================
// GET BY ID
// ============================================================

bool ReminderManager::getReminderById(
    uint32_t id,
    Reminder& reminder
) const
{
    for (
        size_t i = 0;
        i < reminderCount;
        i++
    )
    {
        if (
            reminders[i].id ==
            id
        )
        {
            reminder =
                reminders[i];

            return true;
        }
    }


    return false;
}


// ============================================================
// LOAD
// ============================================================

bool ReminderManager::load()
{
    if (
        !SD.exists(
            REMINDERS_FILE
        )
    )
    {
        Serial.println(
            "[REMINDERS] File does not exist, creating"
        );


        File file =
            SD.open(
                REMINDERS_FILE,
                FILE_WRITE
            );


        if (!file)
        {
            return false;
        }


        file.print(
            "{"
            "\"nextId\":1,"
            "\"reminders\":[]"
            "}"
        );


        file.close();


        return true;
    }


    File file =
        SD.open(
            REMINDERS_FILE,
            FILE_READ
        );


    if (!file)
    {
        return false;
    }


    JsonDocument doc;


    DeserializationError error =
        deserializeJson(
            doc,
            file
        );


    file.close();


    if (error)
    {
        Serial.print(
            "[REMINDERS] JSON error: "
        );

        Serial.println(
            error.c_str()
        );

        return false;
    }


    nextId =
        doc["nextId"] | 1;


    JsonArray array =
        doc["reminders"];


    reminderCount = 0;


    for (
        JsonObject object :
        array
    )
    {
        if (
            reminderCount >=
            MAX_REMINDERS
        )
        {
            break;
        }


        Reminder& reminder =
            reminders[
                reminderCount
            ];


        reminder.id =
            object["id"] | 0;


        reminder.enabled =
            object["enabled"] | true;


        reminder.type =
            stringToType(
                object["type"] |
                "once"
            );


        reminder.year =
            object["year"] | 0;

        reminder.month =
            object["month"] | 0;

        reminder.day =
            object["day"] | 0;

        reminder.weekday =
            object["weekday"] | 0;

        reminder.hour =
            object["hour"] | 0;

        reminder.minute =
            object["minute"] | 0;


        reminder.message =
            String(
                object["message"] |
                ""
            );


        reminder.chatId =
            String(
                object["chatId"] |
                ""
            );


        reminder.lastTriggered =
            object["lastTriggered"] | 0;


        reminder.nextTrigger =
            object["nextTrigger"] | 0;


        reminderCount++;
    }


    return true;
}


// ============================================================
// SAVE
// ============================================================

bool ReminderManager::save()
{
    JsonDocument doc;


    doc["nextId"] =
        nextId;


    JsonArray array =
        doc.createNestedArray(
            "reminders"
        );


    for (
        size_t i = 0;
        i < reminderCount;
        i++
    )
    {
        Reminder& reminder =
            reminders[i];


        JsonObject object =
            array.createNestedObject();


        object["id"] =
            reminder.id;


        object["enabled"] =
            reminder.enabled;


        object["type"] =
            typeToString(
                reminder.type
            );


        object["year"] =
            reminder.year;

        object["month"] =
            reminder.month;

        object["day"] =
            reminder.day;

        object["weekday"] =
            reminder.weekday;

        object["hour"] =
            reminder.hour;

        object["minute"] =
            reminder.minute;


        object["message"] =
            reminder.message;


        object["chatId"] =
            reminder.chatId;


        object["lastTriggered"] =
            reminder.lastTriggered;


        object["nextTrigger"] =
            reminder.nextTrigger;
    }


    File file =
        SD.open(
            REMINDERS_FILE,
            FILE_WRITE
        );


    if (!file)
    {
        Serial.println(
            "[REMINDERS] Failed to open file"
        );

        return false;
    }


    if (
        serializeJson(
            doc,
            file
        ) == 0
    )
    {
        file.close();

        return false;
    }


    file.close();


    return true;
}


// ============================================================
// TYPE TO STRING
// ============================================================

const char* ReminderManager::typeToString(
    ReminderType type
) const
{
    switch (type)
    {
        case ReminderType::ONCE:
            return "once";

        case ReminderType::DAILY:
            return "daily";

        case ReminderType::WEEKLY:
            return "weekly";

        case ReminderType::MONTHLY:
            return "monthly";

        case ReminderType::YEARLY:
            return "yearly";
    }


    return "once";
}


// ============================================================
// STRING TO TYPE
// ============================================================

ReminderType ReminderManager::stringToType(
    const String& type
) const
{
    if (
        type == "daily"
    )
    {
        return ReminderType::DAILY;
    }


    if (
        type == "weekly"
    )
    {
        return ReminderType::WEEKLY;
    }


    if (
        type == "monthly"
    )
    {
        return ReminderType::MONTHLY;
    }


    if (
        type == "yearly"
    )
    {
        return ReminderType::YEARLY;
    }


    return ReminderType::ONCE;
}
