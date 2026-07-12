/*
  Arduino IDE tab 9/12: Temp_humidity_sensor

  Development document uses DAT_DHT11 on GPIO25.
  If your hardware later changes to AHT20, replace this tab with an AHT20 library implementation.
*/

void initTempHumiditySensor() {
  dht.begin();
  sensorState.lastTempHumidityMs = 0;
}

void updateTempHumiditySensor() {
  unsigned long now = millis();
  if (now - sensorState.lastTempHumidityMs < TEMP_HUMIDITY_UPDATE_MS) {
    return;
  }

  sensorState.lastTempHumidityMs = now;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    sensorState.humidity = h;
    sensorState.temperatureC = t;
  } else {
    Serial.println("DHT11 read failed.");
  }
}
