#include "LedActivityManager.h"

#include "../core/Config.h"
#include "../core/Statistics.h"


LedActivityManager ledActivity;


// ============================================================
// BEGIN
// ============================================================

void LedActivityManager::begin()
{
    pinMode(
        LED_RX_PIN,
        OUTPUT
    );

    pinMode(
        LED_TX_PIN,
        OUTPUT
    );


    digitalWrite(
        LED_RX_PIN,
        LOW
    );

    digitalWrite(
        LED_TX_PIN,
        LOW
    );


    lastMessagesReceived =
        statistics.getMessagesReceived();

    lastMessagesSent =
        statistics.getMessagesSent();
}


// ============================================================
// UPDATE
// ============================================================

void LedActivityManager::update()
{
    uint64_t messagesReceived =
        statistics.getMessagesReceived();


    if (
        messagesReceived !=
        lastMessagesReceived
    )
    {
        lastMessagesReceived =
            messagesReceived;

        onRx();
    }


    uint64_t messagesSent =
        statistics.getMessagesSent();


    if (
        messagesSent !=
        lastMessagesSent
    )
    {
        lastMessagesSent =
            messagesSent;

        onTx();
    }


    if (
        rxActive
        &&
        millis() - rxStartedAt
        >= LED_ACTIVITY_DURATION
    )
    {
        digitalWrite(
            LED_RX_PIN,
            LOW
        );

        rxActive = false;
    }


    if (
        txActive
        &&
        millis() - txStartedAt
        >= LED_ACTIVITY_DURATION
    )
    {
        digitalWrite(
            LED_TX_PIN,
            LOW
        );

        txActive = false;
    }
}


// ============================================================
// RX
// ============================================================

void LedActivityManager::onRx()
{
    digitalWrite(
        LED_RX_PIN,
        HIGH
    );

    rxStartedAt =
        millis();

    rxActive = true;
}


// ============================================================
// TX
// ============================================================

void LedActivityManager::onTx()
{
    digitalWrite(
        LED_TX_PIN,
        HIGH
    );

    txStartedAt =
        millis();

    txActive = true;
}
