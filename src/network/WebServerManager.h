#pragma once

#include <Arduino.h>
#include <WebServer.h>


class WebServerManager
{
public:

    void begin();

    void update();


private:

    WebServer server;


    // --------------------------------------------------------
    // PAGES
    // --------------------------------------------------------

    void handleRoot();

    void handleSettings();

    void handleSaveSettings();

    void handleNotFound();

    void handleReminders();

    void handleAddReminder();

    void handleDeleteReminder();

    void handleToggleReminder();

    // --------------------------------------------------------
    // HELPERS
    // --------------------------------------------------------

    String getPageHeader(
        const String& title
    );

    String getPageFooter();

    String getNavigation();
};


extern WebServerManager webServerManager;