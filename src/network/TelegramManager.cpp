#include "TelegramManager.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include "../core/Config.h"
#include "../core/ConfigManager.h"
#include "../core/Statistics.h"
#include "../services/TimeManager.h"
#include "EthernetManager.h"

WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);
TelegramManager telegram;

void TelegramManager::begin()
{
    telegramClient.setInsecure();
    String token = config.getBotToken(); token.trim();
    if (token.length()) bot.updateToken(token);
    Serial.println("[TG] Telegram initialized");
}

void TelegramManager::update()
{
    if (!ethernet.isConnected()) { started=false; return; }
    if (!started) { started=true; Serial.println("[TG] Telegram ready"); sendStartupMessage(); }
    if (millis()-lastCheck>=TELEGRAM_CHECK_INTERVAL) { lastCheck=millis(); checkMessages(); }
}

void TelegramManager::sendStartupMessage()
{
    if (startupMessageSent || !config.isStartupMessageEnabled()) return;
    String message=config.getStartupMessage(); message.trim();
    if (!message.length() || !config.getChatCount()) return;
    if (sendMessage(config.getChat(0).id,message)) startupMessageSent=true;
}

void TelegramManager::checkMessages(){int count=bot.getUpdates(bot.last_message_received+1);while(count>0){for(int i=0;i<count;i++)handleMessage(i);count=bot.getUpdates(bot.last_message_received+1);}}
void TelegramManager::handleMessage(int i){statistics.onTelegramMessageReceived();String id=bot.messages[i].chat_id,text=bot.messages[i].text;if(text.startsWith("/")){statistics.onCommandReceived();handleCommand(id,text);}else sendMessage(id,"Я поки що розумію тільки команди.\n\nСпробуй /start");}
void TelegramManager::handleCommand(const String& id,const String& c){if(c=="/start"){statistics.onCommandExecuted();sendStartMessage(id);return;}if(c=="/about"){statistics.onCommandExecuted();sendAbout(id);return;}if(c=="/ping"){statistics.onCommandExecuted();sendMessage(id,"🏓 Pong!");return;}if(c=="/time"){statistics.onCommandExecuted();sendMessage(id,"🕐 "+timeManager.getDateTimeString());return;}if(c=="/status"){statistics.onCommandExecuted();String m="🤖 "+config.getDeviceName()+"\n\n🟢 Online\n🌐 IP: "+ethernet.getIP()+"\n🔗 Ethernet: "+String(ethernet.isConnected()?"Online":"Offline")+"\n⚡ "+String(ethernet.getLinkSpeed())+" Mbps\n⏱ Uptime: "+statistics.getCurrentUptimeString();sendMessage(id,m);return;}statistics.onCommandError();sendMessage(id,"❓ Невідома команда:\n"+c+"\n\nСпробуй /start");}
void TelegramManager::sendStartMessage(const String& id){sendMessage(id,"🤖 "+config.getDeviceName()+"\n\nЯ запущений і працюю 😎\n\n📊 /about - про себе\n📡 /status - статус\n🏓 /ping - перевірка\n🕐 /time - час");}
void TelegramManager::sendAbout(const String& id){String m="🤖 <b>"+config.getDeviceName()+"</b>\n\n🟢 <b>Стан:</b> Online\n";m+="⏱ <b>Поточний uptime:</b> "+statistics.getCurrentUptimeString()+"\n";m+="🕰 <b>Total uptime:</b> "+statistics.getTotalUptimeString()+"\n";m+="🔄 <b>Перезапусків:</b> "+String(statistics.getBootCount())+"\n\n";m+="💬 <b>Повідомлень:</b>\n📥 Отримано: "+String(statistics.getMessagesReceived())+"\n📤 Надіслано: "+String(statistics.getMessagesSent());sendMessage(id,m);}

bool TelegramManager::sendMessage(const String& chatId,const String& text,bool silent)
{
    if(!chatId.length()) return false;
    DynamicJsonDocument payload(2048);
    payload["chat_id"]=chatId;
    payload["text"]=text;
    payload["parse_mode"]="HTML";
    if(silent) payload["disable_notification"]=true;
    bool result=bot.sendPostMessage(payload.as<JsonObject>());
    if(result) statistics.onTelegramMessageSent();
    else {statistics.onError();Serial.println("[TG] Failed to send message");}
    return result;
}

bool TelegramManager::isStarted(){return started;}
