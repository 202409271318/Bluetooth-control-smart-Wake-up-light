/*
  Arduino IDE tab 3/12: Touch2

  Function:
    Touch2 cycles lighting mode:
      DEFAULT_MODE -> CUSTOM_MODE_1 -> CUSTOM_MODE_2 -> CUSTOM_MODE_3 -> DEFAULT_MODE.
*/

bool touch2WasPressed = false;
unsigned long touch2PressStartMs = 0;
uint16_t touch2Baseline = 0;
uint16_t touch2Threshold = TOUCH_DEFAULT_THRESHOLD;

bool isTouch2Pressed() {
  uint16_t value = touchRead(PIN_TOUCH_MODE);
  return value < touch2Threshold;
}

void handleTouch2ShortPress() {
  if (currentMode == SLEEP_PLAN_MODE) {
    // Sleep plan has highest priority. Manual mode switch is ignored during plan action.
    return;
  }

  switch (currentMode) {
    case DEFAULT_MODE:
      currentMode = CUSTOM_MODE_1;
      break;
    case CUSTOM_MODE_1:
      currentMode = CUSTOM_MODE_2;
      break;
    case CUSTOM_MODE_2:
      currentMode = CUSTOM_MODE_3;
      break;
    case CUSTOM_MODE_3:
    default:
      currentMode = DEFAULT_MODE;
      break;
  }

  customOutputOn = true;
  previousManualMode = currentMode;

  Serial.print("Mode changed to: ");
  Serial.println(modeToString(currentMode));

  applyCurrentMode();
}

void updateTouch2() {
  bool pressed = isTouch2Pressed();
  unsigned long now = millis();

  if (pressed && !touch2WasPressed) {
    touch2WasPressed = true;
    touch2PressStartMs = now;
  }

  if (!pressed && touch2WasPressed) {
    unsigned long duration = now - touch2PressStartMs;
    touch2WasPressed = false;

    if (duration >= TOUCH_DEBOUNCE_MS && duration < TOUCH_LONG_PRESS_MS) {
      handleTouch2ShortPress();
    }
  }
}
