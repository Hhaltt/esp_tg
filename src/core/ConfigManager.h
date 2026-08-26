#pragma once

#include <Arduino.h>


// ============================================================
// CONFIG DATA
// ============================================================

struct SystemConfig
{
    String deviceName;

    String timezone;

    bool startupMessageEnabled;

    String startupMessage;
};


struct WeatherConfig
{
    bool enabled;

    String apiKey;

    String city;

    uint8_t hour;

    uint8_t minute;
};


struct AppConfig
{
    SystemConfig system;

    WeatherConfig weather;
};


// ============================================================
// CONFIG MANAGER
// ============================================================

class ConfigManager
{
public:

    bool begin();

    bool load();

    bool save();


    // --------------------------------------------------------
    // SYSTEM
    // --------------------------------------------------------

    String getDeviceName() const;

    String getTimezone() const;

    bool isStartupMessageEnabled() const;

    String getStartupMessage() const;


    void setDeviceName(
        const String& name
    );

    void setTimezone(
        const String& timezone
    );

    void setStartupMessageEnabled(
        bool enabled
    );

    void setStartupMessage(
        const String& message
    );


    // --------------------------------------------------------
    // WEATHER
    // --------------------------------------------------------

    bool isWeatherEnabled() const;

    String getWeatherApiKey() const;

    String getWeatherCity() const;

    uint8_t getWeatherHour() const;

    uint8_t getWeatherMinute() const;


    void setWeatherEnabled(
        bool enabled
    );

    void setWeatherApiKey(
        const String& apiKey
    );

    void setWeatherCity(
        const String& city
    );

    void setWeatherTime(
        uint8_t hour,
        uint8_t minute
    );


private:

    AppConfig data;

    bool ready = false;


    void setDefaults();

};


extern ConfigManager config;