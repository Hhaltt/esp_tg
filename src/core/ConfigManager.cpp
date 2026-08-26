#include "ConfigManager.h"

#include <SD.h>
#include <ArduinoJson.h>

#include "../storage/SDCardManager.h"


static const char* CONFIG_FILE =
    "/data/config.json";


ConfigManager config;


// ============================================================
// DEFAULTS
// ============================================================

void ConfigManager::setDefaults()
{
    // --------------------------------------------------------
    // SYSTEM
    // --------------------------------------------------------

    data.system.deviceName =
        "Emily";

    data.system.timezone =
        "Europe/Kyiv";

    data.system.startupMessageEnabled =
        true;

    data.system.startupMessage =
        "Emily is online 🤖";


    // --------------------------------------------------------
    // WEATHER
    // --------------------------------------------------------

    data.weather.enabled =
        false;

    data.weather.apiKey =
        "";

    data.weather.city =
        "";

    data.weather.hour =
        7;

    data.weather.minute =
        0;
}


// ============================================================
// BEGIN
// ============================================================

bool ConfigManager::begin()
{
    Serial.println(
        "[CONFIG] Initializing..."
    );


    if (!sdCard.isAvailable())
    {
        Serial.println(
            "[CONFIG] SD card unavailable"
        );

        return false;
    }


    setDefaults();


    if (!load())
    {
        Serial.println(
            "[CONFIG] Failed to load config"
        );

        return false;
    }


    ready = true;


    Serial.println(
        "[CONFIG] Ready"
    );


    return true;
}


// ============================================================
// LOAD
// ============================================================

bool ConfigManager::load()
{
    Serial.println(
        "[CONFIG] Loading..."
    );


    if (!SD.exists(CONFIG_FILE))
    {
        Serial.println(
            "[CONFIG] config.json not found"
        );

        return false;
    }


    File file =
        SD.open(
            CONFIG_FILE,
            FILE_READ
        );


    if (!file)
    {
        Serial.println(
            "[CONFIG] Failed to open config.json"
        );

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
            "[CONFIG] JSON error: "
        );

        Serial.println(
            error.c_str()
        );

        return false;
    }


    // --------------------------------------------------------
    // SYSTEM
    // --------------------------------------------------------

    if (doc["system"]["deviceName"].is<const char*>())
    {
        data.system.deviceName =
            doc["system"]["deviceName"]
                .as<const char*>();
    }


    if (doc["system"]["timezone"].is<const char*>())
    {
        data.system.timezone =
            doc["system"]["timezone"]
                .as<const char*>();
    }


    if (!doc["system"]["startupMessageEnabled"].isNull())
    {
        data.system.startupMessageEnabled =
            doc["system"]["startupMessageEnabled"];
    }


    if (doc["system"]["startupMessage"].is<const char*>())
    {
        data.system.startupMessage =
            doc["system"]["startupMessage"]
                .as<const char*>();
    }


    // --------------------------------------------------------
    // WEATHER
    // --------------------------------------------------------

    if (!doc["weather"]["enabled"].isNull())
    {
        data.weather.enabled =
            doc["weather"]["enabled"];
    }


    if (doc["weather"]["apiKey"].is<const char*>())
    {
        data.weather.apiKey =
            doc["weather"]["apiKey"]
                .as<const char*>();
    }


    if (doc["weather"]["city"].is<const char*>())
    {
        data.weather.city =
            doc["weather"]["city"]
                .as<const char*>();
    }


    if (!doc["weather"]["hour"].isNull())
    {
        data.weather.hour =
            doc["weather"]["hour"];
    }


    if (!doc["weather"]["minute"].isNull())
    {
        data.weather.minute =
            doc["weather"]["minute"];
    }


    // --------------------------------------------------------
    // VALIDATE
    // --------------------------------------------------------

    if (data.system.deviceName.length() == 0)
    {
        data.system.deviceName =
            "Emily";
    }


    if (data.system.timezone.length() == 0)
    {
        data.system.timezone =
            "Europe/Kyiv";
    }


    if (data.weather.hour > 23)
    {
        data.weather.hour = 7;
    }


    if (data.weather.minute > 59)
    {
        data.weather.minute = 0;
    }


    // --------------------------------------------------------
    // LOG
    // --------------------------------------------------------

    Serial.print(
        "[CONFIG] Device name: "
    );

    Serial.println(
        data.system.deviceName
    );


    Serial.print(
        "[CONFIG] Timezone: "
    );

    Serial.println(
        data.system.timezone
    );


    Serial.print(
        "[CONFIG] Startup message: "
    );

    Serial.println(
        data.system.startupMessageEnabled
            ? "enabled"
            : "disabled"
    );


    Serial.print(
        "[CONFIG] Weather: "
    );

    Serial.println(
        data.weather.enabled
            ? "enabled"
            : "disabled"
    );


    return true;
}


