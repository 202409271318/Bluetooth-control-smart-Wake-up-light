/*
  Arduino IDE tab 8/12: OLED_display

  Example:
    23:10 BToff
    25.6C 60% 120
    Mod_Def AutoON
    23:40 / 08:00
*/

static unsigned long lastOledUpdateMs = 0;

void initOledDisplay() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println("OLED init failed.");
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("MEC202 Wake Light");
  display.println("Starting...");
  display.display();
}

void updateOledDisplay() {
  unsigned long now = millis();
  if (now - lastOledUpdateMs < OLED_UPDATE_MS) {
    return;
  }
  lastOledUpdateMs = now;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  String timeShort = clockState.timeValid ? (twoDigits(clockState.hour) + ":" + twoDigits(clockState.minute)) : "--:--";
  display.print(timeShort);
  display.print(bleConnected ? " BtON" : " BtOFF");

  display.setCursor(0, 16);
  if (isnan(sensorState.temperatureC)) {
    display.print("--.-C ");
  } else {
    display.print(sensorState.temperatureC, 1);
    display.print("C ");
  }

  if (isnan(sensorState.humidity)) {
    display.print("--% ");
  } else {
    display.print(sensorState.humidity, 0);
    display.print("% ");
  }

  display.print("EnvLux:");
  display.print(4095 - sensorState.lightRaw);

  display.setCursor(0, 32);
  display.print(shortModeToString(currentMode));
  display.print(" ");
  display.print(autoBrightness ? "AutoON" : "AutoOFF");

  display.setCursor(0, 48);
  display.print("Sleep");
  display.print(minuteToTimeString(planSettings.sleepDarkTime));
  display.print("/Wake");
  display.print(minuteToTimeString(planSettings.wakeOffTime));

  display.display();
}
