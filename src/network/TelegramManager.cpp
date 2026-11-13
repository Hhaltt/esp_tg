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
  client.setTimeout(1000);

  if (!client.connect("api.telegram.org", 443, 5000)) {
    ;
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

  // Consume headers with a total deadline; a stalled peer must not starve loop().
  unsigned long headersStartedAt = millis();
  uint8_t headerTerminator = 0;
  while ((client.connected() || client.available()) &&
         millis() - headersStartedAt < 2000 &&
         headerTerminator < 4) {
    if (!client.available()) {
      delay(1);
      continue;
    }

    char ch = static_cast<char>(client.read());
    if ((headerTerminator == 0 || headerTerminator == 2) && ch == '\r')
      ++headerTerminator;
    else if ((headerTerminator == 1 || headerTerminator == 3) && ch == '\n')
      ++headerTerminator;
    else
      headerTerminator = ch == '\r' ? 1 : 0;
  }

  String response;
  unsigned long startedAt = millis();
  while ((client.connected() || client.available()) && millis() - startedAt < 4000) {
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
    ;
    return true;
  }

  ;
  ;
  if (jsonError) {
    ;
  } else {
    int errorCode = doc["error_code"] | 0;
    const char* description = doc["description"] | "No error description";
    ;
    ;
    ;
    ;
  }
  return false;
}
}


WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);
TelegramManager telegram;

bool TelegramManager::reloadToken(){String token=config.getBotToken();token.trim();if(!token.length())return false;bot.updateToken(token);bot.last_message_received=0;lastCheck=0;started=ethernet.isConnected();;return true;}
void TelegramManager::begin(){telegramClient.setInsecure();telegramClient.setTimeout(5000);if(!reloadToken());;}
void TelegramManager::update()
{
    if (!ethernet.isConnected())
    {
        started = false;
        return;
    }

    if (!started)
    {
        started = true;
        ;
    }

    // Send startup notices one per loop iteration. A multi-chat burst can keep
    // the main task busy long enough to trip the task watchdog.
    if (!startupMessageSent)
    {
        sendStartupMessage();
        return;
    }

    if (millis() - lastCheck >= TELEGRAM_CHECK_INTERVAL)
    {
        lastCheck = millis();
        checkMessages();
    }
}

void TelegramManager::sendStartupMessage()
{
    if (startupMessageSent)
        return;
    if (!config.isStartupMessageEnabled() || !config.getChatCount())
    {
        startupMessageSent = true;
        return;
    }

    String message = config.getStartupMessage();
    message.trim();
    if (!message.length())
    {
        startupMessageSent = true;
        return;
    }

    if (startupChatIndex >= config.getChatCount())
    {
        startupMessageSent = true;
        return;
    }

    TelegramChatConfig chat = config.getChat(startupChatIndex++);
    // Treat startup notices as best-effort; a failed network send must not
    // restart the entire sequence on every pass through loop().
    sendMessage(chat.id, message);
    if (startupChatIndex >= config.getChatCount())
        startupMessageSent = true;
}
void TelegramManager::checkMessages()
{
    // UniversalTelegramBot is configured for one message per poll. Process only
    // that batch and return to loop() so the watchdog and other services run.
    int count = bot.getUpdates(bot.last_message_received + 1);
    for (int i = 0; i < count; ++i)
        handleMessage(i);
}
void TelegramManager::handleMessage(int i){statistics.onTelegramMessageReceived();String chatId=bot.messages[i].chat_id;String text=bot.messages[i].text;text.trim();if(!text.length())return;if(text.startsWith("/")){statistics.onCommandReceived();handleCommand(chatId,text);return;}String reply;if(commandRouteManager.execute(text,reply)){statistics.onCommandReceived();statistics.onCommandExecuted();sendMessage(chatId,reply);}}
void TelegramManager::handleCommand(const String& chatId,const String& command){if(command=="/start"){statistics.onCommandExecuted();sendStartMessage(chatId);return;}if(command=="/about"){statistics.onCommandExecuted();sendAbout(chatId);return;}if(command=="/ping"){statistics.onCommandExecuted();sendMessage(chatId,"🏓 Pong!");return;}if(command=="/time"){statistics.onCommandExecuted();sendMessage(chatId,"🕐 "+timeManager.getDateTimeString());return;}if(command=="/status"){statistics.onCommandExecuted();String message;message+="🤖 "+config.getDeviceName();message+="\n\n🟢 Online\n";message+="🌐 IP: "+ethernet.getIP();message+="\n🔗 Ethernet: ";message+=ethernet.isConnected()?"Online":"Offline";message+="\n⚡ "+String(ethernet.getLinkSpeed())+" Mbps\n";message+="⏱ Uptime: "+statistics.getCurrentUptimeString();sendMessage(chatId,message);return;}statistics.onCommandError();sendMessage(chatId,"❓ Невідома команда:\n"+command+"\n\nСпробуй /start");}
void TelegramManager::sendStartMessage(const String& chatId){String message="🤖 "+config.getDeviceName()+"\n\n";message+="Я запущений і працюю 😎\n\n";message+="📊 /about - про себе\n📡 /status - статус\n🏓 /ping - перевірка\n🕐 /time - час";sendMessage(chatId,message);}
void TelegramManager::sendAbout(const String& chatId){String message;message+="🤖 <b>"+config.getDeviceName()+"</b>\n\n";message+="🟢 <b>Стан:</b> Online\n";message+="⏱ <b>Поточний uptime:</b> "+statistics.getCurrentUptimeString()+"\n";message+="🕰 <b>Total uptime:</b> "+statistics.getTotalUptimeString()+"\n";message+="🔄 <b>Перезапусків:</b> "+String(statistics.getBootCount())+"\n\n";message+="💬 <b>Повідомлень:</b>\n📥 Отримано: "+String(statistics.getMessagesReceived())+"\n📤 Надіслано: "+String(statistics.getMessagesSent())+"\n\n";message+="⚙️ <b>Команди:</b>\n📥 Отримано: "+String(statistics.getCommandsReceived())+"\n✅ Виконано: "+String(statistics.getCommandsExecuted())+"\n❌ Помилок: "+String(statistics.getCommandErrors())+"\n\n";message+="💾 <b>Система:</b>\n🧠 Free heap: "+String(ESP.getFreeHeap())+" bytes\n";message+="🌐 IP: "+ethernet.getIP()+"\n";message+="❗ Помилок: "+String(statistics.getErrors());if(timeManager.isTimeValid())message+="\n\n🕐 "+timeManager.getDateTimeString();sendMessage(chatId,message);}
bool TelegramManager::sendMessage(const String& chatId,const String& text,bool silent){if(!chatId.length())return false;DynamicJsonDocument payload(2048);payload["chat_id"]=chatId;payload["text"]=text;payload["parse_mode"]="HTML";if(silent)payload["disable_notification"]=true;String token=config.getBotToken();token.trim();bool result=token.length()&&sendTelegramMessageWithDiagnostics(token,payload.as<JsonObject>());if(result)statistics.onTelegramMessageSent();else{statistics.onError();;}return result;}
bool TelegramManager::isStarted(){return started;}
