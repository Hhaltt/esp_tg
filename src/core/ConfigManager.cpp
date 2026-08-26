#include "ConfigManager.h"

#include <SD.h>
#include <ArduinoJson.h>

#include "../storage/SDCardManager.h"
#include "Config.h"

static const char* CONFIG_FILE = "/data/config.json";
ConfigManager config;

void ConfigManager::clearChats()
{
    delete[] data.chats;
    data.chats = nullptr;
    data.chatCount = 0;
}

bool ConfigManager::addChatInternal(const String& id, const String& name)
{
    if (!id.length()) return false;

    TelegramChatConfig* next = new TelegramChatConfig[data.chatCount + 1];
    if (!next) return false;

    for (size_t i = 0; i < data.chatCount; i++) next[i] = data.chats[i];
    next[data.chatCount].id = id;
    next[data.chatCount].name = name.length() ? name : id;

    delete[] data.chats;
    data.chats = next;
    data.chatCount++;
    return true;
}

void ConfigManager::setDefaults()
{
    clearChats();
    data.system.deviceName = "Emily";
    data.system.timezone = "Europe/Kyiv";
    data.system.ntpServer = NTP_SERVER_1;
    data.system.botToken = BOT_TOKEN;
    data.system.hubApiKey = HUB_API_KEY;
    data.system.startupMessageEnabled = true;
    data.system.startupMessage = "Emily is online 🤖";
    data.weather = WeatherConfig();
    data.gpio35 = {false, "", ""};
    data.gpio39 = {false, "", ""};
}

bool ConfigManager::begin()
{
    if (!sdCard.isAvailable()) return false;
    setDefaults();
    load(); // Defaults are valid when config file does not exist yet.
    ready = true;
    return true;
}

bool ConfigManager::load()
{
    if (!SD.exists(CONFIG_FILE)) return false;

    File file = SD.open(CONFIG_FILE, FILE_READ);
    if (!file) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) return false;

    JsonObject system = doc["system"];
    if (system["deviceName"].is<const char*>()) data.system.deviceName = system["deviceName"].as<const char*>();
    if (system["timezone"].is<const char*>()) data.system.timezone = system["timezone"].as<const char*>();
    if (system["ntpServer"].is<const char*>()) data.system.ntpServer = system["ntpServer"].as<const char*>();
    if (system["botToken"].is<const char*>()) data.system.botToken = system["botToken"].as<const char*>();
    if (system["hubApiKey"].is<const char*>()) data.system.hubApiKey = system["hubApiKey"].as<const char*>();
    if (!system["startupMessageEnabled"].isNull()) data.system.startupMessageEnabled = system["startupMessageEnabled"].as<bool>();
    if (system["startupMessage"].is<const char*>()) data.system.startupMessage = system["startupMessage"].as<const char*>();

    clearChats();
    JsonArray chats = doc["telegram"]["chats"].as<JsonArray>();
    for (JsonObject chat : chats)
        addChatInternal(String(chat["id"] | ""), String(chat["name"] | ""));
    if (!data.chatCount && strlen(TELEGRAM_CHAT_ID)) addChatInternal(TELEGRAM_CHAT_ID, "Default");

    JsonObject g35 = doc["gpio35"];
    if (!g35["enabled"].isNull()) data.gpio35.enabled = g35["enabled"].as<bool>();
    if (g35["highMessage"].is<const char*>()) data.gpio35.highMessage = g35["highMessage"].as<const char*>();
    if (g35["lowMessage"].is<const char*>()) data.gpio35.lowMessage = g35["lowMessage"].as<const char*>();

    JsonObject g39 = doc["gpio39"];
    if (!g39["enabled"].isNull()) data.gpio39.enabled = g39["enabled"].as<bool>();
    if (g39["highMessage"].is<const char*>()) data.gpio39.highMessage = g39["highMessage"].as<const char*>();
    if (g39["lowMessage"].is<const char*>()) data.gpio39.lowMessage = g39["lowMessage"].as<const char*>();

    if (!data.system.deviceName.length()) data.system.deviceName = "Emily";
    if (!data.system.timezone.length()) data.system.timezone = "Europe/Kyiv";
    if (!data.system.ntpServer.length()) data.system.ntpServer = NTP_SERVER_1;
    return true;
}

bool ConfigManager::save()
{
    if (!sdCard.isAvailable()) return false;

    JsonDocument doc;
    JsonObject system = doc["system"].to<JsonObject>();
    system["deviceName"] = data.system.deviceName;
    system["timezone"] = data.system.timezone;
    system["ntpServer"] = data.system.ntpServer;
    system["botToken"] = data.system.botToken;
    system["hubApiKey"] = data.system.hubApiKey;
    system["startupMessageEnabled"] = data.system.startupMessageEnabled;
    system["startupMessage"] = data.system.startupMessage;

    JsonArray chats = doc["telegram"]["chats"].to<JsonArray>();
    for (size_t i = 0; i < data.chatCount; i++)
    {
        JsonObject chat = chats.add<JsonObject>();
        chat["id"] = data.chats[i].id;
        chat["name"] = data.chats[i].name;
    }

    JsonObject g35 = doc["gpio35"].to<JsonObject>();
    g35["enabled"] = data.gpio35.enabled;
    g35["highMessage"] = data.gpio35.highMessage;
    g35["lowMessage"] = data.gpio35.lowMessage;

    JsonObject g39 = doc["gpio39"].to<JsonObject>();
    g39["enabled"] = data.gpio39.enabled;
    g39["highMessage"] = data.gpio39.highMessage;
    g39["lowMessage"] = data.gpio39.lowMessage;

    const char* tmp = "/data/config.tmp";
    if (SD.exists(tmp)) SD.remove(tmp);
    File file = SD.open(tmp, FILE_WRITE);
    if (!file) return false;
    size_t written = serializeJsonPretty(doc, file);
    file.flush();
    file.close();
    if (!written) { SD.remove(tmp); return false; }
    if (SD.exists(CONFIG_FILE) && !SD.remove(CONFIG_FILE)) return false;
    return SD.rename(tmp, CONFIG_FILE);
}

