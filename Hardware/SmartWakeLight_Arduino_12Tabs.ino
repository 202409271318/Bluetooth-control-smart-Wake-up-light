/*
  MEC202 Light & Sensor Project
  Bluetooth Control Smart Wake-up Light

  Arduino IDE tab 1/12: Main
  Board: ESP32 DevKitC / ESP32-WROOM-32E
  Required libraries:
    - Adafruit NeoPixel
    - Adafruit GFX Library
    - Adafruit SSD1306
    - DHT sensor library
    - ESP32 BLE Arduino (included with many ESP32 Arduino cores)
*/

#include "12_Config_Types.h"

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ---------- Hardware objects ----------
Preferences prefs;
Adafruit_NeoPixel strip(LED_COUNT, PIN_LED_DIN, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
DHT dht(PIN_DHT11, DHT11);

// ---------- BLE objects ----------
BLEServer *bleServer = nullptr;
BLECharacteristic *bleTxCharacteristic = nullptr;
bool bleConnected = false;
bool oldBleConnected = false;

// ---------- Global state ----------
DeviceMode currentMode = DEFAULT_MODE;
DeviceMode previousManualMode = DEFAULT_MODE;

bool autoBrightness = false;
uint8_t brightnessLevel = DEFAULT_BRIGHTNESS_LEVEL;
bool customOutputOn = true;

LightModeSetting modeSettings[3] = {
  {255, 160, 60,  80},
  {80,  180, 255, 100},
  {180, 80,  255, 60}
};

PlanSettings planSettings = {
  23 * 60 + 0,   // sleepLightTime
  23 * 60 + 20,  // sleepTurnTime
  23 * 60 + 40,  // sleepDarkTime
  7  * 60 + 0,   // wakeStartTime
  7  * 60 + 30,  // wakeMaxTime
  8  * 60 + 0    // wakeOffTime
};

ClockState clockState = {2025, 1, 1, 0, 0, 0, 0, false};

SensorState sensorState = {
  NAN, NAN, 0, 0,
  0, 0,
  0, 0
};

// ---------- Function prototypes ----------
void initSettingsFlash();
void loadSettingsFromFlash();
void saveAllSettingsToFlash();
void savePlanToFlash();
void saveModesToFlash();
void saveUserStateToFlash();

void initBle();
void updateBle();
void processBleInput(const String &input);
void sendBleStatus(const String &message);

void initLightControl();
void updateLightControl();
void applyCurrentMode();
void showSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
void showOff();
uint8_t brightnessFromLevel(uint8_t level);

void initTouchInputs();
void updateTouch1();
void updateTouch2();

void initSleepPlan();
void updateSleepPlan();
bool isPlanActiveNow();

void initCountTime();
void updateCountTime();
void setCurrentTimeFromBle(int year, int month, int day, int hour, int minute, int second);
int getMinuteOfDay();
String getTimeString();
String minuteToTimeString(int minuteOfDay);

void initOledDisplay();
void updateOledDisplay();

void initTempHumiditySensor();
void updateTempHumiditySensor();

void initLightSensor();
void updateLightSensor();
uint8_t getAutoBrightness();

bool parseHHMM(const String &text, int &minuteOut);
bool parseCsv4Ints(const String &text, int &a, int &b, int &c, int &d);

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("MEC202 Smart Wake-up Light starting...");

  initSettingsFlash();
  loadSettingsFromFlash();

  initCountTime();
  initLightControl();
  initTouchInputs();
  initOledDisplay();
  initTempHumiditySensor();
  initLightSensor();
  initSleepPlan();
  initBle();

  applyCurrentMode();
  Serial.println("Setup complete.");
}

void loop() {
  updateCountTime();
  updateBle();

  updateTouch1();
  updateTouch2();

  updateTempHumiditySensor();
  updateLightSensor();

  updateSleepPlan();
  updateLightControl();

  updateOledDisplay();

  delay(LOOP_DELAY_MS);
}
