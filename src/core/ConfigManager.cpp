#include "ConfigManager.h"

#include <SD.h>
#include <ArduinoJson.h>
#include "../storage/SDCardManager.h"
#include "Config.h"

static const char* CONFIG_FILE = "/data/config.json";
ConfigManager config;

void ConfigManager::clearChats()
{
    if (data.chats)
    {
        delete[] data.chats;
        data.chats = nullptr;
    }
    data.chatCount = 0;
}

bool ConfigManager::addChatInternal(const String& id, const String& name)
{
    if (id.length() == 0 || data.chatCount >= MAX_CHATS)
        return false;

    TelegramChatConfig* next = new TelegramChatConfig[data.chatCount + 1];
    for (size_t i = 0; i < data.chatCount; i++)
        next[i] = data.chats[i];

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
    data.gpio35.enabled = false;
    data.gpio35.highMessage = "";
    data.gpio35.lowMessage = "";
    data.gpio39.enabled = false;
    data.gpio39.highMessage = "";
    data.gpio39.lowMessage = "";
}

bool ConfigManager::begin()
{
    if (!sdCard.isAvailable()) return false;
    setDefaults();
    if (!load()) return false;
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
    JsonArray chats = doc["telegram"]["chats"];
    if (!chats.isNull())
    {
        for (JsonObject chat : chats)
            addChatInternal(String(chat["id"] | ""), String(chat["name"] | ""));
    }

    // Migrate old installations that had one hardcoded/default chat.
    if (data.chatCount == 0 && strlen(TELEGRAM_CHAT_ID) > 0)
        addChatInternal(TELEGRAM_CHAT_ID, "Default");

    JsonObject g35 = doc["gpio35"];
    if (!g35["enabled"].isNull()) data.gpio35.enabled = g35["enabled"].as<bool>();
    if (g35["highMessage"].is<const char*>()) data.gpio35.highMessage = g35["highMessage"].as<const char*>();
    if (g35["lowMessage"].is<const char*>()) data.gpio35.lowMessage = g35["lowMessage"].as<const char*>();

    JsonObject g39 = doc["gpio39"];
    if (!g39["enabled"].isNull()) data.gpio39.enabled = g39["enabled"].as<bool>();
    if (g39["highMessage"].is<const char*>()) data.gpio39.highMessage = g39["highMessage"].as<const char*>();
    if (g39["lowMessage"].is<const char*>()) data.gpio39.lowMessage = g39["lowMessage"].as<const char*>();

    if (data.system.deviceName.length() == 0) data.system.deviceName = "Emily";
    if (data.system.timezone.length() == 0) data.system.timezone = "Europe/Kyiv";
    if (data.system.ntpServer.length() == 0) data.system.ntpServer = NTP_SERVER_1;
    return true;
}

bool ConfigManager::save()
{
    if (!sdCard.isAvailable()) return false;

    JsonDocument doc;
    doc["system"]["deviceName"] = data.system.deviceName;
    doc["system"]["timezone"] = data.system.timezone;
    doc["system"]["ntpServer"] = data.system.ntpServer;
    doc["system"]["botToken"] = data.system.botToken;
    doc["system"]["hubApiKey"] = data.system.hubApiKey;
    doc["system"]["startupMessageEnabled"] = data.system.startupMessageEnabled;
    doc["system"]["startupMessage"] = data.system.startupMessage;

    JsonArray chats = doc["telegram"]["chats"].to<JsonArray>();
    for (size_t i = 0; i < data.chatCount; i++)
    {
        JsonObject chat = chats.add<JsonObject>();
        chat["id"] = data.chats[i].id;
        chat["name"] = data.chats[i].name;
    }

    doc["gpio35"]["enabled"] = data.gpio35.enabled;
    doc["gpio35"]["highMessage"] = data.gpio35.highMessage;
    doc["gpio35"]["lowMessage"] = data.gpio35.lowMessage;
    doc["gpio39"]["enabled"] = data.gpio39.enabled;
    doc["gpio39"]["highMessage"] = data.gpio39.highMessage;
    doc["gpio39"]["lowMessage"] = data.gpio39.lowMessage;

    const char* tempFile = "/data/config.tmp";
    if (SD.exists(tempFile)) SD.remove(tempFile);
    File file = SD.open(tempFile, FILE_WRITE);
    if (!file) return false;
    size_t written = serializeJsonPretty(doc, file);
    file.flush();
    file.close();
    if (written == 0) { SD.remove(tempFile); return false; }
    if (SD.exists(CONFIG_FILE) && !SD.remove(CONFIG_FILE)) return false;
    if (!SD.rename(tempFile, CONFIG_FILE)) return false;
    return true;
}

