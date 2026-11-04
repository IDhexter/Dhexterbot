// config.h — Compile-time constants for DhexterBot OS
// Repository: https://github.com/IDhexter/Dhexterbot
// Hardware:   GeekMagic SmallTV (ESP-12F / ESP8266, 1.54" 240x240 ST7789 IPS)
#pragma once
#include "Target.h"

// ---------------------------------------------------------------------------
// Firmware Identity
// ---------------------------------------------------------------------------
#define FW_NAME       "DhexterBot"
#define FW_VERSION    "2.1.0"
#define REPO_URL      "https://github.com/IDhexter/Dhexterbot"
#define REPO_OWNER    "IDhexter"
#define REPO_NAME     "Dhexterbot"
#define FW_VARIANT    "esp8266"

// ---------------------------------------------------------------------------
// Display Wiring + Panel Quirks
// ---------------------------------------------------------------------------
#if defined(SMALLTV_ESP32C2)
  #include "board_esp32c2.h"
#elif defined(SMALLTV_ESP32_PRO)
  #include "board_esp32_pro.h"
#elif defined(SMALLTV_ESP32)
  #include "board_esp32.h"
#else
  #include "board_esp8266.h"
#endif

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#ifndef TFT_COL_OFFSET1
#define TFT_COL_OFFSET1 0
#endif
#ifndef TFT_ROW_OFFSET1
#define TFT_ROW_OFFSET1 0
#endif
#ifndef TFT_COL_OFFSET2
#define TFT_COL_OFFSET2 0
#endif
#ifndef TFT_ROW_OFFSET2
#define TFT_ROW_OFFSET2 80
#endif

// ---------------------------------------------------------------------------
// Limits & Capabilities
// ---------------------------------------------------------------------------
#define MAX_WIFI_NETS     4
#define MAX_WATCH_TARGETS 4

#define MAX_AUTH_USER_LEN 32
#define MAX_AUTH_PASS_LEN 64
#define DEFAULT_AUTH_USER "admin"
#define AUTH_REALM        "DhexterBot"

// ---------------------------------------------------------------------------
// Display Modes
// ---------------------------------------------------------------------------
#define MODE_CAROUSEL  3
#define MODE_NOTIFY    4
#define MODE_PET       7
#define DEFAULT_MODE   MODE_PET
#define DEFAULT_CAROUSEL_SEC 30

// ---------------------------------------------------------------------------
// Physical Button (for boards that define BTN_PIN)
// ---------------------------------------------------------------------------
#define BTN_ACT_NONE       0
#define BTN_ACT_SCREEN_OFF 1
#define BTN_ACT_HOME       2
#define BTN_ACT_NEXT       3
#define BTN_ACT_NETINFO    4
#define BTN_ACT_WAKE       5
#define BTN_ACT_ACK        6
#define BTN_ACT_MAX        BTN_ACT_ACK

#define BTN_LONG_MS        600
#define BTN_INFO_MS       4000
#define BTN_WAKE_MS       5000
#define BTN_CONFIRM          3

#ifdef BTN_TOUCH_RATIO
#define DEFAULT_BTN_SENS   BTN_TOUCH_RATIO
#else
#define DEFAULT_BTN_SENS   85
#endif
#define DEFAULT_BTN_SHORT  BTN_ACT_NEXT
#define DEFAULT_BTN_LONG   BTN_ACT_NETINFO

// ---------------------------------------------------------------------------
// Attention Overlay (POST /api/notify)
// ---------------------------------------------------------------------------
#define NOTIFY_TTL_DEFAULT_SEC  20
#define NOTIFY_TTL_MIN_SEC       2
#define NOTIFY_TTL_MAX_SEC     120
#define NOTIFY_QUEUE_DEPTH       2
#define NOTIFY_LABEL_MAX        48
#define NOTIFY_TITLE_MAX        20

#define NOTIFY_PRIO_LOW          0
#define NOTIFY_PRIO_MED          1
#define NOTIFY_PRIO_HIGH         2
#define NOTIFY_PRIO_MAX          3

#define NOTIFY_SCROLL_STEP_PX    4
#define NOTIFY_SCROLL_MS        40
#define NOTIFY_SCROLL_GAP_PX    28
#define NOTIFY_SCROLL_PAUSE_MS 900

// ---------------------------------------------------------------------------
// Feature Flags (Pure DhexterBot OS)
// ---------------------------------------------------------------------------
#ifndef WITH_PET
#define WITH_PET 1
#endif
#ifndef WITH_NOTIFY
#define WITH_NOTIFY 1
#endif
#define WITH_TICKER 0
#define WITH_USAGE  0
#define WITH_RADAR  0
#define WITH_HA     0
#define WITH_THEME  0

// ---------------------------------------------------------------------------
// Device Defaults
// ---------------------------------------------------------------------------
#define DEFAULT_AP_SSID      "DhexterBot-Setup"
#define DEFAULT_AP_PASS      ""
#define DEFAULT_HOSTNAME     "dhexterbot"
#define DEFAULT_BRIGHTNESS    90
#define DEFAULT_HTTP_TIMEOUT  5000

// --- Panel Colour Correction ---
#define COLOR_ORDER_AUTO   0
#define COLOR_ORDER_RGB    1
#define COLOR_ORDER_BGR    2
#define DEFAULT_COLOR_ORDER  COLOR_ORDER_AUTO
#define DEFAULT_COLOR_INVERT false
#define DEFAULT_COLOR_GAIN   100
#define MIN_COLOR_GAIN        50
#define MAX_COLOR_GAIN       150

// --- Clock / Night Mode ---
#define NTP_SERVER1             "pool.ntp.org"
#define NTP_SERVER2             "time.nist.gov"
#define DEFAULT_TZ_NAME         "America/Sao_Paulo"
#define DEFAULT_TZ_POSIX        "<-03>3"
#define DEFAULT_NIGHT_ENABLED   false
#define DEFAULT_NIGHT_START_MIN 1320      // 22:00
#define DEFAULT_NIGHT_END_MIN   420       // 07:00
#define DEFAULT_NIGHT_LEVEL     0

#define NIGHT_NTP_TRUST_MS      300000UL
#define NIGHT_NTP_RESYNC_MS      30000UL
