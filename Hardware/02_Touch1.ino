/*
  Arduino IDE tab 2/12: Touch1

  Function:
    - Short touch:
        DEFAULT_MODE: brightness level cycles 0 off -> 1 low -> 2 mid -> 3 high -> 0.
        CUSTOM_MODE_1/2/3: output cycles off -> saved custom brightness -> off.
    - Long touch for 5 seconds:
        Enter / exit auto brightness mode.
*/

static bool touch1WasPressed = false;
static bool touch1LongHandled = false;
static unsigned long touch1PressStartMs = 0;
static uint16_t touch1Baseline = 0;
static uint16_t touch1Threshold = TOUCH_DEFAULT_THRESHOLD;

void initTouchInputs() {
  delay(50);

  uint32_t sum1 = 0;
  uint32_t sum2 = 0;
  const int samples = 30;

  for (int i = 0; i < samples; i++) {
    sum1 += touchRead(PIN_TOUCH_BRIGHT);
    sum2 += touchRead(PIN_TOUCH_MODE);
    delay(5);
  }

  touch1Baseline = sum1 / samples;
  touch2Baseline = sum2 / samples;

  touch1Threshold = max<uint16_t>(TOUCH_MIN_THRESHOLD, touch1Baseline * 70 / 100);
  touch2Threshold = max<uint16_t>(TOUCH_MIN_THRESHOLD, touch2Baseline * 70 / 100);

  Serial.print("Touch1 baseline=");
  Serial.print(touch1Baseline);
  Serial.print(" threshold=");
  Serial.println(touch1Threshold);

  Serial.print("Touch2 baseline=");
  Serial.print(touch2Baseline);
  Serial.print(" threshold=");
  Serial.println(touch2Threshold);
}

bool isTouch1Pressed() {
  uint16_t value = touchRead(PIN_TOUCH_BRIGHT);
  return value < touch1Threshold;
}

void handleTouch1ShortPress() {
  if (autoBrightness) {
    applyCurrentMode();
    return;
  }

  if (currentMode == DEFAULT_MODE) {
    brightnessLevel = (brightnessLevel + 1) % 4;
    Serial.print("Default brightness level: ");
    Serial.println(brightnessLevel);
  } else if (currentMode == CUSTOM_MODE_1 ||
             currentMode == CUSTOM_MODE_2 ||
             currentMode == CUSTOM_MODE_3) {
    customOutputOn = !customOutputOn;
    Serial.print("Custom output: ");
    Serial.println(customOutputOn ? "ON" : "OFF");
  }

  saveUserStateToFlash();
  applyCurrentMode();
}

void handleTouch1LongPress() {
  autoBrightness = !autoBrightness;
  Serial.print("Auto brightness: ");
  Serial.println(autoBrightness ? "ON" : "OFF");

  saveUserStateToFlash();
  applyCurrentMode();
}

void updateTouch1() {
  bool pressed = isTouch1Pressed();
  unsigned long now = millis();

  if (pressed && !touch1WasPressed) {
    touch1WasPressed = true;
    touch1LongHandled = false;
    touch1PressStartMs = now;
  }

  if (pressed && touch1WasPressed && !touch1LongHandled) {
    if (now - touch1PressStartMs >= TOUCH_LONG_PRESS_MS) {
      touch1LongHandled = true;
      handleTouch1LongPress();
    }
  }

  if (!pressed && touch1WasPressed) {
    unsigned long duration = now - touch1PressStartMs;
    touch1WasPressed = false;

    if (!touch1LongHandled && duration >= TOUCH_DEBOUNCE_MS) {
      handleTouch1ShortPress();
    }
  }
}
