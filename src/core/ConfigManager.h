#pragma once

#include <Arduino.h>

struct TelegramChatConfig
{
    String id;
    String name;
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

    void setDeviceName(const String& name);
    void setTimezone(const String& timezone);
    void setNtpServer(const String& server);
    void setBotToken(const String& token);
    void setHubApiKey(const String& key);
    void setStartupMessageEnabled(bool enabled);
    void setStartupMessage(const String& message);

    size_t getChatCount() const;
    TelegramChatConfig getChat(size_t index) const;
    String getChatName(const String& id) const;
    bool hasChat(const String& id) const;
    bool addChat(const String& id, const String& name);
    bool updateChat(size_t index, const String& id, const String& name);
    bool deleteChat(size_t index);

    bool isGpio35Enabled() const;
    String getGpio35HighMessage() const;
    String getGpio35LowMessage() const;
    void setGpio35Enabled(bool enabled);
    void setGpio35HighMessage(const String& message);
    void setGpio35LowMessage(const String& message);

    bool isGpio39Enabled() const;
    String getGpio39HighMessage() const;
    String getGpio39LowMessage() const;
    void setGpio39Enabled(bool enabled);
    void setGpio39HighMessage(const String& message);
    void setGpio39LowMessage(const String& message);

private:
    static const size_t MAX_CHATS = 32;
    AppConfig data;
    bool ready = false;

    void setDefaults();
    void clearChats();
    bool addChatInternal(const String& id, const String& name);
};

extern ConfigManager config;
