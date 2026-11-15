// Watch V1 - hardware pins and factory defaults.
//
// Pins must match hardware/scripts/design.py (the source of the schematic).
// Everything under "Factory defaults" is only the starting point: each watch
// keeps its own settings in flash, and the owner changes them from the
// Watch Studio app (app/index.html) over USB or Bluetooth.
// See docs/CONFIGURATION.md.
#pragma once

#define FW_VERSION "2.2.0"

// ---------- Pins (ESP32-C3 SuperMini exposes ONLY GPIO0-10, 20, 21) ----------
#define PIN_VBAT_SENSE 0   // ADC1_CH0, battery through 100k/100k divider; accessory AIN rides on it via C4
#define PIN_AIN        PIN_VBAT_SENSE
#define PIN_LDR_SENSE  1   // ADC1_CH1, LDR / 10k divider midpoint
//      GPIO2              // strapping pin, deliberately left unconnected
#define PIN_BTN_MODE   3   // tactile button to GND (deep-sleep wake capable)
#define PIN_BTN_SET    4   // tactile button to GND (deep-sleep wake capable)
#define PIN_CHRG       5   // TP4056 CHRG pad (LOW = charging)
#define PIN_SR_DATA    6   // 74HC595 SER   (U2 pin 14)
#define PIN_SR_CLK     7   // 74HC595 SRCLK (U2+U3 pin 11)
#define PIN_SDA        8   // I2C SDA -> DS3231 + accessory socket J4 (also the on-board LED)
#define PIN_SCL        9   // I2C SCL -> DS3231 + accessory socket J4 (also the BOOT button)
#define PIN_SR_LATCH   10  // 74HC595 RCLK  (U2+U3 pin 12)
#define PIN_SR_OE      20  // 74HC595 /OE   (U2+U3 pin 13), 10k pull-up, PWM = brightness
#define PIN_LDR_PWR    21  // powers the LDR divider and accessory pin AUX_PWR, only while awake

// ---------- Shift-register bit layout ----------
// 16 bits are shifted out MSB first: high byte lands in U3, low byte in U2.
// U2 (low byte):  Q0..Q7 = segments a b c d e f g dp
// U3 (high byte): Q0..Q3 = digit 1..4 (HIGH = digit on), Q4 R, Q5 G, Q6 B, Q7 haptic motor
#define AUX_LED_R  0x10
#define AUX_LED_G  0x20
#define AUX_LED_B  0x40
#define AUX_HAPTIC 0x80

// Which digit's decimal point is wired to the colon. On 5641AS-style displays
// there is no colon and this simply lights the dot between hours and minutes.
#define COLON_DIGIT 1          // 0-based: digit 2

// ---------- Factory defaults (changeable later without reflashing) ----------
#define DEFAULT_NAME        "Watch"
// POSIX TZ string. The default is plain UTC; Watch Studio's Quick setup sets the
// owner's zone from the computer's clock. Examples:
//   "JST-9" Tokyo, "GMT0BST,M3.5.0/1,M10.5.0" London, "EST5EDT,M3.2.0,M11.1.0" New York
#define DEFAULT_TZ          "UTC0"
#define DEFAULT_NTP         "pool.ntp.org"
// The project's time + weather service (https://pymite6941.is-a.bot/api/watch).
// The time comes from it first and from NTP if it does not answer. It allows
// about 30 requests a minute in total, which is plenty for a class set of
// watches syncing weekly and fetching weather hourly. If you build many watches,
// or your own fork, point these at your own service or set DEFAULT_TIME_SRC "ntp".
#define SERVICE_URL         "https://pymite6941.is-a.bot/api/watch"
#define DEFAULT_TIME_SRC    "http"
#define DEFAULT_TIME_URL    SERVICE_URL
#define DEFAULT_TIME_KEY    "unix"
#define DEFAULT_WX_URL      SERVICE_URL
#define DEFAULT_WX_MIN      60     // weather every hour while the weather screen is in use
#define WX_MIN_MINUTES      10     // the service only refreshes weather this often
#define DEFAULT_AWAKE_S     5      // display stays on this long after the last button press
#define DEFAULT_RESYNC_H    168    // with a DS3231 fitted: weekly is plenty
#define RESYNC_S_NO_RTC     (3UL * 3600)   // internal clock only: drifts, so every 3 h
#define DEFAULT_TIMER_S     300
#define DEFAULT_DICE        6
#define DEFAULT_MESH_CH     1
#define DEFAULT_MESH_GROUP  "class"

#define LOG_CAPACITY 240         // environment samples kept (in sleep-proof RAM; lost if the battery dies)

// ---------- Limits ----------
#define MAX_WIFI     5
#define MAX_ALERTS   16

// ---------- Fixed behaviour ----------
#define WIFI_TIMEOUT_MS  8000
#define NTP_TIMEOUT_MS   6000
#define HTTP_TIMEOUT_MS  6000
#define SET_TIMEOUT_MS   20000   // leave set mode if nothing is pressed
#define APP_AWAKE_MS     120000  // a running stopwatch / timer / mesh view keeps the display on this long
#define LINK_AWAKE_MS    180000  // link (Bluetooth) mode lasts this long after the last command
#define LED_MAX_MS       60000   // RGB LED / party mode auto-off
#define LONG_PRESS_MS    800
#define DEBOUNCE_MS      25
#define MUX_PERIOD_US    2000    // per digit -> 125 Hz full refresh
#define LDR_DARK_MV      150     // divider reading in a dark room
#define LDR_BRIGHT_MV    2400    // divider reading in bright light
#define VBAT_EMPTY_MV    3500
#define VBAT_FULL_MV     4150
#define VBAT_DIVIDER     2.0f    // (R18 + R19) / R19
