#include "TimeManager.h"
#include <time.h>
#include <stdlib.h>
#include "../core/Config.h"
#include "../core/ConfigManager.h"
#include "../network/EthernetManager.h"

TimeManager timeManager;

void TimeManager::begin(){ ; }
void TimeManager::update()
{
    String timezone = config.getTimezone();
    if (!timezone.length()) timezone = "Europe/Kyiv";
    String posixTimezone = timezone;
    if (timezone == "Europe/Kyiv" || timezone == "Europe/Kiev")
        posixTimezone = "EET-2EEST,M3.5.0/3,M10.5.0/4";
    else if (timezone == "UTC" || timezone == "Etc/UTC")
        posixTimezone = "UTC0";

    if (timezone != appliedTimezone)
    {
        setenv("TZ", posixTimezone.c_str(), 1);
        tzset();
        appliedTimezone = timezone;
        ;
    }

    if (!ethernet.isConnected()) return;
    unsigned long interval = timeValid ? NTP_SYNC_INTERVAL : NTP_RETRY_INTERVAL;
    if (lastSyncAttempt != 0 && millis() - lastSyncAttempt < interval) return;
    lastSyncAttempt = millis();
    ;
    String server = config.getNtpServer();
    if (!server.length()) server = NTP_SERVER_1;
    // The TZ environment controls local time and DST; NTP only sets UTC.
    configTime(0, 0, server.c_str(), NTP_SERVER_2);
    // configTime may reset TZ on some Arduino-ESP32 versions; reapply the configured rule.
    setenv("TZ", posixTimezone.c_str(), 1);
    tzset();
    struct tm t;
    if (getLocalTime(&t, 5000))
    {
        bool was = timeValid;
        timeValid = true;
        ;
        ;
    }
    else ;
}
bool TimeManager::isTimeValid(){return timeValid;}
String TimeManager::getTimeString(){if(!timeValid)return "Time not synchronized";struct tm t;if(!getLocalTime(&t))return "Time error";char b[16];strftime(b,sizeof(b),"%H:%M:%S",&t);return String(b);}String TimeManager::getDateString(){if(!timeValid)return "Date not synchronized";struct tm t;if(!getLocalTime(&t))return "Date error";char b[32];strftime(b,sizeof(b),"%d.%m.%Y",&t);return String(b);}String TimeManager::getDateTimeString(){return timeValid?getDateString()+" "+getTimeString():"Time not synchronized";}time_t TimeManager::getTimestamp(){return time(nullptr);}int TimeManager::getHour(){struct tm t;if(!getLocalTime(&t))return -1;return t.tm_hour;}int TimeManager::getMinute(){struct tm t;if(!getLocalTime(&t))return -1;return t.tm_min;}int TimeManager::getSecond(){struct tm t;if(!getLocalTime(&t))return -1;return t.tm_sec;}