// ============================================================
// SAVE
// ============================================================

bool ConfigManager::save()
{
    if (!sdCard.isAvailable())
    {
        Serial.println(
            "[CONFIG] Cannot save: SD unavailable"
        );

        return false;
    }


    Serial.println(
        "[CONFIG] Saving..."
    );


    JsonDocument doc;


    // --------------------------------------------------------
    // SYSTEM
    // --------------------------------------------------------

    doc["system"]["deviceName"] =
        data.system.deviceName;

    doc["system"]["timezone"] =
        data.system.timezone;

    doc["system"]["startupMessageEnabled"] =
        data.system.startupMessageEnabled;

    doc["system"]["startupMessage"] =
        data.system.startupMessage;


    // --------------------------------------------------------
    // WEATHER
    // --------------------------------------------------------

    doc["weather"]["enabled"] =
        data.weather.enabled;

    doc["weather"]["apiKey"] =
        data.weather.apiKey;

    doc["weather"]["city"] =
        data.weather.city;

    doc["weather"]["hour"] =
        data.weather.hour;

    doc["weather"]["minute"] =
        data.weather.minute;


    // --------------------------------------------------------
    // TEMP FILE
    // --------------------------------------------------------

    const char* tempFile =
        "/data/config.tmp";


    if (SD.exists(tempFile))
    {
        SD.remove(tempFile);
    }


    File file =
        SD.open(
            tempFile,
            FILE_WRITE
        );


    if (!file)
    {
        Serial.println(
            "[CONFIG] Failed to open temp file"
        );

        return false;
    }


    size_t written =
        serializeJsonPretty(
            doc,
            file
        );


    file.flush();

    file.close();


    if (written == 0)
    {
        Serial.println(
            "[CONFIG] Failed to write config"
        );

        SD.remove(tempFile);

        return false;
    }


    // --------------------------------------------------------
    // ACTIVATE NEW FILE
    // --------------------------------------------------------

    if (SD.exists(CONFIG_FILE))
    {
        if (!SD.remove(CONFIG_FILE))
        {
            Serial.println(
                "[CONFIG] Failed to remove old config"
            );

            return false;
        }
    }


    if (!SD.rename(
            tempFile,
            CONFIG_FILE
        ))
    {
        Serial.println(
            "[CONFIG] Failed to activate config"
        );

        return false;
    }


    Serial.println(
        "[CONFIG] Saved"
    );


    return true;
}


// ============================================================
// SYSTEM GETTERS
// ============================================================

String ConfigManager::getDeviceName() const
{
    return data.system.deviceName;
}


String ConfigManager::getTimezone() const
{
    return data.system.timezone;
}


bool ConfigManager::isStartupMessageEnabled() const
{
    return data.system.startupMessageEnabled;
}


String ConfigManager::getStartupMessage() const
{
    return data.system.startupMessage;
}


// ============================================================
// SYSTEM SETTERS
// ============================================================

void ConfigManager::setDeviceName(
    const String& name
)
{
    data.system.deviceName = name;
}


void ConfigManager::setTimezone(
    const String& timezone
)
{
    data.system.timezone = timezone;
}


void ConfigManager::setStartupMessageEnabled(
    bool enabled
)
{
    data.system.startupMessageEnabled = enabled;
}


void ConfigManager::setStartupMessage(
    const String& message
)
{
    data.system.startupMessage = message;
}


// ============================================================
// WEATHER GETTERS
// ============================================================

bool ConfigManager::isWeatherEnabled() const
{
    return data.weather.enabled;
}


String ConfigManager::getWeatherApiKey() const
{
    return data.weather.apiKey;
}


String ConfigManager::getWeatherCity() const
{
    return data.weather.city;
}


uint8_t ConfigManager::getWeatherHour() const
{
    return data.weather.hour;
}


uint8_t ConfigManager::getWeatherMinute() const
{
    return data.weather.minute;
}


// ============================================================
// WEATHER SETTERS
// ============================================================

void ConfigManager::setWeatherEnabled(
    bool enabled
)
{
    data.weather.enabled = enabled;
}


void ConfigManager::setWeatherApiKey(
    const String& apiKey
)
{
    data.weather.apiKey = apiKey;
}


void ConfigManager::setWeatherCity(
    const String& city
)
{
    data.weather.city = city;
}


void ConfigManager::setWeatherTime(
    uint8_t hour,
    uint8_t minute
)
{
    if (hour <= 23)
    {
        data.weather.hour = hour;
    }


    if (minute <= 59)
    {
        data.weather.minute = minute;
    }
}