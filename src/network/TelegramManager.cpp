#include "TelegramManager.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "../core/Config.h"
#include "../core/ConfigManager.h"
#include "../core/Statistics.h"
#include "../services/TimeManager.h"
#include "../services/CommandRouteManager.h"
#include "EthernetManager.h"

namespace {
bool sendTelegramMessageWithDiagnostics(const String& token, JsonObject payload) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(5000);

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("[TG] Send failed: TLS connection to api.telegram.org:443 failed");
    client.stop();
    return false;
  }

  String requestBody;
  serializeJson(payload, requestBody);

  client.print("POST /bot");
  client.print(token);
  client.println("/sendMessage HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(requestBody.length());
  client.println();
  client.print(requestBody);

  String statusLine = client.readStringUntil('\n');
  statusLine.trim();
  int httpStatus = 0;
  if (statusLine.startsWith("HTTP/")) {
    int firstSpace = statusLine.indexOf(' ');
    if (firstSpace >= 0) httpStatus = statusLine.substring(firstSpace + 1).toInt();
  }

  // Skip HTTP headers without logging request or sensitive data.
  while (client.connected() || client.available()) {
    String header = client.readStringUntil('\n');
    if (header == "\r" || header.length() == 0) break;
  }

  String response;
  unsigned long startedAt = millis();
  while ((client.connected() || client.available()) && millis() - startedAt < 6000) {
    while (client.available()) {
      response += static_cast<char>(client.read());
      if (response.length() >= 2048) break;
    }
    if (response.length() >= 2048) break;
    delay(1);
  }
  client.stop();

  DynamicJsonDocument doc(2048);
  DeserializationError jsonError = deserializeJson(doc, response);
  bool ok = !jsonError && doc["ok"] == true && httpStatus >= 200 && httpStatus < 300;
  if (ok) {
    Serial.println("[TG] Telegram API accepted message");
    return true;
  }

  Serial.print("[TG] Send failed; HTTP status: ");
  Serial.println(httpStatus);
  if (jsonError) {
    Serial.println(response.length()
      ? "[TG] Response was not valid JSON (response omitted)"
      : "[TG] Empty response from Telegram API");
  } else {
    int errorCode = doc["error_code"] | 0;
    const char* description = doc["description"] | "No error description";
    Serial.print("[TG] Telegram error ");
    Serial.print(errorCode);
    Serial.print(": ");
    Serial.println(description);
  }
  return false;
}
}


WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);
TelegramManager telegram;

