#pragma once
#include <Arduino.h>
#include <WebServer.h>
class WebServerManager
{
public:
    void begin(); void update(); WebServer& rawServer(){return server;}
private:
    WebServer server;
    void handleSaveSettings(); void handleAddChat(); void handleUpdateChat(); void handleDeleteChat();
    void handleAddCommandRoute(); void handleEditCommandRoute(); void handleDeleteCommandRoute();
    void handleAddReminder(); void handleUpdateReminder(); void handleCloneReminder(); void handleDeleteReminder(); void handleToggleReminder();
    void handleHubTelegramSend(); void handleNotFound();
};
extern WebServerManager webServerManager;
