#pragma once

/*
  Arduino IDE tab 12/12: Config_Types

  Contains:
    - pin definitions
    - LED number
    - BLE UUID
    - default values
    - structs
    - enums
    - global variable declarations
*/

#include <Arduino.h>

// ---------- Pin mapping from development document ----------
#define PIN_LED_DIN       18
#define PIN_OLED_SCL      22
#define PIN_OLED_SDA      21
#define PIN_TOUCH_BRIGHT  32
#define PIN_TOUCH_MODE    33
#define PIN_LIGHT_AO      34
#define PIN_DHT11         25

// ---------- LED ----------
#define LED_COUNT 128       // WS2812B 8*8*2 cascaded
#define DEFAULT_COLOR_R 255
#define DEFAULT_COLOR_G 170
#define DEFAULT_COLOR_B 80

#define DEFAULT_BRIGHTNESS_LEVEL 1
#define DEFAULT_BRIGHTNESS_LOW   45
#define DEFAULT_BRIGHTNESS_MID   120
#define DEFAULT_BRIGHTNESS_HIGH  230

// ---------- Sleep / wake plan light ----------
#define PLAN_WARM_R 255
#define PLAN_WARM_G 170
#define PLAN_WARM_B 70
#define PLAN_MAX_BRIGHTNESS 230

// ---------- OLED ----------
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C

// ---------- Touch ----------
#define TOUCH_DEBOUNCE_MS 80
#define TOUCH_LONG_PRESS_MS 2500
#define TOUCH_DEFAULT_THRESHOLD 35
#define TOUCH_MIN_THRESHOLD 12

// ---------- Sensors / timing ----------
#define LOOP_DELAY_MS 20
#define TEMP_HUMIDITY_UPDATE_MS 2000
#define LIGHT_SENSOR_UPDATE_MS 400
#define AUTO_BRIGHTNESS_UPDATE_MS 700
#define OLED_UPDATE_MS 500

#define AUTO_BRIGHTNESS_MIN 10
#define AUTO_BRIGHTNESS_MAX 255

// ---------- Flash ----------
#define FLASH_NAMESPACE "wakeLight"

// ---------- BLE ----------
#define BLE_DEVICE_NAME "WakeLight"
#define BLE_SERVICE_UUID ""
#define BLE_RX_CHAR_UUID ""
#define BLE_TX_CHAR_UUID ""

// ---------- Enums ----------
enum DeviceMode {
  DEFAULT_MODE = 0,
  CUSTOM_MODE_1,
  CUSTOM_MODE_2,
  CUSTOM_MODE_3,
  SLEEP_PLAN_MODE
};

// ---------- Structs ----------
struct LightModeSetting {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t brightness;
};

struct PlanSettings {
  int sleepLightTime;
  int sleepTurnTime;
  int sleepDarkTime;
  int wakeStartTime;
  int wakeMaxTime;
  int wakeOffTime;
};

struct ClockState {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  unsigned long baseMillis;
  bool timeValid;
};

struct SensorState {
  float temperatureC;
  float humidity;
  int lightRaw;
  uint8_t autoBrightnessValue;

  unsigned long lastTempHumidityMs;
  unsigned long lastLightSensorMs;
  unsigned long lastAutoBrightnessApplyMs;
  unsigned long lastOledMs;
};

// ---------- Global declarations ----------
extern DeviceMode currentMode;
extern DeviceMode previousManualMode;

extern bool autoBrightness;
extern uint8_t brightnessLevel;
extern bool customOutputOn;

extern LightModeSetting modeSettings[3];
extern PlanSettings planSettings;
extern ClockState clockState;
extern SensorState sensorState;

extern bool bleConnected;

// Touch2 variables are shared with Touch1 init calibration.
extern bool touch2WasPressed;
extern unsigned long touch2PressStartMs;
extern uint16_t touch2Baseline;
extern uint16_t touch2Threshold;

// ---------- Helper functions ----------
inline int normalizeMinute(int minuteOfDay) {
  while (minuteOfDay < 0) {
    minuteOfDay += 24 * 60;
  }
  return minuteOfDay % (24 * 60);
}

inline const char *modeToString(DeviceMode mode) {
  switch (mode) {
    case DEFAULT_MODE: return "Default";
    case CUSTOM_MODE_1: return "Mode1";
    case CUSTOM_MODE_2: return "Mode2";
    case CUSTOM_MODE_3: return "Mode3";
    case SLEEP_PLAN_MODE: return "Plan";
    default: return "Unknown";
  }
}

inline const char *shortModeToString(DeviceMode mode) {
  switch (mode) {
    case DEFAULT_MODE: return "Mod_Def";
    case CUSTOM_MODE_1: return "Mod_1";
    case CUSTOM_MODE_2: return "Mod_2";
    case CUSTOM_MODE_3: return "Mod_3";
    case SLEEP_PLAN_MODE: return "Mod_Plan";
    default: return "Mod_?";
  }
}

// 解析紧凑时间字符串 "HHMM" → 当日分钟数 (0..1439)
// 例: "2300" → 1380,  "0700" → 420
inline bool parseHHMM(const String &text, int &minuteOut) {
  String t = text;
  t.trim();

  if (t.length() != 4) return false;

  // 必须全是数字
  for (int i = 0; i < 4; i++) {
    if (!isDigit(t.charAt(i))) return false;
  }

  int h = t.substring(0, 2).toInt();
  int m = t.substring(2, 4).toInt();

  if (h < 0 || h > 23 || m < 0 || m > 59) return false;

  minuteOut = h * 60 + m;
  return true;
}

inline bool parseCsv4Ints(const String &text, int &a, int &b, int &c, int &d) {
  return sscanf(text.c_str(), "%d,%d,%d,%d", &a, &b, &c, &d) == 4;
}

// 12_Config_Types.h 末尾追加声明：
String getTimeString();
String getDateString();
String getDateTimeString();
String makeStatusJson();
String minuteToTimeString(int minuteOfDay);

void setCurrentTimeFromBle(int year, int month, int day,
                           int hour, int minute, int second);

void savePlanToFlash();
void saveModesToFlash();
void saveUserStateToFlash();
void applyCurrentMode();
void showSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);
void showOff();
void sendBleStatus(const String &message);

