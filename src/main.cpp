#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_idf_version.h>
#include <SD.h>
#include <ETH.h>

#include "core/Config.h"
#include "core/Statistics.h"
#include "core/ConfigManager.h"
#include "network/EthernetManager.h"
#include "network/WebServerManager.h"
#include "network/TelegramManager.h"
#include "services/TimeManager.h"
#include "services/ReminderManager.h"
#include "services/GpioMonitorManager.h"
#include "services/LedActivityManager.h"
#include "storage/SDCardManager.h"
#include "storage/StorageManager.h"

unsigned long lastStatisticsSave = 0;
static const uint32_t WATCHDOG_TIMEOUT_SECONDS = 15;

static String jsonEscape(String v){v.replace("\\", "\\\\");v.replace("\"", "\\\"");v.replace("\n", "\\n");v.replace("\r", "\\r");return v;}
static void beginWatchdog(){
#if ESP_IDF_VERSION_MAJOR >= 5
    esp_task_wdt_config_t configWdt = {};
    configWdt.timeout_ms = WATCHDOG_TIMEOUT_SECONDS * 1000;
    configWdt.idle_core_mask = 0;
    configWdt.trigger_panic = true;
    esp_task_wdt_init(&configWdt);
#else
    esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true);
#endif
    esp_task_wdt_add(NULL);
    Serial.printf("[WDT] Main loop watched, timeout: %lu s\n", (unsigned long)WATCHDOG_TIMEOUT_SECONDS);
}

static void streamSd(WebServer& server,const char* path,const char* type){if(!sdCard.isAvailable()||!SD.exists(path)){server.send(404,"text/plain",String("Missing ")+path+" on SD card");return;}File file=SD.open(path,FILE_READ);server.streamFile(file,type);file.close();}

static String buildStatusJson(){uint64_t total=sdCard.isAvailable()?SD.totalBytes():0,used=sdCard.isAvailable()?SD.usedBytes():0;String j="{";j+="\"deviceName\":\""+jsonEscape(config.getDeviceName())+"\",";j+="\"linkUp\":"+String(ETH.linkUp()?"true":"false")+",";j+="\"ip\":\""+ETH.localIP().toString()+"\",";j+="\"gateway\":\""+ETH.gatewayIP().toString()+"\",";j+="\"linkSpeed\":"+String(ETH.linkSpeed())+",";j+="\"fullDuplex\":"+String(ETH.fullDuplex()?"true":"false")+",";j+="\"time\":\""+jsonEscape(timeManager.getDateTimeString())+"\",";j+="\"totalUptime\":\""+statistics.getTotalUptimeString()+"\",";j+="\"currentUptime\":\""+statistics.getCurrentUptimeString()+"\",";j+="\"bootCount\":"+String(statistics.getBootCount())+",";j+="\"errors\":"+String(statistics.getErrors())+",";j+="\"messagesReceived\":"+String(statistics.getMessagesReceived())+",";j+="\"messagesSent\":"+String(statistics.getMessagesSent())+",";j+="\"commandsReceived\":"+String(statistics.getCommandsReceived())+",";j+="\"commandsExecuted\":"+String(statistics.getCommandsExecuted())+",";j+="\"commandErrors\":"+String(statistics.getCommandErrors())+",";j+="\"chatCount\":"+String(config.getChatCount())+",";j+="\"reminderCount\":"+String(reminderManager.getCount())+",";j+="\"sdAvailable\":"+String(sdCard.isAvailable()?"true":"false")+",";j+="\"sdTotal\":"+String((uint32_t)total)+",";j+="\"sdUsed\":"+String((uint32_t)used)+"}";return j;}

static String buildSettingsJson(){String j="{";j+="\"deviceName\":\""+jsonEscape(config.getDeviceName())+"\",";j+="\"timezone\":\""+jsonEscape(config.getTimezone())+"\",";j+="\"ntpServer\":\""+jsonEscape(config.getNtpServer())+"\",";j+="\"startupMessageEnabled\":"+String(config.isStartupMessageEnabled()?"true":"false")+",";j+="\"startupMessage\":\""+jsonEscape(config.getStartupMessage())+"\",";j+="\"gpio35Enabled\":"+String(config.isGpio35Enabled()?"true":"false")+",";j+="\"gpio35HighMessage\":\""+jsonEscape(config.getGpio35HighMessage())+"\",";j+="\"gpio35LowMessage\":\""+jsonEscape(config.getGpio35LowMessage())+"\",";j+="\"gpio39Enabled\":"+String(config.isGpio39Enabled()?"true":"false")+",";j+="\"gpio39HighMessage\":\""+jsonEscape(config.getGpio39HighMessage())+"\",";j+="\"gpio39LowMessage\":\""+jsonEscape(config.getGpio39LowMessage())+"\",";j+="\"chats\":[";for(size_t i=0;i<config.getChatCount();i++){if(i)j+=",";auto c=config.getChat(i);j+="{\"index\":"+String(i)+",\"id\":\""+jsonEscape(c.id)+"\",\"name\":\""+jsonEscape(c.name)+"\"}";}j+="]}";return j;}

