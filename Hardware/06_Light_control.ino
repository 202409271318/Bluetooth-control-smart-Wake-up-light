/*
  Arduino IDE tab 6/12: Light_control

  Responsible for:
    - DEFAULT_MODE
    - CUSTOM_MODE_1
    - CUSTOM_MODE_2
    - CUSTOM_MODE_3
    - SLEEP_PLAN_MODE
    - autoBrightness
    - off, fade in, fade out
*/

static uint8_t lastShownR = 0;
static uint8_t lastShownG = 0;
static uint8_t lastShownB = 0;
static uint8_t lastShownBrightness = 255;

void initLightControl() {
  strip.begin();
  strip.clear();
  strip.setBrightness(255);
  strip.show();
}

uint8_t brightnessFromLevel(uint8_t level) {
  switch (level) {
    case 0: return 0;
    case 1: return DEFAULT_BRIGHTNESS_LOW;
    case 2: return DEFAULT_BRIGHTNESS_MID;
    case 3: return DEFAULT_BRIGHTNESS_HIGH;
    default: return DEFAULT_BRIGHTNESS_LOW;
  }
}

void showSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  lastShownR = r;
  lastShownG = g;
  lastShownB = b;
  lastShownBrightness = brightness;

  strip.setBrightness(brightness);
  uint32_t color = strip.Color(r, g, b);

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }

  strip.show();
}

void showOff() {
  showSolidColor(0, 0, 0, 0);
}

void fadeToColor(uint8_t r, uint8_t g, uint8_t b, uint8_t targetBrightness, uint16_t durationMs) {
  uint8_t startBrightness = lastShownBrightness;
  uint8_t startR = lastShownR;
  uint8_t startG = lastShownG;
  uint8_t startB = lastShownB;

  const uint8_t steps = 30;
  for (uint8_t i = 1; i <= steps; i++) {
    uint8_t nr = startR + ((int)r - startR) * i / steps;
    uint8_t ng = startG + ((int)g - startG) * i / steps;
    uint8_t nb = startB + ((int)b - startB) * i / steps;
    uint8_t nbri = startBrightness + ((int)targetBrightness - startBrightness) * i / steps;
    showSolidColor(nr, ng, nb, nbri);
    delay(durationMs / steps);
  }
}

void applyDefaultMode() {
  uint8_t brightness = autoBrightness ? getAutoBrightness() : brightnessFromLevel(brightnessLevel);

  if (brightnessLevel == 0 && !autoBrightness) {
    showOff();
    return;
  }

  showSolidColor(DEFAULT_COLOR_R, DEFAULT_COLOR_G, DEFAULT_COLOR_B, brightness);
}

void applyCustomMode(uint8_t index) {
  if (index > 2) {
    return;
  }

  if (!customOutputOn) {
    showOff();
    return;
  }

  LightModeSetting &m = modeSettings[index];
  uint8_t brightness = autoBrightness ? getAutoBrightness() : m.brightness;
  showSolidColor(m.r, m.g, m.b, brightness);
}

void applyCurrentMode() {
  switch (currentMode) {
    case DEFAULT_MODE:
      applyDefaultMode();
      break;
    case CUSTOM_MODE_1:
      applyCustomMode(0);
      break;
    case CUSTOM_MODE_2:
      applyCustomMode(1);
      break;
    case CUSTOM_MODE_3:
      applyCustomMode(2);
      break;
    case SLEEP_PLAN_MODE:
      // Sleep_plan tab directly controls the light during scheduled action.
      break;
    default:
      showOff();
      break;
  }
}

void updateLightControl() {
  if (currentMode == SLEEP_PLAN_MODE) {
    return;
  }

  if (autoBrightness && millis() - sensorState.lastAutoBrightnessApplyMs >= AUTO_BRIGHTNESS_UPDATE_MS) {
    sensorState.lastAutoBrightnessApplyMs = millis();
    applyCurrentMode();
  }
}
