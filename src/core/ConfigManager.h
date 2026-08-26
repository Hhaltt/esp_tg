#pragma once

#include <Arduino.h>

struct TelegramChatConfig { String id; String name; };
struct WeatherConfig { bool enabled=false; String apiKey; String city; uint8_t hour=7; uint8_t minute=0; };
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
struct GpioMonitorConfig { bool enabled; String highMessage; String lowMessage; };
struct AppConfig
{
    SystemConfig system;
    WeatherConfig weather; // compatibility only; weather UI/service is being removed.
    TelegramChatConfig* chats = nullptr;
    size_t chatCount = 0;
    GpioMonitorConfig gpio35;
    GpioMonitorConfig gpio39;
};

class ConfigManager
{
public:
    bool begin(); bool load(); bool save();
    String getDeviceName() const; String getTimezone() const; String getNtpServer() const;
    String getBotToken() const; String getHubApiKey() const;
    bool isStartupMessageEnabled() const; String getStartupMessage() const;
    void setDeviceName(const String&); void setTimezone(const String&); void setNtpServer(const String&);
    void setBotToken(const String&); void setHubApiKey(const String&);
    void setStartupMessageEnabled(bool); void setStartupMessage(const String&);

    size_t getChatCount() const; TelegramChatConfig getChat(size_t index) const;
    String getChatName(const String& id) const; bool hasChat(const String& id) const;
    bool addChat(const String& id, const String& name); bool updateChat(size_t index, const String& id, const String& name);
    bool deleteChat(size_t index);

    // Legacy weather accessors kept temporarily so existing modules compile.
    bool isWeatherEnabled() const; String getWeatherApiKey() const; String getWeatherCity() const;
    uint8_t getWeatherHour() const; uint8_t getWeatherMinute() const;
    void setWeatherEnabled(bool); void setWeatherApiKey(const String&); void setWeatherCity(const String&);
    void setWeatherTime(uint8_t, uint8_t);

    bool isGpio35Enabled() const; String getGpio35HighMessage() const; String getGpio35LowMessage() const;
    void setGpio35Enabled(bool); void setGpio35HighMessage(const String&); void setGpio35LowMessage(const String&);
    bool isGpio39Enabled() const; String getGpio39HighMessage() const; String getGpio39LowMessage() const;
    void setGpio39Enabled(bool); void setGpio39HighMessage(const String&); void setGpio39LowMessage(const String&);
private:
    static const size_t MAX_CHATS = 32;
    AppConfig data; bool ready = false;
    void setDefaults(); void clearChats(); bool addChatInternal(const String&, const String&);
};
extern ConfigManager config;
