#include "WebServerManager.h"

#include <ETH.h>
#include <SD.h>

#include "../core/Config.h"
#include "../core/ConfigManager.h"

#include "../storage/SDCardManager.h"
#include "../services/ReminderManager.h"
#include "../services/TimeManager.h"
#include "../core/Statistics.h"


WebServerManager webServerManager;


// ============================================================
// BEGIN
// ============================================================

void WebServerManager::begin()
{
    Serial.println(
        "[WEB] Initializing..."
    );


    // --------------------------------------------------------
    // ROUTES
    // --------------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        [this]()
        {
            handleRoot();
        }
    );


    server.on(
        "/settings",
        HTTP_GET,
        [this]()
        {
            handleSettings();
        }
    );


    server.on(
        "/settings/save",
        HTTP_POST,
        [this]()
        {
            handleSaveSettings();
        }
    );

    server.on(
    "/reminders",
    HTTP_GET,
    [this]()
    {
        handleReminders();
    }
    );

    server.on(
    "/reminders/add",
    HTTP_POST,
    [this]()
    {
        handleAddReminder();
    }
    );

    server.on(
    "/reminders/delete",
    HTTP_POST,
    [this]()
    {
        handleDeleteReminder();
    }
    );

    server.onNotFound(
        [this]()
        {
            handleNotFound();
        }
    );

    server.on(
    "/reminders/toggle",
    HTTP_POST,
    [this]()
    {
        handleToggleReminder();
    }
    );

    // --------------------------------------------------------
    // START
    // --------------------------------------------------------

    server.begin();


    Serial.println(
        "[WEB] Started"
    );


    Serial.print(
        "[WEB] Open: http://"
    );

    Serial.println(
        ETH.localIP()
    );
}


// ============================================================
// UPDATE
// ============================================================

void WebServerManager::update()
{
    server.handleClient();
}


// ============================================================
// ROOT / DASHBOARD
// ============================================================

void WebServerManager::handleRoot()
{
    String html;


    html += getPageHeader(
        config.getDeviceName()
    );


    html += getNavigation();


    // --------------------------------------------------------
    // TITLE
    // --------------------------------------------------------

    html += "<h1>";
    html += config.getDeviceName();
    html += "</h1>";


    // ========================================================
    // NETWORK
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>Network</h2>";


    html += "<p><b>Status:</b> ";

    if (ETH.linkUp())
    {
        html += "Online";
    }
    else
    {
        html += "Offline";
    }

    html += "</p>";


    html += "<p><b>IP:</b> ";
    html += ETH.localIP().toString();
    html += "</p>";


    html += "<p><b>Gateway:</b> ";
    html += ETH.gatewayIP().toString();
    html += "</p>";


    html += "<p><b>Speed:</b> ";
    html += String(
        ETH.linkSpeed()
    );
    html += " Mbps";
    html += "</p>";


    html += "<p><b>Duplex:</b> ";

    if (ETH.fullDuplex())
    {
        html += "Full";
    }
    else
    {
        html += "Half";
    }

    html += "</p>";


    html += "</div>";


    // ========================================================
    // SYSTEM
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>System</h2>";


    html += "<p><b>Total uptime:</b> ";
    html += statistics.getTotalUptimeString();
    html += "</p>";


    html += "<p><b>Current session:</b> ";
    html += statistics.getCurrentUptimeString();
    html += "</p>";


    html += "<p><b>Boot count:</b> ";
    html += String(
        statistics.getBootCount()
    );
    html += "</p>";


    html += "<p><b>Time:</b> ";
    html += timeManager.getDateTimeString();
    html += "</p>";


    html += "<p><b>Total errors:</b> ";
    html += String(
        statistics.getErrors()
    );
    html += "</p>";


    html += "</div>";


    // ========================================================
    // TELEGRAM
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>Telegram</h2>";


    html += "<p><b>Messages received:</b> ";
    html += String(
        statistics.getMessagesReceived()
    );
    html += "</p>";


    html += "<p><b>Messages sent:</b> ";
    html += String(
        statistics.getMessagesSent()
    );
    html += "</p>";


    html += "</div>";


    // ========================================================
    // COMMANDS
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>Commands</h2>";


    html += "<p><b>Received:</b> ";
    html += String(
        statistics.getCommandsReceived()
    );
    html += "</p>";


    html += "<p><b>Executed:</b> ";
    html += String(
        statistics.getCommandsExecuted()
    );
    html += "</p>";


    html += "<p><b>Errors:</b> ";
    html += String(
        statistics.getCommandErrors()
    );
    html += "</p>";


    html += "</div>";


    // ========================================================
    // STORAGE
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>Storage</h2>";


    if (sdCard.isAvailable())
    {
        uint64_t total =
            SD.totalBytes();

        uint64_t used =
            SD.usedBytes();


        double totalMB =
            (double)total /
            1024.0 /
            1024.0;


        double usedMB =
            (double)used /
            1024.0 /
            1024.0;


        double freeMB =
            (double)(total - used) /
            1024.0 /
            1024.0;


        html += "<p><b>SD total:</b> ";
        html += String(
            totalMB,
            2
        );
        html += " MB</p>";


        html += "<p><b>Used:</b> ";
        html += String(
            usedMB,
            2
        );
        html += " MB</p>";


        html += "<p><b>Free:</b> ";
        html += String(
            freeMB,
            2
        );
        html += " MB</p>";
    }
    else
    {
        html +=
            "<p class='error'>"
            "SD card unavailable"
            "</p>";
    }


    html += "</div>";


    // --------------------------------------------------------
    // FOOTER
    // --------------------------------------------------------

    html += getPageFooter();


    server.send(
        200,
        "text/html; charset=utf-8",
        html
    );
}