String ConfigManager::getDeviceName() const { return data.system.deviceName; }
String ConfigManager::getTimezone() const { return data.system.timezone; }
String ConfigManager::getNtpServer() const { return data.system.ntpServer; }
String ConfigManager::getBotToken() const { return data.system.botToken; }
String ConfigManager::getHubApiKey() const { return data.system.hubApiKey; }
bool ConfigManager::isStartupMessageEnabled() const { return data.system.startupMessageEnabled; }
String ConfigManager::getStartupMessage() const { return data.system.startupMessage; }
void ConfigManager::setDeviceName(const String& v) { data.system.deviceName = v; }
void ConfigManager::setTimezone(const String& v) { data.system.timezone = v; }
void ConfigManager::setNtpServer(const String& v) { data.system.ntpServer = v; }
void ConfigManager::setBotToken(const String& v) { data.system.botToken = v; }
void ConfigManager::setHubApiKey(const String& v) { data.system.hubApiKey = v; }
void ConfigManager::setStartupMessageEnabled(bool v) { data.system.startupMessageEnabled = v; }
void ConfigManager::setStartupMessage(const String& v) { data.system.startupMessage = v; }

size_t ConfigManager::getChatCount() const { return data.chatCount; }
TelegramChatConfig ConfigManager::getChat(size_t i) const { return i < data.chatCount ? data.chats[i] : TelegramChatConfig(); }
String ConfigManager::getChatName(const String& id) const { for (size_t i=0;i<data.chatCount;i++) if (data.chats[i].id==id) return data.chats[i].name; return id; }
bool ConfigManager::hasChat(const String& id) const { for (size_t i=0;i<data.chatCount;i++) if (data.chats[i].id==id) return true; return false; }
bool ConfigManager::addChat(const String& id, const String& name) { return !hasChat(id) && addChatInternal(id, name); }
bool ConfigManager::updateChat(size_t i, const String& id, const String& name) { if (i>=data.chatCount || !id.length()) return false; for(size_t j=0;j<data.chatCount;j++) if(j!=i && data.chats[j].id==id) return false; data.chats[i].id=id; data.chats[i].name=name.length()?name:id; return true; }
bool ConfigManager::deleteChat(size_t i) { if(i>=data.chatCount) return false; for(size_t j=i+1;j<data.chatCount;j++) data.chats[j-1]=data.chats[j]; data.chatCount--; if(!data.chatCount){ delete[] data.chats; data.chats=nullptr; return true; } TelegramChatConfig* next=new TelegramChatConfig[data.chatCount]; if(!next) return false; for(size_t j=0;j<data.chatCount;j++) next[j]=data.chats[j]; delete[] data.chats; data.chats=next; return true; }

bool ConfigManager::isWeatherEnabled() const { return false; }
String ConfigManager::getWeatherApiKey() const { return ""; }
String ConfigManager::getWeatherCity() const { return ""; }
uint8_t ConfigManager::getWeatherHour() const { return 0; }
uint8_t ConfigManager::getWeatherMinute() const { return 0; }
void ConfigManager::setWeatherEnabled(bool) {}
void ConfigManager::setWeatherApiKey(const String&) {}
void ConfigManager::setWeatherCity(const String&) {}
void ConfigManager::setWeatherTime(uint8_t, uint8_t) {}

bool ConfigManager::isGpio35Enabled() const { return data.gpio35.enabled; }
String ConfigManager::getGpio35HighMessage() const { return data.gpio35.highMessage; }
String ConfigManager::getGpio35LowMessage() const { return data.gpio35.lowMessage; }
void ConfigManager::setGpio35Enabled(bool v) { data.gpio35.enabled=v; }
void ConfigManager::setGpio35HighMessage(const String& v) { data.gpio35.highMessage=v; }
void ConfigManager::setGpio35LowMessage(const String& v) { data.gpio35.lowMessage=v; }
bool ConfigManager::isGpio39Enabled() const { return data.gpio39.enabled; }
String ConfigManager::getGpio39HighMessage() const { return data.gpio39.highMessage; }
String ConfigManager::getGpio39LowMessage() const { return data.gpio39.lowMessage; }
void ConfigManager::setGpio39Enabled(bool v) { data.gpio39.enabled=v; }
void ConfigManager::setGpio39HighMessage(const String& v) { data.gpio39.highMessage=v; }
void ConfigManager::setGpio39LowMessage(const String& v) { data.gpio39.lowMessage=v; }
