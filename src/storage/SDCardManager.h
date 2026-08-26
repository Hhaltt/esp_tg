#pragma once

#include <Arduino.h>

class SDCardManager
{
public:
    void begin();
    bool isAvailable();

private:
    bool available = false;
};

extern SDCardManager sdCard;