// ============================================================
// SETTINGS PAGE
// ============================================================

void WebServerManager::handleSettings()
{
    String html;


    html += getPageHeader(
        "Settings"
    );


    html += getNavigation();


    html += "<h1>Settings</h1>";


    html +=
        "<form "
        "method='POST' "
        "action='/settings/save'>";


    // ========================================================
    // SYSTEM
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>System</h2>";


    html +=
        "<label>"
        "Device name"
        "</label>";


    html +=
        "<input "
        "type='text' "
        "name='deviceName' "
        "value='";

    html += config.getDeviceName();

    html += "'>";


    html +=
        "<label>"
        "Timezone"
        "</label>";


    html +=
        "<input "
        "type='text' "
        "name='timezone' "
        "value='";

    html += config.getTimezone();

    html += "'>";


    html += "</div>";
        // ========================================================
    // STARTUP MESSAGE
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>Startup message</h2>";


    html +=
        "<label class='checkbox'>";


    html +=
        "<input "
        "type='checkbox' "
        "name='startupMessageEnabled' ";


    if (config.isStartupMessageEnabled())
    {
        html += "checked";
    }


    html +=
        "> "
        "Send message after startup"
        "</label>";


    html +=
        "<label>"
        "Message"
        "</label>";


    html +=
        "<textarea "
        "name='startupMessage' "
        "rows='4'>";


    html += config.getStartupMessage();


    html +=
        "</textarea>";


    html += "</div>";

    // ========================================================
    // WEATHER
    // ========================================================

    html += "<div class='card'>";

    html += "<h2>Weather</h2>";


    html +=
        "<label class='checkbox'>";


    html +=
        "<input "
        "type='checkbox' "
        "name='weatherEnabled' ";


    if (config.isWeatherEnabled())
    {
        html += "checked";
    }


    html +=
        "> "
        "Enable weather"
        "</label>";


    html +=
        "<label>"
        "OpenWeather API key"
        "</label>";


    html +=
        "<input "
        "type='text' "
        "name='weatherApiKey' "
        "value='";

    html += config.getWeatherApiKey();

    html += "'>";


    html +=
        "<label>"
        "City"
        "</label>";


    html +=
        "<input "
        "type='text' "
        "name='weatherCity' "
        "value='";

    html += config.getWeatherCity();

    html += "'>";


    html +=
        "<label>"
        "Send hour"
        "</label>";


    html +=
        "<input "
        "type='number' "
        "name='weatherHour' "
        "min='0' "
        "max='23' "
        "value='";

    html += String(
        config.getWeatherHour()
    );

    html += "'>";


    html +=
        "<label>"
        "Send minute"
        "</label>";


    html +=
        "<input "
        "type='number' "
        "name='weatherMinute' "
        "min='0' "
        "max='59' "
        "value='";

    html += String(
        config.getWeatherMinute()
    );

    html += "'>";


    html += "</div>";
    


    // ========================================================
    // SAVE BUTTON
    // ========================================================

    html +=
        "<button type='submit'>"
        "Save settings"
        "</button>";


    html +=
        "</form>";


    html += getPageFooter();


    server.send(
        200,
        "text/html; charset=utf-8",
        html
    );
}


