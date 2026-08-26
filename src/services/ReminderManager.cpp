#include "ReminderManager.h"
#include <SD.h>
#include <ArduinoJson.h>
#include "../services/TimeManager.h"
#include "../network/TelegramManager.h"

static const char* REMINDERS_FILE="/data/reminders.json";
ReminderManager reminderManager;

bool ReminderManager::begin(){Serial.println("[REMINDERS] Initializing...");if(!load()){Serial.println("[REMINDERS] Failed to load reminders");return false;}Serial.printf("[REMINDERS] Loaded: %u\n",(unsigned)reminderCount);return true;}
void ReminderManager::update(){if(!timeManager.isTimeValid()||millis()-lastCheck<5000)return;lastCheck=millis();time_t now=timeManager.getTimestamp();if(now<=0)return;for(size_t i=0;i<reminderCount;i++)checkReminder(reminders[i],now);}

uint32_t ReminderManager::addReminder(Reminder reminder){if(reminderCount>=MAX_REMINDERS)return 0;reminder.id=nextId++;reminder.enabled=true;reminder.lastTriggered=0;reminder.nextTrigger=0;if(timeManager.isTimeValid())reminder.nextTrigger=calculateNextTrigger(reminder,timeManager.getTimestamp());reminders[reminderCount++]=reminder;if(!save()){reminderCount--;return 0;}return reminder.id;}
bool ReminderManager::updateReminder(const Reminder& reminder){for(size_t i=0;i<reminderCount;i++)if(reminders[i].id==reminder.id){reminders[i]=reminder;return save();}return false;}
bool ReminderManager::deleteReminder(uint32_t id){for(size_t i=0;i<reminderCount;i++)if(reminders[i].id==id){for(size_t j=i+1;j<reminderCount;j++)reminders[j-1]=reminders[j];reminderCount--;return save();}return false;}
bool ReminderManager::setEnabled(uint32_t id,bool enabled){for(size_t i=0;i<reminderCount;i++)if(reminders[i].id==id){reminders[i].enabled=enabled;if(enabled){reminders[i].nextTrigger=0;reminders[i].lastTriggered=0;}return save();}return false;}
size_t ReminderManager::getCount()const{return reminderCount;}
Reminder ReminderManager::getReminder(size_t i)const{return i<reminderCount?reminders[i]:Reminder();}
bool ReminderManager::getReminderById(uint32_t id,Reminder& r)const{for(size_t i=0;i<reminderCount;i++)if(reminders[i].id==id){r=reminders[i];return true;}return false;}

void ReminderManager::checkReminder(Reminder& r,time_t now){if(!r.enabled)return;if(r.nextTrigger==0){r.nextTrigger=calculateNextTrigger(r,now);save();return;}if(now<r.nextTrigger)return;if(!r.chatId.length()||!telegram.isStarted())return;if(!telegram.sendMessage(r.chatId,r.message,r.silent))return;r.lastTriggered=now;if(r.type==ReminderType::ONCE){r.enabled=false;r.nextTrigger=0;}else r.nextTrigger=calculateNextTrigger(r,now+1);save();}

static time_t makeLocal(int y,int mon,int d,int h,int min,int sec=0){struct tm t={};t.tm_year=y-1900;t.tm_mon=mon-1;t.tm_mday=d;t.tm_hour=h;t.tm_min=min;t.tm_sec=sec;t.tm_isdst=-1;return mktime(&t);}

time_t ReminderManager::calculateScheduledTime(Reminder& r,time_t ref){struct tm t;localtime_r(&ref,&t);int y=t.tm_year+1900,m=t.tm_mon+1,d=t.tm_mday;if(r.type==ReminderType::ONCE){y=r.year;m=r.month;d=r.day;}else if(r.type==ReminderType::MONTHLY)d=r.day;else if(r.type==ReminderType::YEARLY){m=r.month;d=r.day;}return makeLocal(y,m,d,r.hour,r.minute);}

