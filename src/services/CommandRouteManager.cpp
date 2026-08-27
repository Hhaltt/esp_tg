#include "CommandRouteManager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../core/ConfigManager.h"

CommandRouteManager commandRouteManager;

String CommandRouteManager::normalize(const String& value) const
{
    String out=value;
    out.trim();
    out.toLowerCase();
    while(out.indexOf("  ")>=0)out.replace("  "," ");
    return out;
}

bool CommandRouteManager::execute(const String& phrase, String& reply)
{
    String normalized=normalize(phrase);
    for(size_t i=0;i<config.getCommandRouteCount();i++)
    {
        CommandRouteConfig route=config.getCommandRoute(i);
        if(normalize(route.phrase)!=normalized)continue;

        HTTPClient http;
        if(!http.begin(route.url))
        {
            reply="❌ Не вдалося підключитися до пристрою";
            return true;
        }
        http.setConnectTimeout(3000);
        http.setTimeout(5000);
        http.addHeader("Content-Type","application/x-www-form-urlencoded");
        if(route.apiKey.length())http.addHeader("X-API-Key",route.apiKey);
        String body="command="+route.command;
        int code=http.POST(body);
        String response=http.getString();
        http.end();

        if(code<200||code>=300)
        {
            reply="❌ Пристрій не виконав команду (HTTP "+String(code)+")";
            return true;
        }

        JsonDocument doc;
        DeserializationError error=deserializeJson(doc,response);
        if(!error && doc["message"].is<const char*>())
        {
            reply=String(doc["message"].as<const char*>());
        }
        if(!reply.length())reply="✅ Виконано";
        return true;
    }
    return false;
}
