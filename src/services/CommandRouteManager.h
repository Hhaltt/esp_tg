#pragma once
#include <Arduino.h>

class CommandRouteManager
{
public:
    // Returns true when the phrase matched a configured route.
    // reply contains the target response or a transport error.
    bool execute(const String& phrase, String& reply);
private:
    String normalize(const String& value) const;
};

extern CommandRouteManager commandRouteManager;
