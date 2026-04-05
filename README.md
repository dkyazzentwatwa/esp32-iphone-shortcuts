# ESP32 iPhone Shortcuts Bridge

Turn Apple Shortcuts into a simple homegrown IoT remote for ESP32 boards.

This project gives ESP32-based boards a tiny HTTP API that works well with the iPhone Shortcuts app. The result is a lightweight bridge between Apple automation and physical hardware: tap a shortcut, call a local URL, and toggle a GPIO pin on your board.

It is intentionally small, easy to understand, and beginner-friendly:

- No cloud dependency
- No MQTT broker
- No custom iOS app
- No complex firmware framework
- Just Wi-Fi, HTTP, JSON, and Apple Shortcuts

## What This Project Does

Each sketch connects an ESP32 board to your local Wi-Fi network and starts a web server on port `80`.

From your iPhone, iPad, Mac, browser, or any device on the same LAN, you can call routes like:

- `GET /health`
- `GET /system`
- `GET /wifi/status`
- `GET /wifi/scan`
- `GET /ble/scan`
- `GET /gpio/<pin>/on`
- `GET /gpio/<pin>/off`
- `GET /gpio/<pin>/status`
- `GET /pins` on the multi-pin variants

That means Apple Shortcuts can act as a clean control layer for:

- LEDs
- relays
- simple automation triggers
- desk gadgets
- maker demos
- low-friction local IoT experiments

## Why It’s Useful

The main accomplishment in this repo is the bridge itself:

- Apple Shortcuts becomes the user interface
- the ESP32 becomes the local hardware endpoint
- HTTP becomes the simple control protocol

This lets you build automations such as:

- a Home Screen button that turns an LED strip relay on
- a voice shortcut that powers a desk fan through a relay module
- a one-tap scene that flips multiple pins using the multi-pin variants
- a quick hardware dashboard that checks pin states with `/pins`

For many personal IoT projects, that is enough. You get a practical control path from iPhone to GPIO without having to build a full app or backend.

## Repository Layout

This repository contains several standalone Arduino sketches, each targeting a specific board or control style:

| Folder | Board / Variant | What it does |
| --- | --- | --- |
| `esp32_gpio_api/` | ESP32 single-pin | Exposes one default pin with `on`, `off`, and `status` routes |
| `esp32_gpio_api_all/` | ESP32 multi-pin | Exposes a safe allowlist of pins plus `/pins` |
| `esp32_gpio_api_s3/` | ESP32-S3 single-pin | Single-pin variant for ESP32-S3 boards |
| `esp32_gpio_api_s3_all/` | ESP32-S3 multi-pin | Multi-pin ESP32-S3 variant with `/pins` |
| `esp32_c3_supermini_gpio_api_all/` | ESP32-C3 Super Mini multi-pin | Multi-pin variant for common C3 Super Mini boards |
| `esp32_gpio_api_universal/` | Official universal sketch | Auto-detects ESP32 family and exposes the same Shortcut API |

## Feature Highlights

- Local HTTP API for GPIO control
- Discovery endpoints for Wi-Fi, BLE, and system diagnostics
- Official universal sketch for auto-detected board families
- JSON responses that are easy to consume in Apple Shortcuts
- `GET /health` endpoint for quick connectivity checks
- Optional mDNS hostname support via `http://esp32-gpio.local`
- Safe output pin allowlists for multi-pin builds
- Beginner-friendly structure with very little abstraction

## How The Bridge Works

The flow is simple:

1. Flash one of the sketches to your ESP32 board.
2. The board joins your Wi-Fi network and prints its IP address in Serial Monitor.
3. Apple Shortcuts calls the board with `Get Contents of URL`.
4. The sketch sets or reads the requested GPIO pin.
5. The board responds with JSON.

Example:

```text
iPhone Shortcut
  -> GET http://192.168.1.50/gpio/23/on
  -> ESP32 sets GPIO 23 HIGH
  -> Shortcut receives JSON confirmation
```

Example JSON response:

```json
{
  "success": true,
  "pin": 23,
  "state": "on",
  "ip": "192.168.1.50"
}
```

## Quick Start

If you want the fastest path:

1. Pick the sketch that matches your board.
2. Open the sketch in Arduino IDE.
3. Replace `ssid` and `password` with your Wi-Fi credentials.
4. Select the correct ESP32 board and port.
5. Upload the sketch.
6. Open Serial Monitor at `115200`.
7. Copy the board IP address.
8. Test `/health` in a browser.
9. Create an Apple Shortcut that calls one of the GPIO URLs.

## Beginner Setup Guide

### 1. Hardware You Need

- an ESP32, ESP32-S3, or ESP32-C3 Super Mini board
- a USB cable for programming
- the Arduino IDE
- an iPhone with the Shortcuts app
- something simple to control, like an LED or relay

### 2. Install ESP32 Board Support

In Arduino IDE:

1. Open `Settings`
2. Add the ESP32 boards package URL if needed
3. Open `Boards Manager`
4. Search for `esp32`
5. Install Espressif's ESP32 package

### 3. Open The Right Sketch

Choose the folder that matches your board and use case:

- use a `single-pin` sketch if you only need one GPIO endpoint
- use an `all` sketch if you want multiple safe pins and `/pins`
- use `esp32_gpio_api_universal/` if you want one official sketch that auto-detects the board family

### 4. Set Wi-Fi Credentials

Copy [secrets.example.h](/Users/cypher/Documents/GitHub/esp32-iphone-shortcuts/secrets.example.h) to `secrets.h` at the repo root, then update:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

The sketches include `../secrets.h`, and `.gitignore` keeps the local file out of git.

### 5. Select The Board

Typical board selections:

- `ESP32 Dev Module` for standard ESP32 boards
- `ESP32S3 Dev Module` for ESP32-S3 boards
- the closest compatible ESP32-C3 option for your C3 Super Mini

Set the partition scheme to `Huge APP (3MB No OTA/1MB SPIFFS)` for every sketch in this repo. The discovery features, including BLE scanning, push these builds beyond the default 1.2MB app partition.

### 6. Upload The Sketch

Upload from Arduino IDE, then open Serial Monitor at `115200 baud`.

After boot, the sketch prints:

- Wi-Fi connection status
- local IP address
- example URLs
- mDNS info when available

### 7. Test In A Browser

Try:

```text
http://<board-ip>/health
```

You should get:

```text
OK
```

Then try a pin route such as:

```text
http://<board-ip>/gpio/23/on
```

or:

```text
http://<board-ip>/pins
```

depending on the sketch you flashed.

## Apple Shortcuts Setup

This is the simplest basic shortcut setup for beginners.

### Example: Turn A Pin On

1. Open the `Shortcuts` app on iPhone.
2. Tap `+` to create a new shortcut.
3. Tap `Add Action`.
4. Search for `Get Contents of URL`.
5. In the URL field, enter something like:

```text
http://192.168.1.50/gpio/23/on
```

6. Leave the method as `GET`.
7. Optionally add `Quick Look` or `Show Result` to inspect the JSON response.
8. Rename the shortcut to something like `Desk Light On`.
9. Run it.

### Example: Turn A Pin Off

Use:

```text
http://192.168.1.50/gpio/23/off
```

### Example: Check Pin Status

Use:

```text
http://192.168.1.50/gpio/23/status
```

### Example: Read All Safe Pins

For the multi-pin variants:

```text
http://192.168.1.50/pins
```

### Basic Shortcut Pattern

For a beginner-friendly control shortcut, use:

1. `Get Contents of URL`
2. `Get Dictionary from Input` if you want to parse the JSON
3. `Show Result` to display `success`, `pin`, or `state`

That gives you a reusable pattern you can duplicate for any endpoint in the project.

## API Guide

### Common Routes

All sketches include:

- `GET /`
- `GET /health`

The main `esp32_gpio_api_all/` sketch also includes:

- `GET /system`
- `GET /wifi/status`
- `GET /wifi/scan`
- `GET /ble/scan`

Pin routes:

- `GET /gpio/<pin>/on`
- `GET /gpio/<pin>/off`
- `GET /gpio/<pin>/status`

Multi-pin sketches also include:

- `GET /pins`

### Example Requests

Standard ESP32 single-pin sketch:

```text
GET /gpio/23/on
GET /gpio/23/off
GET /gpio/23/status
```

ESP32 multi-pin sketch:

```text
GET /system
GET /wifi/status
GET /wifi/scan
GET /ble/scan
GET /gpio/18/on
GET /gpio/23/off
GET /gpio/33/status
GET /pins
```

ESP32-S3 single-pin sketch:

```text
GET /gpio/4/on
GET /gpio/4/off
GET /gpio/4/status
```

ESP32-S3 multi-pin sketch:

```text
GET /gpio/4/on
GET /gpio/17/off
GET /gpio/42/status
GET /pins
```

ESP32-C3 Super Mini multi-pin sketch:

```text
GET /gpio/0/on
GET /gpio/4/off
GET /gpio/21/status
GET /pins
```

### Example JSON Responses

Successful pin update:

```json
{
  "success": true,
  "pin": 4,
  "state": "on",
  "ip": "192.168.1.50"
}
```

Invalid pin:

```json
{
  "success": false,
  "pin": 9,
  "state": "unknown",
  "ip": "192.168.1.50",
  "message": "Pin is not in the safe output allowlist."
}
```

`/pins` response:

```json
{
  "success": true,
  "ip": "192.168.1.50",
  "pins": [
    { "pin": 18, "state": "off" },
    { "pin": 19, "state": "off" },
    { "pin": 23, "state": "on" }
  ]
}
```

`/wifi/status` response:

```json
{
  "success": true,
  "connected": true,
  "ssid": "thanos lives forever",
  "ip": "10.0.0.176",
  "hostname": "esp32-gpio",
  "mac": "C0:5D:89:DE:14:58",
  "rssi": -20,
  "channel": 11
}
```

`/wifi/scan` response:

```json
{
  "success": true,
  "count": 2,
  "ip": "10.0.0.176",
  "networks": [
    { "ssid": "thanos lives forever", "rssi": -26, "channel": 11, "encryption": "wpa2_psk", "open": false },
    { "ssid": "Neighbor WiFi", "rssi": -78, "channel": 6, "encryption": "wpa2_wpa3_psk", "open": false }
  ]
}
```

`/system` response:

```json
{
  "success": true,
  "ip": "10.0.0.176",
  "hostname": "esp32-gpio",
  "uptime_ms": 11365,
  "free_heap": 180968,
  "min_free_heap": 177516,
  "chip_model": "ESP32-D0WD-V3",
  "chip_revision": 301,
  "cpu_mhz": 240,
  "flash_size": 4194304,
  "sdk_version": "v5.5.2-249-gf56bea3d1f"
}
```

`/ble/scan` unsupported response:

```json
{
  "success": false,
  "supported": false,
  "message": "BLE scanning is not available in this build.",
  "ip": "10.0.0.176"
}
```

## Safe Pin Notes

The multi-pin sketches intentionally expose only conservative allowlists.

That matters because some ESP32-family GPIOs can be tied to:

- boot behavior
- USB functions
- flash access
- onboard buttons
- onboard LEDs
- board-specific electrical quirks

If you expand the allowlist, verify the board pinout first.

Notable examples already reflected in the repo:

- the ESP32-C3 Super Mini sketch leaves out `GPIO8` and `GPIO9`
- the ESP32-S3 sketches use a limited safe set rather than exposing every pin

## mDNS Support

The sketches try to advertise a local hostname:

```text
http://esp32-gpio.local
```

If your network and device support mDNS, you can use that instead of a raw IP.

If not, use the IP address shown in Serial Monitor.

## Build And Validation

Arduino IDE is the default workflow, but `arduino-cli` is useful for quick compile checks.

Examples:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app esp32_gpio_api
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app esp32_gpio_api_all
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app esp32_gpio_api_universal
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app esp32_gpio_api_s3
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app esp32_gpio_api_s3_all
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=huge_app esp32_c3_supermini_gpio_api_all
```

For the ESP32-C3 Super Mini variant, use the matching ESP32-C3 FQBN available in your local board package.

After flashing, validate:

- boot logs in Serial Monitor
- successful Wi-Fi connection
- `GET /health`
- one `on` request
- one `off` request
- one `status` request
- `/pins` if you are using a multi-pin variant

## Beginner Demo Ideas

Here are a few simple demos that fit this repo well:

- `LED Button`: tap a shortcut to turn an LED on and off
- `Relay Switch`: trigger a relay from your iPhone
- `Desk Mode`: create two shortcuts, one for `on` and one for `off`
- `GPIO Dashboard`: use `/pins` with Shortcuts to display current pin states
- `Wi-Fi Scanner`: call `/wifi/scan` and show nearby SSIDs in Shortcuts
- `Signal Check`: call `/wifi/status` before running a GPIO shortcut
- `ESP32 Health Card`: call `/system` to show uptime, heap, and chip details
- `Voice Trigger`: run the shortcut through Siri

## Security And Sharing Notes

This project is designed for trusted local-network use.

Important precautions:

- do not expose these endpoints directly to the public internet
- do not commit real Wi-Fi credentials
- replace `ssid` and `password` with placeholders before publishing changes
- verify electrical safety before connecting relays, loads, or external power circuits

## Troubleshooting

### The Board Does Not Respond

- confirm the board joined Wi-Fi
- confirm the IP address in Serial Monitor
- make sure your iPhone is on the same local network
- test `http://<board-ip>/health` in a browser first

### The Shortcut Fails

- confirm the URL is correct
- confirm the method is `GET`
- test the same URL in Safari
- add `Show Result` to inspect what the shortcut receives

### A Pin Does Not Work

- confirm that pin is in the sketch allowlist
- check your board pinout
- verify wiring, polarity, and power requirements
- avoid pins with boot, flash, or USB conflicts

## Where To Start

If you are new to ESP32 and Apple Shortcuts, start here:

1. Flash `esp32_gpio_api_all/` on a standard ESP32 board if you have one.
2. Test `/health` and `/pins` in a browser.
3. Create one `Get Contents of URL` shortcut for `/gpio/23/on`.
4. Duplicate it for `/gpio/23/off`.
5. Expand from there into LEDs, relays, and simple automation flows.

That path shows the core idea of the repo quickly: Apple Shortcuts can be a clean, surprisingly capable front end for local ESP32 and IoT control.