// ============================================================
// SAVE SETTINGS
// ============================================================

void WebServerManager::handleSaveSettings()
{
    // ========================================================
    // SYSTEM
    // ========================================================

    if (server.hasArg("deviceName"))
    {
        config.setDeviceName(
            server.arg(
                "deviceName"
            )
        );
    }


    if (server.hasArg("timezone"))
    {
        config.setTimezone(
            server.arg(
                "timezone"
            )
        );
    }
        // ========================================================
    // STARTUP MESSAGE
    // ========================================================

    config.setStartupMessageEnabled(
        server.hasArg(
            "startupMessageEnabled"
        )
    );


    if (
        server.hasArg(
            "startupMessage"
        )
    )
    {
        config.setStartupMessage(
            server.arg(
                "startupMessage"
            )
        );
    }

    // ========================================================
    // WEATHER
    // ========================================================

    config.setWeatherEnabled(
        server.hasArg(
            "weatherEnabled"
        )
    );


    if (server.hasArg("weatherApiKey"))
    {
        config.setWeatherApiKey(
            server.arg(
                "weatherApiKey"
            )
        );
    }


    if (server.hasArg("weatherCity"))
    {
        config.setWeatherCity(
            server.arg(
                "weatherCity"
            )
        );
    }


    uint8_t hour =
        server.arg(
            "weatherHour"
        ).toInt();


    uint8_t minute =
        server.arg(
            "weatherMinute"
        ).toInt();


    config.setWeatherTime(
        hour,
        minute
    );


    // ========================================================
    // SAVE CONFIG
    // ========================================================

    bool saved =
        config.save();


    // ========================================================
    // RESPONSE
    // ========================================================

    String html;


    html += getPageHeader(
        saved
            ? "Settings saved"
            : "Save error"
    );


    html += getNavigation();


    html += "<div class='card'>";


    if (saved)
    {
        html +=
            "<h2>Settings saved</h2>";


        html +=
            "<p>"
            "Configuration has been saved "
            "to the SD card."
            "</p>";
    }
    else
    {
        html +=
            "<h2 class='error'>"
            "Failed to save settings"
            "</h2>";
    }


    html +=
        "<p>"
        "<a href='/settings'>"
        "Back to settings"
        "</a>"
        "</p>";


    html += "</div>";


    html += getPageFooter();


    server.send(
        saved
            ? 200
            : 500,

        "text/html; charset=utf-8",

        html
    );
}


// ============================================================
// 404
// ============================================================

void WebServerManager::handleNotFound()
{
    server.send(
        404,
        "text/plain; charset=utf-8",
        "404 - Not found"
    );
}


// ============================================================
// PAGE HEADER
// ============================================================