time_t ReminderManager::calculateNextTrigger(Reminder& r,time_t from){if(r.type==ReminderType::ONCE){time_t x=makeLocal(r.year,r.month,r.day,r.hour,r.minute);return x>=from?x:0;}struct tm t;localtime_r(&from,&t);if(r.type==ReminderType::DAILY){time_t x=makeLocal(t.tm_year+1900,t.tm_mon+1,t.tm_mday,r.hour,r.minute);if(x<from)x+=86400;return x;}if(r.type==ReminderType::WEEKLY){int cur=t.tm_wday;int delta=(r.weekday-cur+7)%7;time_t x=makeLocal(t.tm_year+1900,t.tm_mon+1,t.tm_mday,r.hour,r.minute)+delta*86400;if(x<from)x+=7*86400;return x;}if(r.type==ReminderType::MONTHLY){int y=t.tm_year+1900,m=t.tm_mon+1;time_t x=makeLocal(y,m,r.day,r.hour,r.minute);if(x<from){m++;if(m>12){m=1;y++;}x=makeLocal(y,m,r.day,r.hour,r.minute);}return x;}if(r.type==ReminderType::YEARLY){int y=t.tm_year+1900;time_t x=makeLocal(y,r.month,r.day,r.hour,r.minute);if(x<from)x=makeLocal(y+1,r.month,r.day,r.hour,r.minute);return x;}return 0;}
void ReminderManager::advanceReminder(Reminder& r,time_t now){r.nextTrigger=calculateNextTrigger(r,now+1);}

const char* ReminderManager::typeToString(ReminderType t)const{switch(t){case ReminderType::DAILY:return "daily";case ReminderType::WEEKLY:return "weekly";case ReminderType::MONTHLY:return "monthly";case ReminderType::YEARLY:return "yearly";default:return "once";}}
ReminderType ReminderManager::stringToType(const String& s)const{if(s=="daily")return ReminderType::DAILY;if(s=="weekly")return ReminderType::WEEKLY;if(s=="monthly")return ReminderType::MONTHLY;if(s=="yearly")return ReminderType::YEARLY;return ReminderType::ONCE;}

bool ReminderManager::load(){if(!SD.exists(REMINDERS_FILE)){JsonDocument d;d["nextId"]=1;d["reminders"].to<JsonArray>();File f=SD.open(REMINDERS_FILE,FILE_WRITE);if(!f)return false;serializeJson(d,f);f.close();return true;}File f=SD.open(REMINDERS_FILE,FILE_READ);if(!f)return false;JsonDocument d;auto e=deserializeJson(d,f);f.close();if(e)return false;nextId=d["nextId"]|1;reminderCount=0;for(JsonObject o:d["reminders"].as<JsonArray>()){if(reminderCount>=MAX_REMINDERS)break;Reminder& r=reminders[reminderCount];r.id=o["id"]|0;r.enabled=o["enabled"]|true;r.type=stringToType(String(o["type"]|"once"));r.year=o["year"]|0;r.month=o["month"]|0;r.day=o["day"]|0;r.weekday=o["weekday"]|0;r.hour=o["hour"]|0;r.minute=o["minute"]|0;r.message=String(o["message"]|"");r.chatId=String(o["chatId"]|"");r.silent=o["silent"]|false;r.lastTriggered=o["lastTriggered"]|0;r.nextTrigger=o["nextTrigger"]|0;reminderCount++;}return true;}

bool ReminderManager::save(){JsonDocument d;d["nextId"]=nextId;JsonArray a=d["reminders"].to<JsonArray>();for(size_t i=0;i<reminderCount;i++){Reminder&r=reminders[i];JsonObject o=a.add<JsonObject>();o["id"]=r.id;o["enabled"]=r.enabled;o["type"]=typeToString(r.type);o["year"]=r.year;o["month"]=r.month;o["day"]=r.day;o["weekday"]=r.weekday;o["hour"]=r.hour;o["minute"]=r.minute;o["message"]=r.message;o["chatId"]=r.chatId;o["silent"]=r.silent;o["lastTriggered"]=r.lastTriggered;o["nextTrigger"]=r.nextTrigger;}const char* tmp="/data/reminders.tmp";if(SD.exists(tmp))SD.remove(tmp);File f=SD.open(tmp,FILE_WRITE);if(!f)return false;size_t n=serializeJsonPretty(d,f);f.flush();f.close();if(!n){SD.remove(tmp);return false;}if(SD.exists(REMINDERS_FILE)&&!SD.remove(REMINDERS_FILE))return false;return SD.rename(tmp,REMINDERS_FILE);}
