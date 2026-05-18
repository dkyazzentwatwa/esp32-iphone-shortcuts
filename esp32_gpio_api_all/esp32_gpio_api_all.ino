/*
  ESP32 DevKitC V4 GPIO HTTP API for Apple Shortcuts

  Wi-Fi credentials:
  - Copy `secrets.example.h` to `secrets.h` at the repo root
  - Fill in your Wi-Fi name and password there

  Uploading the sketch:
  - In Arduino IDE, install the ESP32 board package if needed
  - Open this sketch folder, select an ESP32 Dev Module / ESP32 DevKitC V4 compatible board,
    choose the correct serial port, then upload
  - For BLE discovery, use the Huge APP partition scheme so the sketch fits

  Testing in a browser:
  - Open the Serial Monitor after boot to find the assigned local IP address
  - Visit the listed URLs from a device on the same LAN

  Apple Shortcuts:
  - Use the "Get Contents of URL" action
  - Call endpoints like `/gpio/23/on`, `/gpio/18/off`, or `/pins`
  - Read the JSON response to confirm the result
*/

#include "../secrets.h"
#include "../esp32_shortcut_api.h"

const char* kBoardLabel = "ESP32 GPIO HTTP API";
const char* kHostname = "esp32-gpio";
const uint16_t kHttpPort = 80;
const int kDefaultOutputPin = 23;
const unsigned long kWifiRetryDelayMs = 500;
const unsigned long kWifiConnectTimeoutMs = 30000;
const unsigned long kWifiReconnectIntervalMs = 10000;
const uint32_t kBleScanDurationMs = 3000;

const int kSafeOutputPins[] = {12, 13, 14, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
const size_t kSafeOutputPinCount = sizeof(kSafeOutputPins) / sizeof(kSafeOutputPins[0]);

WebServer server(kHttpPort);
bool bleInitialized = false;
bool mdnsStarted = false;
bool wasWifiConnected = false;
unsigned long lastWifiReconnectAttemptMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 GPIO HTTP API starting...");
  Serial.println("Copy secrets.example.h to secrets.h and update your Wi-Fi credentials before uploading.");

  initializeSafePins(kSafeOutputPins, kSafeOutputPinCount);
  Serial.println("Configured all safe GPIO pins as OUTPUT and set them LOW.");

  bool wifiConnected = connectToWifi(kHostname, ssid, password, kWifiRetryDelayMs, true, kWifiConnectTimeoutMs);
  if (wifiConnected) {
    mdnsStarted = startMdns(kHostname);
  } else {
    Serial.println("Continuing without Wi-Fi. Reconnect attempts will run in loop().");
  }
  wasWifiConnected = wifiConnected;

  registerShortcutRoutes(
      server,
      kBoardLabel,
      kHostname,
      kSafeOutputPins,
      kSafeOutputPinCount,
      kDefaultOutputPin,
      true,
      bleInitialized,
      kBleScanDurationMs);

  server.begin();
  Serial.print("HTTP server started on port ");
  Serial.println(kHttpPort);
  Serial.print("Default example pin: ");
  Serial.println(kDefaultOutputPin);

  if (wifiConnected) {
    printUsageExamples(kHostname, kSafeOutputPins, kSafeOutputPinCount, kDefaultOutputPin, true);
  } else {
    Serial.println("HTTP routes are registered and will be reachable after Wi-Fi reconnects.");
  }
}

void loop() {
  bool wasConnectedBefore = wasWifiConnected;
  if (maintainWifiConnection(
          kHostname,
          ssid,
          password,
          kWifiReconnectIntervalMs,
          lastWifiReconnectAttemptMs,
          wasWifiConnected,
          mdnsStarted)) {
    if (!wasConnectedBefore && wasWifiConnected) {
      printUsageExamples(kHostname, kSafeOutputPins, kSafeOutputPinCount, kDefaultOutputPin, true);
    }
    server.handleClient();
  }
}

/*
  How to use

  Browser or Shortcut examples:
  - http://192.168.1.50/pins
  - http://192.168.1.50/system
  - http://192.168.1.50/wifi/status
  - http://192.168.1.50/gpio/23/on
  - http://192.168.1.50/gpio/18/off
  - http://esp32-gpio.local/ble/scan   (if BLE and mDNS both work on your network)

  Notes:
  - Replace 192.168.1.50 with the IP shown in Serial Monitor.
  - In Apple Shortcuts, use "Get Contents of URL" with GET requests.
  - BLE scanning requires the Huge APP partition scheme.
*/
