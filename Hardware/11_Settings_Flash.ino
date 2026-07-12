/*
  Arduino IDE tab 11/12: Settings_Flash

  Saves and reads:
    - sleep plan
    - wake plan
    - custom mode 1
    - custom mode 2
    - custom mode 3
    - autoBrightness
    - default brightness level
*/

void initSettingsFlash() {
  prefs.begin(FLASH_NAMESPACE, false);
}

void loadSettingsFromFlash() {
  brightnessLevel = prefs.getUChar("brightLevel", DEFAULT_BRIGHTNESS_LEVEL);
  brightnessLevel = constrain(brightnessLevel, 0, 3);

  autoBrightness = prefs.getBool("autoBright", false);

  planSettings.sleepLightTime = prefs.getInt("sleepLight", planSettings.sleepLightTime);
  planSettings.sleepTurnTime  = prefs.getInt("sleepTurn",  planSettings.sleepTurnTime);
  planSettings.sleepDarkTime  = prefs.getInt("sleepDark",  planSettings.sleepDarkTime);

  planSettings.wakeStartTime = prefs.getInt("wakeStart", planSettings.wakeStartTime);
  planSettings.wakeMaxTime   = prefs.getInt("wakeMax",   planSettings.wakeMaxTime);
  planSettings.wakeOffTime   = prefs.getInt("wakeOff",   planSettings.wakeOffTime);

  for (int i = 0; i < 3; i++) {
    String prefix = "m" + String(i + 1);

    modeSettings[i].r = prefs.getUChar((prefix + "r").c_str(), modeSettings[i].r);
    modeSettings[i].g = prefs.getUChar((prefix + "g").c_str(), modeSettings[i].g);
    modeSettings[i].b = prefs.getUChar((prefix + "b").c_str(), modeSettings[i].b);
    modeSettings[i].brightness = prefs.getUChar((prefix + "br").c_str(), modeSettings[i].brightness);
  }

  Serial.println("Settings loaded from flash.");
}

void saveAllSettingsToFlash() {
  savePlanToFlash();
  saveModesToFlash();
  saveUserStateToFlash();
}

void savePlanToFlash() {
  prefs.putInt("sleepLight", normalizeMinute(planSettings.sleepLightTime));
  prefs.putInt("sleepTurn",  normalizeMinute(planSettings.sleepTurnTime));
  prefs.putInt("sleepDark",  normalizeMinute(planSettings.sleepDarkTime));

  prefs.putInt("wakeStart", normalizeMinute(planSettings.wakeStartTime));
  prefs.putInt("wakeMax",   normalizeMinute(planSettings.wakeMaxTime));
  prefs.putInt("wakeOff",   normalizeMinute(planSettings.wakeOffTime));

  Serial.println("Plan settings saved.");
}

void saveModesToFlash() {
  for (int i = 0; i < 3; i++) {
    String prefix = "m" + String(i + 1);

    prefs.putUChar((prefix + "r").c_str(), modeSettings[i].r);
    prefs.putUChar((prefix + "g").c_str(), modeSettings[i].g);
    prefs.putUChar((prefix + "b").c_str(), modeSettings[i].b);
    prefs.putUChar((prefix + "br").c_str(), modeSettings[i].brightness);
  }

  Serial.println("Mode settings saved.");
}

void saveUserStateToFlash() {
  prefs.putBool("autoBright", autoBrightness);
  prefs.putUChar("brightLevel", brightnessLevel);

  Serial.println("User state saved.");
}
