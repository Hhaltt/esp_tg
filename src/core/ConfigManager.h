#pragma once

#include <Arduino.h>

struct TelegramChatConfig
{
    String id;
    String name;
};

struct WeatherConfig
{
    bool enabled = false;
    String apiKey;
    String city;
    uint8_t hour = 7;
    uint8_t minute = 0;
};

struct SystemConfig
{
    String deviceName;
    String timezone;
    String ntpServer;
    String botToken;
    String hubApiKey;
    bool startupMessageEnabled;
    String startupMessage;
};

struct GpioMonitorConfig
{
    bool enabled;
    String highMessage;
    String lowMessage;
};

struct AppConfig
{
    SystemConfig system;
    WeatherConfig weather; // Backward compatibility for old config files.
    TelegramChatConfig* chats = nullptr;
    size_t chatCount = 0;
    GpioMonitorConfig gpio35;
    GpioMonitorConfig gpio39;
};

class ConfigManager
{
public:
    bool begin();
    bool load();
    bool save();

    String getDeviceName() const;
    String getTimezone() const;
    String getNtpServer() const;
    String getBotToken() const;
    String getHubApiKey() const;
    bool isStartupMessageEnabled() const;
    String getStartupMessage() const;

    void setDeviceName(const String& value);
    void setTimezone(const String& value);
    void setNtpServer(const String& value);
    void setBotToken(const String& value);
    void setHubApiKey(const String& value);
    void setStartupMessageEnabled(bool value);
    void setStartupMessage(const String& value);

    size_t getChatCount() const;
    TelegramChatConfig getChat(size_t index) const;
    String getChatName(const String& id) const;
    bool hasChat(const String& id) const;
    bool addChat(const String& id, const String& name);
    bool updateChat(size_t index, const String& id, const String& name);
    bool deleteChat(size_t index);

    // Legacy weather accessors only for compatibility with existing code.
    bool isWeatherEnabled() const;
    String getWeatherApiKey() const;
    String getWeatherCity() const;
    uint8_t getWeatherHour() const;
    uint8_t getWeatherMinute() const;
    void setWeatherEnabled(bool value);
    void setWeatherApiKey(const String& value);
    void setWeatherCity(const String& value);
    void setWeatherTime(uint8_t hour, uint8_t minute);

    bool isGpio35Enabled() const;
    String getGpio35HighMessage() const;
    String getGpio35LowMessage() const;
    void setGpio35Enabled(bool value);
    void setGpio35HighMessage(const String& value);
    void setGpio35LowMessage(const String& value);

    bool isGpio39Enabled() const;
    String getGpio39HighMessage() const;
    String getGpio39LowMessage() const;
    void setGpio39Enabled(bool value);
    void setGpio39HighMessage(const String& value);
    void setGpio39LowMessage(const String& value);

private:
    AppConfig data;
    bool ready = false;

    void setDefaults();
    void clearChats();
    bool addChatInternal(const String& id, const String& name);
};

extern ConfigManager config;
