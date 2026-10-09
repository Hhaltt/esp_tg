#include "CommandRouteManager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../core/ConfigManager.h"

CommandRouteManager commandRouteManager;

static String formEncode(const String& value)
{
    const char hex[] = "0123456789ABCDEF";
    String encoded;
    for (size_t i = 0; i < value.length(); ++i)
    {
        uint8_t ch = static_cast<uint8_t>(value[i]);
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '*')
            encoded += static_cast<char>(ch);
        else if (ch == ' ')
            encoded += '+';
        else
        {
            encoded += '%';
            encoded += hex[(ch >> 4) & 0x0F];
            encoded += hex[ch & 0x0F];
        }
    }
    return encoded;
}

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
        String body="command="+formEncode(route.command);
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