String WebServerManager::getPageHeader(
    const String& title
)
{
    String html;


    html += "<!DOCTYPE html>";

    html +=
        "<html "
        "lang='en'>";


    html += "<head>";


    html +=
        "<meta "
        "charset='UTF-8'>";


    html +=
        "<meta "
        "name='viewport' "
        "content='width=device-width, initial-scale=1'>";


    html += "<title>";

    html += title;

    html += "</title>";


    // --------------------------------------------------------
    // CSS
    // --------------------------------------------------------

    html += "<style>";


    html +=
        "*{"
        "box-sizing:border-box;"
        "}";


    html +=
        "body{"
        "margin:0;"
        "font-family:Arial,sans-serif;"
        "background:#f4f6f8;"
        "color:#222;"
        "}";


    html +=
        ".container{"
        "max-width:900px;"
        "margin:auto;"
        "padding:20px;"
        "}";


    html +=
        "nav{"
        "background:#222;"
        "padding:12px;"
        "border-radius:8px;"
        "}";


    html +=
        "nav a{"
        "color:white;"
        "text-decoration:none;"
        "margin-right:20px;"
        "}";


    html +=
        "nav a:last-child{"
        "margin-right:0;"
        "}";


    html +=
        ".card{"
        "background:white;"
        "padding:20px;"
        "margin:20px 0;"
        "border-radius:8px;"
        "box-shadow:0 2px 5px rgba(0,0,0,.1);"
        "}";


    html +=
        "h1,h2{"
        "margin-top:0;"
        "}";


    html +=
        "label{"
        "display:block;"
        "margin-top:15px;"
        "font-weight:bold;"
        "}";


    html +=
        "input{"
        "width:100%;"
        "padding:10px;"
        "margin-top:5px;"
        "border:1px solid #ccc;"
        "border-radius:5px;"
        "font-size:16px;"
        "}";


    html +=
        ".checkbox{"
        "display:flex;"
        "align-items:center;"
        "gap:8px;"
        "}";


    html +=
        ".checkbox input{"
        "width:auto;"
        "margin:0;"
        "}";


    html +=
        "button{"
        "padding:12px 20px;"
        "font-size:16px;"
        "border:0;"
        "border-radius:5px;"
        "background:#333;"
        "color:white;"
        "cursor:pointer;"
        "}";


    html +=
        "button:hover{"
        "opacity:.85;"
        "}";


    html +=
        ".error{"
        "color:#c00;"
        "}";


    html +=
        "footer{"
        "text-align:center;"
        "color:#777;"
        "padding:20px 0;"
        "font-size:14px;"
        "}";


    html += "</style>";


    html += "</head>";


    html += "<body>";


    html +=
        "<div class='container'>";


    return html;
}


// ============================================================
// NAVIGATION
// ============================================================

String WebServerManager::getNavigation()
{
    String html;


    html += "<nav>";


    html +=
        "<a href='/'>"
        "Home"
        "</a>";


    html +=
        "<a href='/settings'>"
        "Settings"
        "</a>";


    html +=
        "<a href='/reminders'>"
        "Reminders"
        "</a>";


    html += "</nav>";


    return html;
}


// ============================================================
// FOOTER
// ============================================================

String WebServerManager::getPageFooter()
{
    String html;


    html += "<footer>";


    html +=
        config.getDeviceName();


    html +=
        " Assistant";


    html += "</footer>";


    html += "</div>";


    html += "</body>";


    html += "</html>";


    return html;
}
// ============================================================
// REMINDERS PAGE
// ============================================================

// ============================================================
// REMINDERS PAGE
// ============================================================

