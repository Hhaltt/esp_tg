#pragma once

#include <Arduino.h>

struct TelegramChatConfig { String id; String name; };
struct CommandRouteConfig
{
    String phrase;
    String url;
    String apiKey;
    String command;
};
struct WeatherConfig { bool enabled = false; String apiKey; String city; uint8_t hour = 7; uint8_t minute = 0; };
struct SystemConfig { String deviceName; String timezone; String ntpServer; String botToken; String hubApiKey; bool startupMessageEnabled; String startupMessage; };
struct GpioMonitorConfig { bool enabled; String highMessage; String lowMessage; };
struct AppConfig
{
    SystemConfig system;
    WeatherConfig weather;
    TelegramChatConfig* chats = nullptr;
    size_t chatCount = 0;
    CommandRouteConfig* commandRoutes = nullptr;
    size_t commandRouteCount = 0;
    GpioMonitorConfig gpio35;
    GpioMonitorConfig gpio39;
};

class ConfigManager
{
public:
    bool begin(); bool load(); bool save();
    String getDeviceName() const; String getTimezone() const; String getNtpServer() const;
    String getBotToken() const; String getHubApiKey() const; bool isStartupMessageEnabled() const; String getStartupMessage() const;
    void setDeviceName(const String&); void setTimezone(const String&); void setNtpServer(const String&);
    void setBotToken(const String&); void setHubApiKey(const String&); void setStartupMessageEnabled(bool); void setStartupMessage(const String&);

    size_t getChatCount() const; TelegramChatConfig getChat(size_t) const; String getChatName(const String&) const;
    bool hasChat(const String&) const; bool addChat(const String&, const String&); bool updateChat(size_t, const String&, const String&); bool deleteChat(size_t);

    size_t getCommandRouteCount() const;
    CommandRouteConfig getCommandRoute(size_t index) const;
    bool addCommandRoute(const String& phrase, const String& url, const String& apiKey, const String& command);
    bool updateCommandRoute(size_t index, const String& phrase, const String& url, const String& apiKey, const String& command);
    bool deleteCommandRoute(size_t index);

    bool isWeatherEnabled() const; String getWeatherApiKey() const; String getWeatherCity() const; uint8_t getWeatherHour() const; uint8_t getWeatherMinute() const;
    void setWeatherEnabled(bool); void setWeatherApiKey(const String&); void setWeatherCity(const String&); void setWeatherTime(uint8_t, uint8_t);

    bool isGpio35Enabled() const; String getGpio35HighMessage() const; String getGpio35LowMessage() const;
    void setGpio35Enabled(bool); void setGpio35HighMessage(const String&); void setGpio35LowMessage(const String&);
    bool isGpio39Enabled() const; String getGpio39HighMessage() const; String getGpio39LowMessage() const;
    void setGpio39Enabled(bool); void setGpio39HighMessage(const String&); void setGpio39LowMessage(const String&);

private:
    AppConfig data; bool ready = false;
    void setDefaults(); void clearChats(); void clearCommandRoutes();
    bool addChatInternal(const String&, const String&);
    bool addCommandRouteInternal(const String&, const String&, const String&, const String&);
};
extern ConfigManager config;
