/*
  Arduino IDE tab 5/12: BLE

  BLE command format from WeChat Mini Program:
    TIME=20251212,231000
    SLEEP=2300,2320,2340
    WAKE=0700,0730,0800
    MODE1=255,160,60,80
    MODE2=80,180,255,100
    MODE3=180,80,255,60

  Optional commands:
    AUTO=1
    AUTO=0
    BRIGHT=0
    BRIGHT=1
    BRIGHT=2
    BRIGHT=3
    GET
*/

class WakeLightServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    bleConnected = true;
    Serial.println("BLE connected.");
  }

  void onDisconnect(BLEServer *server) override {
    bleConnected = false;
    Serial.println("BLE disconnected.");
  }
};

class WakeLightRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String input = characteristic->getValue().c_str();
    if (input.length() > 0) {
      Serial.print("BLE RX: ");
      Serial.println(input);
      processBleInput(input);
    }
  }
};

void initBle() {
  BLEDevice::init(BLE_DEVICE_NAME);

  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new WakeLightServerCallbacks());

  BLEService *service = bleServer->createService(BLE_SERVICE_UUID);

  bleTxCharacteristic = service->createCharacteristic(
    BLE_TX_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  bleTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *rxCharacteristic = service->createCharacteristic(
    BLE_RX_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  rxCharacteristic->setCallbacks(new WakeLightRxCallbacks());

  bleTxCharacteristic->setValue("READY");
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.print("BLE advertising as ");
  Serial.println(BLE_DEVICE_NAME);
}

void updateBle() {
  if (!bleConnected && oldBleConnected) {
    delay(300);
    bleServer->startAdvertising();
    oldBleConnected = bleConnected;
    Serial.println("BLE advertising restarted.");
  }

  if (bleConnected && !oldBleConnected) {
    oldBleConnected = bleConnected;
    sendBleStatus("CONNECTED");
  }
}

void sendBleStatus(const String &message) {
  if (!bleConnected || bleTxCharacteristic == nullptr) return;

  const int CHUNK = 20;          // 默认 MTU 23 - 3 ATT header = 20
  for (int i = 0; i < (int)message.length(); i += CHUNK) {
    String part = message.substring(i, min((int)message.length(), i + CHUNK));
    bleTxCharacteristic->setValue(part.c_str());
    bleTxCharacteristic->notify();
    delay(20);                   // 给手机端处理时间
  }
  // 结尾补换行，方便小程序按行拼包
  bleTxCharacteristic->setValue("\n");
  bleTxCharacteristic->notify();
}

void processBleInput(const String &input) {
  String buffer = input;
  buffer.replace("\r", "\n");
  buffer.replace(";", "\n");

  int start = 0;
  while (start < buffer.length()) {
    int end = buffer.indexOf('\n', start);
    if (end < 0) {
      end = buffer.length();
    }

    String line = buffer.substring(start, end);
    line.trim();
    if (line.length() > 0) {
      processBleCommand(line);
    }

    start = end + 1;
  }
}

void processBleCommand(const String &line) {
  if (line.equalsIgnoreCase("GET")) {
    sendBleStatus(makeStatusJson());
    return;
  }

  if (line.startsWith("TIME=")) {
    String payload = line.substring(5);
    int y, mo, d, h, mi, s;

    if (sscanf(payload.c_str(), "%4d%2d%2d,%2d%2d%2d", &y, &mo, &d, &h, &mi, &s) == 6) {
      setCurrentTimeFromBle(y, mo, d, h, mi, s);
      sendBleStatus("OK:TIME");
    } else {
      sendBleStatus("ERR:TIME");
    }
    return;
  }

  if (line.startsWith("SLEEP=")) {
    String payload = line.substring(6);
    int firstComma = payload.indexOf(',');
    int secondComma = payload.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      int t1, t2, t3;
      bool ok = parseHHMM(payload.substring(0, firstComma), t1) &&
                parseHHMM(payload.substring(firstComma + 1, secondComma), t2) &&
                parseHHMM(payload.substring(secondComma + 1), t3);

      if (ok) {
        planSettings.sleepLightTime = t1;
        planSettings.sleepTurnTime = t2;
        planSettings.sleepDarkTime = t3;
        savePlanToFlash();
        sendBleStatus("OK:SLEEP");
      } else {
        sendBleStatus("ERR:SLEEP");
      }
    } else {
      sendBleStatus("ERR:SLEEP");
    }
    return;
  }

  if (line.startsWith("WAKE=")) {
    String payload = line.substring(5);
    int firstComma = payload.indexOf(',');
    int secondComma = payload.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      int t1, t2, t3;
      bool ok = parseHHMM(payload.substring(0, firstComma), t1) &&
                parseHHMM(payload.substring(firstComma + 1, secondComma), t2) &&
                parseHHMM(payload.substring(secondComma + 1), t3);

      if (ok) {
        planSettings.wakeStartTime = t1;
        planSettings.wakeMaxTime = t2;
        planSettings.wakeOffTime = t3;
        savePlanToFlash();
        sendBleStatus("OK:WAKE");
      } else {
        sendBleStatus("ERR:WAKE");
      }
    } else {
      sendBleStatus("ERR:WAKE");
    }
    return;
  }

  if (line.startsWith("MODE1=") || line.startsWith("MODE2=") || line.startsWith("MODE3=")) {
    int index = line.substring(4, 5).toInt() - 1;
    String payload = line.substring(6);
    int r, g, b, bright;

    if (index >= 0 && index < 3 && parseCsv4Ints(payload, r, g, b, bright)) {
      modeSettings[index].r = constrain(r, 0, 255);
      modeSettings[index].g = constrain(g, 0, 255);
      modeSettings[index].b = constrain(b, 0, 255);
      modeSettings[index].brightness = constrain(bright, 0, 255);

      saveModesToFlash();
      applyCurrentMode();
      sendBleStatus("OK:MODE");
    } else {
      sendBleStatus("ERR:MODE");
    }
    return;
  }

  if (line.startsWith("AUTO=")) {
    int value = line.substring(5).toInt();
    autoBrightness = value != 0;
    saveUserStateToFlash();
    applyCurrentMode();
    sendBleStatus(autoBrightness ? "OK:AUTO=1" : "OK:AUTO=0");
    return;
  }

  if (line.startsWith("BRIGHT=")) {
    int value = line.substring(7).toInt();
    brightnessLevel = constrain(value, 0, 3);
    saveUserStateToFlash();
    applyCurrentMode();
    sendBleStatus("OK:BRIGHT");
    return;
  }

  sendBleStatus("ERR:UNKNOWN");
}

String makeStatusJson() {
  String s = "{";
  s += "\"date\":\""  + getDateString() + "\",";
  s += "\"time\":\""  + getTimeString() + "\",";
  s += "\"ble\":"      + String(bleConnected ? "true" : "false") + ",";
  s += "\"mode\":\""  + String(modeToString(currentMode)) + "\",";
  s += "\"auto\":"     + String(autoBrightness ? "true" : "false") + ",";
  s += "\"brightLevel\":" + String(brightnessLevel) + ",";
  s += "\"temp\":"     + String(sensorState.temperatureC, 1) + ",";
  s += "\"hum\":"      + String(sensorState.humidity, 0) + ",";
  s += "\"lightRaw\":" + String(sensorState.lightRaw);
  s += "}";
  return s;
}
