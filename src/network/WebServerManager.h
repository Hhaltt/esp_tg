#pragma once

#include <Arduino.h>
#include <WebServer.h>

class WebServerManager
{
public:
    void begin();
    void update();

    // Experimental static web frontend can register routes and files here.
    WebServer& rawServer()
    {
        return server;
    }

private:
    WebServer server;

    void handleRoot();
    void handleSettings();
    void handleSaveSettings();

    void handleAddChat();
    void handleEditChat();
    void handleUpdateChat();
    void handleDeleteChat();

    void handleReminders();
    void handleAddReminder();
    void handleEditReminder();
    void handleUpdateReminder();
    void handleCloneReminder();
    void handleDeleteReminder();
    void handleToggleReminder();

    void handleHubTelegramSend();
    void handleNotFound();

    String getPageHeader(const String& title);
    String getPageFooter();
    String getNavigation();
};

extern WebServerManager webServerManager;