void WebServerManager::handleReminders()
{
    String html;

    html.reserve(16000);


    html += getPageHeader(
        "Reminders"
    );

    html += getNavigation();


    html +=
        "<style>"

        ".reminder-card{"
        "margin-bottom:15px;"
        "}"

        ".reminder-disabled{"
        "opacity:0.55;"
        "}"

        ".reminder-message{"
        "font-size:18px;"
        "font-weight:bold;"
        "margin-bottom:12px;"
        "}"

        ".reminder-info{"
        "line-height:1.8;"
        "}"

        ".reminder-status{"
        "display:inline-block;"
        "padding:3px 8px;"
        "border-radius:5px;"
        "font-size:13px;"
        "font-weight:bold;"
        "}"

        ".status-on{"
        "background:#d4edda;"
        "color:#155724;"
        "}"

        ".status-off{"
        "background:#f8d7da;"
        "color:#721c24;"
        "}"

        ".button-row{"
        "display:flex;"
        "gap:8px;"
        "margin-top:15px;"
        "flex-wrap:wrap;"
        "}"

        ".btn-danger{"
        "background:#c0392b;"
        "}"

        ".btn-secondary{"
        "background:#555;"
        "}"

        ".form-section{"
        "margin-bottom:15px;"
        "}"

        "</style>";


    html +=
        "<h1>Reminders</h1>";


    // ========================================================
    // EXISTING REMINDERS
    // ========================================================

    html +=
        "<div class='card'>";


    html +=
        "<h2>My reminders (";

    html += String(
        reminderManager.getCount()
    );

    html +=
        ")</h2>";


    if (
        reminderManager.getCount() == 0
    )
    {
        html +=
            "<p>No reminders yet.</p>";
    }
    else
    {
        for (
            size_t i = 0;
            i < reminderManager.getCount();
            i++
        )
        {
            Reminder reminder =
                reminderManager.getReminder(
                    i
                );


            html +=
                "<div class='card reminder-card";


            if (
                !reminder.enabled
            )
            {
                html +=
                    " reminder-disabled";
            }


            html +=
                "'>";


            // ------------------------------------------------
            // MESSAGE
            // ------------------------------------------------

            html +=
                "<div class='reminder-message'>";

            html +=
                reminder.message;

            html +=
                "</div>";


            // ------------------------------------------------
            // STATUS
            // ------------------------------------------------

            html +=
                "<div class='reminder-info'>";


            html +=
                "<b>Status:</b> ";


            if (
                reminder.enabled
            )
            {
                html +=
                    "<span class='reminder-status status-on'>"
                    "Enabled"
                    "</span>";
            }
            else
            {
                html +=
                    "<span class='reminder-status status-off'>"
                    "Disabled"
                    "</span>";
            }


            html +=
                "<br>";


            // ------------------------------------------------
            // SCHEDULE
            // ------------------------------------------------

            html +=
                "<b>Schedule:</b> ";


            switch (
                reminder.type
            )
            {
                // --------------------------------------------
                // ONCE
                // --------------------------------------------

                case ReminderType::ONCE:

                    html +=
                        "Once: ";

                    if (
                        reminder.day < 10
                    )
                    {
                        html += "0";
                    }

                    html += String(
                        reminder.day
                    );

                    html += ".";


                    if (
                        reminder.month < 10
                    )
                    {
                        html += "0";
                    }

                    html += String(
                        reminder.month
                    );

                    html += ".";

                    html += String(
                        reminder.year
                    );

                    break;


                // --------------------------------------------
                // DAILY
                // --------------------------------------------

                case ReminderType::DAILY:

                    html +=
                        "Every day";

                    break;


                // --------------------------------------------
                // WEEKLY
                // --------------------------------------------

                case ReminderType::WEEKLY:

                    html +=
                        "Every ";

                    switch (
                        reminder.weekday
                    )
                    {
                        case 0:
                            html += "Sunday";
                            break;

                        case 1:
                            html += "Monday";
                            break;

                        case 2:
                            html += "Tuesday";
                            break;

                        case 3:
                            html += "Wednesday";
                            break;

                        case 4:
                            html += "Thursday";
                            break;

                        case 5:
                            html += "Friday";
                            break;

                        case 6:
                            html += "Saturday";
                            break;

                        default:
                            html += "Unknown";
                            break;
                    }

                    break;


                // --------------------------------------------
                // MONTHLY
                // --------------------------------------------

                case ReminderType::MONTHLY:

                    html +=
                        "Every month on day ";

                    html += String(
                        reminder.day
                    );

                    break;


                // --------------------------------------------
                // YEARLY
                // --------------------------------------------

                case ReminderType::YEARLY:

                    html +=
                        "Every year on ";


                    if (
                        reminder.day < 10
                    )
                    {
                        html += "0";
                    }

                    html += String(
                        reminder.day
                    );

                    html += ".";


                    if (
                        reminder.month < 10
                    )
                    {
                        html += "0";
                    }

                    html += String(
                        reminder.month
                    );

                    break;
            }


            html +=
                "<br>";


            // ------------------------------------------------
            // TIME
            // ------------------------------------------------

            html +=
                "<b>Time:</b> ";


            if (
                reminder.hour < 10
            )
            {
                html += "0";
            }

            html += String(
                reminder.hour
            );

            html += ":";


            if (
                reminder.minute < 10
            )
            {
                html += "0";
            }

            html += String(
                reminder.minute
            );


            // ------------------------------------------------
            // NEXT TRIGGER
            // ------------------------------------------------

            if (
                reminder.nextTrigger > 0
            )
            {
                struct tm nextTime;

                localtime_r(
                    &reminder.nextTrigger,
                    &nextTime
                );


                char buffer[32];


                strftime(
                    buffer,
                    sizeof(buffer),
                    "%d.%m.%Y %H:%M",
                    &nextTime
                );


                html +=
                    "<br><b>Next:</b> ";

                html +=
                    buffer;
            }


            // ------------------------------------------------
            // LAST TRIGGER
            // ------------------------------------------------

            if (
                reminder.lastTriggered > 0
            )
            {
                struct tm lastTime;

                localtime_r(
                    &reminder.lastTriggered,
                    &lastTime
                );


                char buffer[32];


                strftime(
                    buffer,
                    sizeof(buffer),
                    "%d.%m.%Y %H:%M",
                    &lastTime
                );


                html +=
                    "<br><b>Last:</b> ";

                html +=
                    buffer;
            }


            html +=
                "</div>";


            // ------------------------------------------------
            // BUTTONS
            // ------------------------------------------------

            html +=
                "<div class='button-row'>";


            // Toggle

            html +=
                "<form method='POST' "
                "action='/reminders/toggle'>";


            html +=
                "<input type='hidden' "
                "name='id' "
                "value='";

            html += String(
                reminder.id
            );

            html += "'>";


            html +=
                "<input type='hidden' "
                "name='enabled' "
                "value='";

            html +=
                reminder.enabled
                ? "0"
                : "1";

            html += "'>";


            html +=
                "<button "
                "type='submit' "
                "class='btn-secondary'>";


            html +=
                reminder.enabled
                ? "Disable"
                : "Enable";


            html +=
                "</button>";


            html +=
                "</form>";


            // Delete

            html +=
                "<form method='POST' "
                "action='/reminders/delete'>";


            html +=
                "<input type='hidden' "
                "name='id' "
                "value='";

            html += String(
                reminder.id
            );

            html += "'>";


            html +=
                "<button "
                "type='submit' "
                "class='btn-danger' "
                "onclick='return confirm("
                "\"Delete this reminder?\""
                ")'>"
                "Delete"
                "</button>";


            html +=
                "</form>";


            html +=
                "</div>";


            html +=
                "</div>";
        }
    }


    html +=
        "</div>";


    // ========================================================
    // ADD REMINDER
    // ========================================================

    html +=
        "<div class='card'>";


    html +=
        "<h2>Add reminder</h2>";


    html +=
        "<form "
        "method='POST' "
        "action='/reminders/add'>";


    // --------------------------------------------------------
    // MESSAGE
    // --------------------------------------------------------

    html +=
        "<div class='form-section'>";

    html +=
        "<label>Message</label>";

    html +=
        "<input "
        "type='text' "
        "name='message' "
        "required>";

    html +=
        "</div>";


    // --------------------------------------------------------
    // TYPE
    // --------------------------------------------------------

    html +=
        "<div class='form-section'>";

    html +=
        "<label>Repeat</label>";

    html +=
        "<select "
        "name='type' "
        "id='reminderType' "
        "onchange='updateReminderFields()'>";

    html +=
        "<option value='once'>Once</option>";

    html +=
        "<option value='daily'>Every day</option>";

    html +=
        "<option value='weekly'>Every week</option>";

    html +=
        "<option value='monthly'>Every month</option>";

    html +=
        "<option value='yearly'>Every year</option>";

    html +=
        "</select>";

    html +=
        "</div>";


    // --------------------------------------------------------
    // ONCE DATE
    // --------------------------------------------------------

    html +=
        "<div "
        "class='form-section' "
        "id='onceFields'>";


    html +=
        "<label>Date</label>";

    html +=
        "<input "
        "type='date' "
        "name='onceDate' "
        "id='onceDate'>";


    html +=
        "</div>";


    // --------------------------------------------------------
    // WEEKDAY
    // --------------------------------------------------------

    html +=
        "<div "
        "class='form-section' "
        "id='weeklyFields'>";


    html +=
        "<label>Day of week</label>";

    html +=
        "<select "
        "name='weekday'>";


    html +=
        "<option value='0'>Sunday</option>";

    html +=
        "<option value='1'>Monday</option>";

    html +=
        "<option value='2'>Tuesday</option>";

    html +=
        "<option value='3'>Wednesday</option>";

    html +=
        "<option value='4'>Thursday</option>";

    html +=
        "<option value='5'>Friday</option>";

    html +=
        "<option value='6'>Saturday</option>";


    html +=
        "</select>";


    html +=
        "</div>";


    // --------------------------------------------------------
    // MONTHLY DAY
    // --------------------------------------------------------

    html +=
        "<div "
        "class='form-section' "
        "id='monthlyFields'>";


    html +=
        "<label>Day of month</label>";

    html +=
        "<input "
        "type='number' "
        "name='monthlyDay' "
        "min='1' "
        "max='31' "
        "value='1'>";


    html +=
        "</div>";


    // --------------------------------------------------------
    // YEARLY DATE
    // --------------------------------------------------------

    html +=
        "<div "
        "class='form-section' "
        "id='yearlyFields'>";


    html +=
        "<label>Day</label>";

    html +=
        "<input "
        "type='number' "
        "name='yearlyDay' "
        "min='1' "
        "max='31' "
        "value='1'>";


    html +=
        "<label>Month</label>";

    html +=
        "<select "
        "name='yearlyMonth'>";

    html +=
        "<option value='1'>January</option>";

    html +=
        "<option value='2'>February</option>";

    html +=
        "<option value='3'>March</option>";

    html +=
        "<option value='4'>April</option>";

    html +=
        "<option value='5'>May</option>";

    html +=
        "<option value='6'>June</option>";

    html +=
        "<option value='7'>July</option>";

    html +=
        "<option value='8'>August</option>";

    html +=
        "<option value='9'>September</option>";

    html +=
        "<option value='10'>October</option>";

    html +=
        "<option value='11'>November</option>";

    html +=
        "<option value='12'>December</option>";


    html +=
        "</select>";


    html +=
        "</div>";


    // --------------------------------------------------------
    // TIME
    // --------------------------------------------------------

    html +=
        "<div class='form-section'>";

    html +=
        "<label>Time</label>";

    html +=
        "<input "
        "type='time' "
        "name='time' "
        "value='09:00' "
        "required>";

    html +=
        "</div>";


    // --------------------------------------------------------
    // SUBMIT
    // --------------------------------------------------------

    html +=
        "<button "
        "type='submit'>"
        "Add reminder"
        "</button>";


    html +=
        "</form>";


    html +=
        "</div>";


    // ========================================================
    // JAVASCRIPT
    // ========================================================

    html +=
        "<script>"

        "function updateReminderFields(){"

        "const type=document.getElementById('reminderType').value;"

        "document.getElementById('onceFields').style.display="
        "(type==='once')?'block':'none';"

        "document.getElementById('weeklyFields').style.display="
        "(type==='weekly')?'block':'none';"

        "document.getElementById('monthlyFields').style.display="
        "(type==='monthly')?'block':'none';"

        "document.getElementById('yearlyFields').style.display="
        "(type==='yearly')?'block':'none';"

        "}"

        "updateReminderFields();"

        "</script>";


    html +=
        getPageFooter();


    server.send(
        200,
        "text/html; charset=utf-8",
        html
    );
}
// ============================================================
// ADD REMINDER
// ============================================================