String ConfigManager::getDeviceName() const { return data.system.deviceName; }
String ConfigManager::getTimezone() const { return data.system.timezone; }
String ConfigManager::getNtpServer() const { return data.system.ntpServer; }
String ConfigManager::getBotToken() const { return data.system.botToken; }
String ConfigManager::getHubApiKey() const { return data.system.hubApiKey; }
bool ConfigManager::isStartupMessageEnabled() const { return data.system.startupMessageEnabled; }
String ConfigManager::getStartupMessage() const { return data.system.startupMessage; }
void ConfigManager::setDeviceName(const String& value) { data.system.deviceName = value; }
void ConfigManager::setTimezone(const String& value) { data.system.timezone = value; }
void ConfigManager::setNtpServer(const String& value) { data.system.ntpServer = value; }
void ConfigManager::setBotToken(const String& value) { data.system.botToken = value; }
void ConfigManager::setHubApiKey(const String& value) { data.system.hubApiKey = value; }
void ConfigManager::setStartupMessageEnabled(bool value) { data.system.startupMessageEnabled = value; }
void ConfigManager::setStartupMessage(const String& value) { data.system.startupMessage = value; }

size_t ConfigManager::getChatCount() const { return data.chatCount; }
TelegramChatConfig ConfigManager::getChat(size_t index) const { return index < data.chatCount ? data.chats[index] : TelegramChatConfig(); }
String ConfigManager::getChatName(const String& id) const { for (size_t i = 0; i < data.chatCount; i++) if (data.chats[i].id == id) return data.chats[i].name; return id; }
bool ConfigManager::hasChat(const String& id) const { for (size_t i = 0; i < data.chatCount; i++) if (data.chats[i].id == id) return true; return false; }
bool ConfigManager::addChat(const String& id, const String& name) { if (hasChat(id)) return false; return addChatInternal(id, name); }
bool ConfigManager::updateChat(size_t index, const String& id, const String& name) { if (index >= data.chatCount || id.length() == 0) return false; for (size_t i=0;i<data.chatCount;i++) if (i!=index && data.chats[i].id==id) return false; data.chats[index].id=id; data.chats[index].name=name.length()?name:id; return true; }
bool ConfigManager::deleteChat(size_t index) { if (index >= data.chatCount) return false; for (size_t i=index+1;i<data.chatCount;i++) data.chats[i-1]=data.chats[i]; data.chatCount--; if (data.chatCount==0) { delete[] data.chats; data.chats=nullptr; } return true; }

bool ConfigManager::isGpio35Enabled() const { return data.gpio35.enabled; }
String ConfigManager::getGpio35HighMessage() const { return data.gpio35.highMessage; }
String ConfigManager::getGpio35LowMessage() const { return data.gpio35.lowMessage; }
void ConfigManager::setGpio35Enabled(bool value) { data.gpio35.enabled = value; }
void ConfigManager::setGpio35HighMessage(const String& value) { data.gpio35.highMessage = value; }
void ConfigManager::setGpio35LowMessage(const String& value) { data.gpio35.lowMessage = value; }
bool ConfigManager::isGpio39Enabled() const { return data.gpio39.enabled; }
String ConfigManager::getGpio39HighMessage() const { return data.gpio39.highMessage; }
String ConfigManager::getGpio39LowMessage() const { return data.gpio39.lowMessage; }
void ConfigManager::setGpio39Enabled(bool value) { data.gpio39.enabled = value; }
void ConfigManager::setGpio39HighMessage(const String& value) { data.gpio39.highMessage = value; }
void ConfigManager::setGpio39LowMessage(const String& value) { data.gpio39.lowMessage = value; }
