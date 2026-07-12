# SmartWakeLight_Arduino_12Tabs

这是根据开发文档整理的 Arduino IDE 12 个 tab 版本 ESP32 程序。

## 硬件对应

- ESP32 DevKitC / ESP32-WROOM-32E
- WS2812B 8x8 * 2 级联，DIN = GPIO18
- OLED SSD1306 I2C，SCL = GPIO22，SDA = GPIO21
- Touch Bright = GPIO32
- Touch Mode = GPIO33
- GL5516 光敏 AO = GPIO34
- DHT11 DAT = GPIO25
- USB-C 5V 供电，WS2812B 与 ESP32 共地

## Arduino 库

请在 Arduino IDE Library Manager 安装：

- Adafruit NeoPixel
- Adafruit GFX Library
- Adafruit SSD1306
- DHT sensor library
- ESP32 Arduino core / BLE 库

## BLE 指令

参考format.txt

## 打开方式

把整个 `SmartWakeLight_Arduino_12Tabs` 文件夹放到 Arduino sketchbook 目录，打开 `SmartWakeLight_Arduino_12Tabs.ino`。