void WebServerManager::handleAddReminder()
{
    if (
        !server.hasArg("message") ||
        !server.hasArg("type") ||
        !server.hasArg("time")
    )
    {
        server.send(
            400,
            "text/plain",
            "Missing reminder data"
        );

        return;
    }


    Reminder reminder;


    // ========================================================
    // MESSAGE
    // ========================================================

    reminder.message =
        server.arg(
            "message"
        );

    reminder.message.trim();


    if (
        reminder.message.length() == 0
    )
    {
        server.send(
            400,
            "text/plain",
            "Message cannot be empty"
        );

        return;
    }


    // ========================================================
    // TYPE
    // ========================================================

    String type =
        server.arg(
            "type"
        );


    if (
        type == "daily"
    )
    {
        reminder.type =
            ReminderType::DAILY;
    }
    else if (
        type == "weekly"
    )
    {
        reminder.type =
            ReminderType::WEEKLY;
    }
    else if (
        type == "monthly"
    )
    {
        reminder.type =
            ReminderType::MONTHLY;
    }
    else if (
        type == "yearly"
    )
    {
        reminder.type =
            ReminderType::YEARLY;
    }
    else
    {
        reminder.type =
            ReminderType::ONCE;
    }


    // ========================================================
    // TIME
    // ========================================================

    String timeValue =
        server.arg(
            "time"
        );


    if (
        timeValue.length() != 5
    )
    {
        server.send(
            400,
            "text/plain",
            "Invalid time"
        );

        return;
    }


    reminder.hour =
        timeValue.substring(
            0,
            2
        ).toInt();


    reminder.minute =
        timeValue.substring(
            3,
            5
        ).toInt();


    if (
        reminder.hour < 0 ||
        reminder.hour > 23 ||
        reminder.minute < 0 ||
        reminder.minute > 59
    )
    {
        server.send(
            400,
            "text/plain",
            "Invalid time"
        );

        return;
    }


    // ========================================================
    // ONCE
    // ========================================================

    if (
        reminder.type ==
        ReminderType::ONCE
    )
    {
        if (
            !server.hasArg(
                "onceDate"
            )
        )
        {
            server.send(
                400,
                "text/plain",
                "Date required"
            );

            return;
        }


        String date =
            server.arg(
                "onceDate"
            );


        // YYYY-MM-DD

        if (
            date.length() != 10
        )
        {
            server.send(
                400,
                "text/plain",
                "Invalid date"
            );

            return;
        }


        reminder.year =
            date.substring(
                0,
                4
            ).toInt();


        reminder.month =
            date.substring(
                5,
                7
            ).toInt();


        reminder.day =
            date.substring(
                8,
                10
            ).toInt();
    }


    // ========================================================
    // WEEKLY
    // ========================================================

    if (
        reminder.type ==
        ReminderType::WEEKLY
    )
    {
        reminder.weekday =
            server.arg(
                "weekday"
            ).toInt();
    }


    // ========================================================
    // MONTHLY
    // ========================================================

    if (
        reminder.type ==
        ReminderType::MONTHLY
    )
    {
        reminder.day =
            server.arg(
                "monthlyDay"
            ).toInt();


        if (
            reminder.day < 1 ||
            reminder.day > 31
        )
        {
            server.send(
                400,
                "text/plain",
                "Invalid day"
            );

            return;
        }
    }


    // ========================================================
    // YEARLY
    // ========================================================

    if (
        reminder.type ==
        ReminderType::YEARLY
    )
    {
        reminder.day =
            server.arg(
                "yearlyDay"
            ).toInt();


        reminder.month =
            server.arg(
                "yearlyMonth"
            ).toInt();


        if (
            reminder.day < 1 ||
            reminder.day > 31 ||
            reminder.month < 1 ||
            reminder.month > 12
        )
        {
            server.send(
                400,
                "text/plain",
                "Invalid date"
            );

            return;
        }
    }


    // ========================================================
    // CHAT ID
    // ========================================================

    reminder.chatId =
        TELEGRAM_CHAT_ID;


    // ========================================================
    // CREATE
    // ========================================================

    uint32_t id =
        reminderManager.addReminder(
            reminder
        );


    if (
        id == 0
    )
    {
        server.send(
            500,
            "text/plain",
            "Failed to create reminder"
        );

        return;
    }


    Serial.printf(
        "[REMINDERS] Created reminder #%lu\n",
        (unsigned long)id
    );


    // ========================================================
    // REDIRECT
    // ========================================================

    server.sendHeader(
        "Location",
        "/reminders"
    );

    server.send(
        303
    );
}
// ============================================================
// TOGGLE REMINDER
// ============================================================

