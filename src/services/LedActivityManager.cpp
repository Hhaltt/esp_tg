#include "LedActivityManager.h"

#include "../core/Config.h"
#include "../core/Statistics.h"

LedActivityManager ledActivity;

void LedActivityManager::begin()
{
    pinMode(LED_RX_PIN, OUTPUT);
    pinMode(LED_TX_PIN, OUTPUT);

    // LEDs are active LOW.
    digitalWrite(LED_RX_PIN, HIGH);
    digitalWrite(LED_TX_PIN, HIGH);

    esp_timer_create_args_t rxTimerArgs = {};
    rxTimerArgs.callback = rxTimerCallback;
    rxTimerArgs.arg = this;
    rxTimerArgs.name = "led_rx";
    esp_timer_create(&rxTimerArgs, &rxTimer);

    esp_timer_create_args_t txTimerArgs = {};
    txTimerArgs.callback = txTimerCallback;
    txTimerArgs.arg = this;
    txTimerArgs.name = "led_tx";
    esp_timer_create(&txTimerArgs, &txTimer);

    lastMessagesReceived = statistics.getMessagesReceived();
    lastMessagesSent = statistics.getMessagesSent();
}

void LedActivityManager::update()
{
    uint64_t messagesReceived = statistics.getMessagesReceived();
    if (messagesReceived != lastMessagesReceived)
    {
        lastMessagesReceived = messagesReceived;
        onRx();
    }

    uint64_t messagesSent = statistics.getMessagesSent();
    if (messagesSent != lastMessagesSent)
    {
        lastMessagesSent = messagesSent;
        onTx();
    }
}

void LedActivityManager::onRx()
{
    digitalWrite(LED_RX_PIN, LOW);
    if (rxTimer)
    {
        esp_timer_stop(rxTimer);
        esp_timer_start_once(rxTimer, (uint64_t)LED_ACTIVITY_DURATION * 1000ULL);
    }
}

void LedActivityManager::onTx()
{
    digitalWrite(LED_TX_PIN, LOW);
    if (txTimer)
    {
        esp_timer_stop(txTimer);
        esp_timer_start_once(txTimer, (uint64_t)LED_ACTIVITY_DURATION * 1000ULL);
    }
}

void LedActivityManager::rxTimerCallback(void* arg)
{
    digitalWrite(LED_RX_PIN, HIGH);
}

void LedActivityManager::txTimerCallback(void* arg)
{
    digitalWrite(LED_TX_PIN, HIGH);
}
