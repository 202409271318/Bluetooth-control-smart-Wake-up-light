/*
  Arduino IDE tab 10/12: Light_sensor

  GL5516 module:
    AO_Light -> GPIO34 ADC input

  In autoBrightness mode:
    brightness = map(lightValue, 0, 4095, 0, 255)
    brightness = constrain(brightness, 0, 255)
*/

void initLightSensor() {
  pinMode(PIN_LIGHT_AO, INPUT);
  sensorState.lastLightSensorMs = 0;
}

void updateLightSensor() {
  unsigned long now = millis();
  if (now - sensorState.lastLightSensorMs < LIGHT_SENSOR_UPDATE_MS) {
    return;
  }

  sensorState.lastLightSensorMs = now;

  int raw = analogRead(PIN_LIGHT_AO);
  sensorState.lightRaw = constrain(raw, 0, 4095);
  sensorState.autoBrightnessValue = getAutoBrightness();
}

uint8_t getAutoBrightness() {
  int brightness = map(sensorState.lightRaw, 0, 4095, AUTO_BRIGHTNESS_MIN, AUTO_BRIGHTNESS_MAX);
  brightness = constrain(brightness, AUTO_BRIGHTNESS_MIN, AUTO_BRIGHTNESS_MAX);
  return (uint8_t)brightness;
}
