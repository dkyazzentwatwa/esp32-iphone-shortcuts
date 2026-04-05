# ESP32 GPIO HTTP API - Universal

The universal sketch is the official single-firmware option for this repo. It auto-detects the chip family at runtime, selects a safe GPIO allowlist, and exposes the same Shortcut-friendly discovery API as the board-specific sketches.

## What It Supports

- `GET /health`
- `GET /info`
- `GET /system`
- `GET /wifi/status`
- `GET /wifi/scan`
- `GET /ble/scan`
- `GET /pins`
- `GET /gpio/<pin>/on`
- `GET /gpio/<pin>/off`
- `GET /gpio/<pin>/status`

## Setup

1. Copy [secrets.example.h](/Users/cypher/Documents/GitHub/esp32-iphone-shortcuts/secrets.example.h) to `secrets.h` at the repo root.
2. Fill in your Wi-Fi SSID and password.
3. Open `esp32_gpio_api_universal.ino` in Arduino IDE.
4. Select any supported ESP32 board family.
5. Set the partition scheme to `Huge APP (3MB No OTA/1MB SPIFFS)`.
6. Upload and open Serial Monitor at `115200`.

## Why Use It

- one sketch for ESP32, ESP32-S3, and ESP32-C3 boards
- the same JSON response shapes as the other sketches
- automatic safe-pin selection at boot
- Shortcut-friendly discovery features without duplicating the UI

## `/info`

The universal-only endpoint returns the detected chip and active pin allowlist:

```json
{
  "success": true,
  "ip": "192.168.1.50",
  "chip_model": "ESP32-S3",
  "detection_ok": true,
  "allowed_pins": [4,5,6,7,15,16,17,18,21,35,36,37,38,39,40,41,42],
  "pin_count": 17
}
```

## Shortcut Tip

Start with `GET /info`, then use `GET /gpio/<pin>/on` and `GET /gpio/<pin>/status` to build shortcuts that adapt to the board you plugged in.