void WebServerManager::handleToggleReminder()
{
    if (
        !server.hasArg("id") ||
        !server.hasArg("enabled")
    )
    {
        server.send(
            400,
            "text/plain",
            "Missing reminder data"
        );

        return;
    }


    uint32_t id =
        server.arg(
            "id"
        ).toInt();


    bool enabled =
        server.arg(
            "enabled"
        ).toInt() == 1;


    if (
        !reminderManager.setEnabled(
            id,
            enabled
        )
    )
    {
        server.send(
            404,
            "text/plain",
            "Reminder not found"
        );

        return;
    }


    server.sendHeader(
        "Location",
        "/reminders"
    );

    server.send(
        303
    );
}
// ============================================================
// DELETE REMINDER
// ============================================================

void WebServerManager::handleDeleteReminder()
{
    if (!server.hasArg("id"))
    {
        server.send(
            400,
            "text/plain",
            "Missing reminder ID"
        );

        return;
    }


    uint32_t id =
        server.arg("id").toInt();


    if (id == 0)
    {
        server.send(
            400,
            "text/plain",
            "Invalid reminder ID"
        );

        return;
    }


    if (!reminderManager.deleteReminder(id))
    {
        Serial.printf(
            "[REMINDERS] Reminder #%lu not found\n",
            (unsigned long)id
        );

        server.send(
            404,
            "text/plain",
            "Reminder not found"
        );

        return;
    }


    Serial.printf(
        "[REMINDERS] Reminder #%lu deleted\n",
        (unsigned long)id
    );


    server.sendHeader(
        "Location",
        "/reminders"
    );

    server.send(303);
}