static String buildChatsJson(){String j="{\"items\":[";for(size_t i=0;i<config.getChatCount();i++){if(i)j+=",";auto c=config.getChat(i);j+="{\"id\":\""+jsonEscape(c.id)+"\",\"name\":\""+jsonEscape(c.name)+"\"}";}return j+"]}";}

static String buildRemindersJson(){String j="{\"count\":"+String(reminderManager.getCount())+",\"items\":[";for(size_t i=0;i<reminderManager.getCount();i++){if(i)j+=",";Reminder r=reminderManager.getReminder(i);j+="{\"id\":"+String(r.id)+",\"message\":\""+jsonEscape(r.message)+"\",\"chatId\":\""+jsonEscape(r.chatId)+"\",\"type\":"+String((int)r.type)+",\"year\":"+String(r.year)+",\"month\":"+String(r.month)+",\"day\":"+String(r.day)+",\"weekday\":"+String(r.weekday)+",\"hour\":"+String(r.hour)+",\"minute\":"+String(r.minute)+",\"enabled\":"+String(r.enabled?"true":"false")+",\"silent\":"+String(r.silent?"true":"false")+"}";}return j+"]}";}

static void setupSdWebExperiment(){
    WebServer& server=webServerManager.rawServer();
    server.on("/sd",HTTP_GET,[&server](){server.sendHeader("Location","/sd/");server.send(303);});
    server.on("/sd/",HTTP_GET,[&server](){streamSd(server,"/www/index.html","text/html; charset=utf-8");});
    server.on("/sd/index.html",HTTP_GET,[&server](){streamSd(server,"/www/index.html","text/html; charset=utf-8");});
    server.on("/sd/settings.html",HTTP_GET,[&server](){streamSd(server,"/www/settings.html","text/html; charset=utf-8");});
    server.on("/sd/reminders.html",HTTP_GET,[&server](){streamSd(server,"/www/reminders.html","text/html; charset=utf-8");});
    server.on("/sd/style.css",HTTP_GET,[&server](){streamSd(server,"/www/style.css","text/css; charset=utf-8");});
    server.on("/sd/app.js",HTTP_GET,[&server](){streamSd(server,"/www/app.js","application/javascript; charset=utf-8");});

    server.on("/api/status",HTTP_GET,[&server](){server.send(200,"application/json; charset=utf-8",buildStatusJson());});
    server.on("/api/settings",HTTP_GET,[&server](){server.send(200,"application/json; charset=utf-8",buildSettingsJson());});
    server.on("/api/reminders",HTTP_GET,[&server](){server.send(200,"application/json; charset=utf-8",buildRemindersJson());});

    // Versioned public API. No secrets are ever returned here.
    server.on("/api/v1/status",HTTP_GET,[&server](){server.send(200,"application/json; charset=utf-8",buildStatusJson());});
    server.on("/api/v1/chats",HTTP_GET,[&server](){server.send(200,"application/json; charset=utf-8",buildChatsJson());});

    Serial.println("[WEB] SD frontend experiment: /sd/");
}

void setup(){Serial.begin(115200);delay(2000);Serial.println();Serial.println();Serial.println("========================================");Serial.println(DEVICE_NAME);Serial.println("========================================");beginWatchdog();statistics.begin();ethernet.begin();sdCard.begin();if(storage.begin()){StatisticsData loadedData;storage.loadStatistics(loadedData);statistics.load(loadedData);statistics.onBoot();storage.saveStatistics(statistics.getData());}config.begin();reminderManager.begin();timeManager.begin();telegram.begin();setupSdWebExperiment();webServerManager.begin();gpioMonitor.begin();ledActivity.begin();Serial.println("[SYSTEM] Initialization complete");esp_task_wdt_reset();}

void loop(){esp_task_wdt_reset();ethernet.update();timeManager.update();telegram.update();esp_task_wdt_reset();gpioMonitor.update();webServerManager.update();statistics.update();reminderManager.update();ledActivity.update();if(sdCard.isAvailable()&&millis()-lastStatisticsSave>=STATISTICS_SAVE_INTERVAL){lastStatisticsSave=millis();Serial.println("[SYSTEM] Statistics checkpoint");storage.saveStatistics(statistics.getData());}esp_task_wdt_reset();delay(1);}
