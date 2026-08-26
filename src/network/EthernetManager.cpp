#include "EthernetManager.h"

#include <ETH.h>
#include <WiFi.h>

#include "../core/Config.h"


EthernetManager ethernet;


// ============================================================
// WIFI / ETHERNET EVENTS
// ============================================================

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
        case SYSTEM_EVENT_ETH_START:

            Serial.println("[ETH] Started");

            ETH.setHostname(
                DEVICE_NAME
            );

            break;


        case SYSTEM_EVENT_ETH_CONNECTED:

            Serial.println(
                "[ETH] Link UP"
            );

            break;


        case SYSTEM_EVENT_ETH_GOT_IP:

            Serial.println(
                "[ETH] GOT_IP event"
            );

            break;


        case SYSTEM_EVENT_ETH_DISCONNECTED:

            Serial.println(
                "[ETH] Link DOWN"
            );

            break;


        default:
            break;
    }
}


// ============================================================
// BEGIN
// ============================================================

void EthernetManager::begin()
{
    Serial.println(
        "[ETH] Initializing..."
    );


    WiFi.onEvent(
        WiFiEvent
    );


    bool ok = ETH.begin(
        ETH_PHY_ADDR,
        ETH_PHY_POWER,
        ETH_PHY_MDC,
        ETH_PHY_MDIO,
        ETH_PHY_TYPE,
        ETH_CLK_MODE
    );


    Serial.print(
        "[ETH] ETH.begin() = "
    );

    Serial.println(
        ok ? "OK" : "FAILED"
    );
}


// ============================================================
// UPDATE
// ============================================================

void EthernetManager::update()
{
    bool currentConnected =
        ETH.linkUp()
        &&
        (uint32_t)ETH.localIP() != 0;


    // --------------------------------------------------------
    // Connection changed
    // --------------------------------------------------------

    if (
        currentConnected
        &&
        !connected
    )
    {
        connected = true;

        Serial.println(
            "\n========== ETHERNET CONNECTED =========="
        );

        Serial.print("IP: ");
        Serial.println(
            ETH.localIP()
        );

        Serial.print("Gateway: ");
        Serial.println(
            ETH.gatewayIP()
        );

        Serial.print("DNS: ");
        Serial.println(
            ETH.dnsIP()
        );

        Serial.print("Speed: ");
        Serial.print(
            ETH.linkSpeed()
        );

        Serial.println(" Mbps");

        Serial.println(
            "========================================\n"
        );
    }


    if (
        !currentConnected
        &&
        connected
    )
    {
        connected = false;

        Serial.println(
            "[ETH] Connection lost"
        );
    }
}


// ============================================================
// STATUS
// ============================================================

bool EthernetManager::isConnected()
{
    return connected;
}


bool EthernetManager::hasIP()
{
    return
        (uint32_t)ETH.localIP()
        != 0;
}


bool EthernetManager::linkUp()
{
    return ETH.linkUp();
}


// ============================================================
// INFORMATION
// ============================================================

String EthernetManager::getIP()
{
    return ETH.localIP().toString();
}


String EthernetManager::getGateway()
{
    return ETH.gatewayIP().toString();
}


String EthernetManager::getMAC()
{
    return ETH.macAddress();
}


int EthernetManager::getLinkSpeed()
{
    return ETH.linkSpeed();
}


bool EthernetManager::isFullDuplex()
{
    return ETH.fullDuplex();
}