bool TelegramManager::reloadToken(){String token=config.getBotToken();token.trim();if(!token.length())return false;bot.updateToken(token);bot.last_message_received=0;lastCheck=0;started=ethernet.isConnected();Serial.println("[TG] Telegram token reloaded");return true;}
void TelegramManager::begin(){telegramClient.setInsecure();telegramClient.setTimeout(5000);if(!reloadToken())Serial.println("[TG] Telegram token is empty");Serial.println("[TG] Telegram initialized");}
void TelegramManager::update(){if(!ethernet.isConnected()){started=false;return;}if(!started){started=true;Serial.println("[TG] Telegram ready");sendStartupMessage();}if(millis()-lastCheck>=TELEGRAM_CHECK_INTERVAL){lastCheck=millis();checkMessages();}}
void TelegramManager::sendStartupMessage(){if(startupMessageSent||!config.isStartupMessageEnabled())return;String message=config.getStartupMessage();message.trim();if(!message.length()||!config.getChatCount())return;bool allSent=true;for(size_t i=0;i<config.getChatCount();++i){TelegramChatConfig chat=config.getChat(i);if(!sendMessage(chat.id,message))allSent=false;}startupMessageSent=allSent;}
void TelegramManager::checkMessages(){int count=bot.getUpdates(bot.last_message_received+1);while(count>0){for(int i=0;i<count;i++)handleMessage(i);count=bot.getUpdates(bot.last_message_received+1);}}
void TelegramManager::handleMessage(int i){statistics.onTelegramMessageReceived();String chatId=bot.messages[i].chat_id;String text=bot.messages[i].text;text.trim();if(!text.length())return;if(text.startsWith("/")){statistics.onCommandReceived();handleCommand(chatId,text);return;}String reply;if(commandRouteManager.execute(text,reply)){statistics.onCommandReceived();statistics.onCommandExecuted();sendMessage(chatId,reply);}}
void TelegramManager::handleCommand(const String& chatId,const String& command){if(command=="/start"){statistics.onCommandExecuted();sendStartMessage(chatId);return;}if(command=="/about"){statistics.onCommandExecuted();sendAbout(chatId);return;}if(command=="/ping"){statistics.onCommandExecuted();sendMessage(chatId,"🏓 Pong!");return;}if(command=="/time"){statistics.onCommandExecuted();sendMessage(chatId,"🕐 "+timeManager.getDateTimeString());return;}if(command=="/status"){statistics.onCommandExecuted();String message;message+="🤖 "+config.getDeviceName();message+="\n\n🟢 Online\n";message+="🌐 IP: "+ethernet.getIP();message+="\n🔗 Ethernet: ";message+=ethernet.isConnected()?"Online":"Offline";message+="\n⚡ "+String(ethernet.getLinkSpeed())+" Mbps\n";message+="⏱ Uptime: "+statistics.getCurrentUptimeString();sendMessage(chatId,message);return;}statistics.onCommandError();sendMessage(chatId,"❓ Невідома команда:\n"+command+"\n\nСпробуй /start");}
void TelegramManager::sendStartMessage(const String& chatId){String message="🤖 "+config.getDeviceName()+"\n\n";message+="Я запущений і працюю 😎\n\n";message+="📊 /about - про себе\n📡 /status - статус\n🏓 /ping - перевірка\n🕐 /time - час";sendMessage(chatId,message);}
void TelegramManager::sendAbout(const String& chatId){String message;message+="🤖 <b>"+config.getDeviceName()+"</b>\n\n";message+="🟢 <b>Стан:</b> Online\n";message+="⏱ <b>Поточний uptime:</b> "+statistics.getCurrentUptimeString()+"\n";message+="🕰 <b>Total uptime:</b> "+statistics.getTotalUptimeString()+"\n";message+="🔄 <b>Перезапусків:</b> "+String(statistics.getBootCount())+"\n\n";message+="💬 <b>Повідомлень:</b>\n📥 Отримано: "+String(statistics.getMessagesReceived())+"\n📤 Надіслано: "+String(statistics.getMessagesSent())+"\n\n";message+="⚙️ <b>Команди:</b>\n📥 Отримано: "+String(statistics.getCommandsReceived())+"\n✅ Виконано: "+String(statistics.getCommandsExecuted())+"\n❌ Помилок: "+String(statistics.getCommandErrors())+"\n\n";message+="💾 <b>Система:</b>\n🧠 Free heap: "+String(ESP.getFreeHeap())+" bytes\n";message+="🌐 IP: "+ethernet.getIP()+"\n";message+="❗ Помилок: "+String(statistics.getErrors());if(timeManager.isTimeValid())message+="\n\n🕐 "+timeManager.getDateTimeString();sendMessage(chatId,message);}
bool TelegramManager::sendMessage(const String& chatId,const String& text,bool silent){if(!chatId.length())return false;DynamicJsonDocument payload(2048);payload["chat_id"]=chatId;payload["text"]=text;payload["parse_mode"]="HTML";if(silent)payload["disable_notification"]=true;String token=config.getBotToken();token.trim();bool result=token.length()&&sendTelegramMessageWithDiagnostics(token,payload.as<JsonObject>());if(result)statistics.onTelegramMessageSent();else{statistics.onError();Serial.println("[TG] Failed to send message");}return result;}
bool TelegramManager::isStarted(){return started;}
