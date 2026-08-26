#pragma once


// ============================================================
// PROJECT
// ============================================================

// Технічна назва проєкту
#define PROJECT_NAME "Emily"

// Поточна назва пристрою.
// Поки всі старі модулі використовують DEVICE_NAME,
// залишаємо її.
#define DEVICE_NAME "Emily"


// ============================================================
// ETHERNET LAN8720
// ============================================================

#define ETH_PHY_ADDR     1
#define ETH_PHY_POWER    16
#define ETH_PHY_MDC      23
#define ETH_PHY_MDIO     18
#define ETH_PHY_TYPE     ETH_PHY_LAN8720
#define ETH_CLK_MODE     ETH_CLOCK_GPIO0_IN


// ============================================================
// TELEGRAM
// ============================================================

#define BOT_TOKEN "8206398846:AAEfAIcjoywOUI56qQjKpX5kUIQYLM0uchs"

#define ADMIN_CHAT_ID "1351588344"
#define TELEGRAM_CHAT_ID "1351588344"

#define TELEGRAM_CHECK_INTERVAL 1000


// ============================================================
// TIME / NTP
// ============================================================

// Поки TimeManager працює зі старою схемою,
// залишаємо ці значення тут.
//
// Для України:
// UTC+2 взимку
#define GMT_OFFSET_SEC        7200

// +1 година влітку
#define DAYLIGHT_OFFSET_SEC   3600

#define NTP_SERVER_1          "pool.ntp.org"
#define NTP_SERVER_2          "time.nist.gov"


// ============================================================
// STATISTICS
// ============================================================

#define STATISTICS_UPDATE_INTERVAL 1000

// 15 хвилин
#define STATISTICS_SAVE_INTERVAL 900000UL
// ============================================================
// HUB API
// ============================================================

#define HUB_API_KEY "emily-secret-key"