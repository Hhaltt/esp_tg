#pragma once

#include <Arduino.h>


class EthernetManager
{
public:

    void begin();

    void update();


    // --------------------------------------------------------
    // Status
    // --------------------------------------------------------

    bool isConnected();

    bool hasIP();

    bool linkUp();


    // --------------------------------------------------------
    // Information
    // --------------------------------------------------------

    String getIP();

    String getGateway();

    String getMAC();

    int getLinkSpeed();

    bool isFullDuplex();


private:

    bool connected = false;

    unsigned long lastStatusCheck = 0;
};


extern EthernetManager ethernet;