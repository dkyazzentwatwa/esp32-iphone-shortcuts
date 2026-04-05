/*
  ESP32 GPIO HTTP API for Apple Shortcuts

  Wi-Fi credentials:
  - Paste your home Wi-Fi name into `ssid`
  - Paste your Wi-Fi password into `password`

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
  - Call endpoints like `/gpio/23/on`, `/system`, or `/wifi/scan`
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
const uint32_t kBleScanDurationMs = 3000;

const int kSafeOutputPins[] = {kDefaultOutputPin};
const size_t kSafeOutputPinCount = sizeof(kSafeOutputPins) / sizeof(kSafeOutputPins[0]);

WebServer server(kHttpPort);
bool bleInitialized = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 GPIO HTTP API starting...");
  Serial.println("Update the ssid and password constants before uploading.");

  initializeSafePins(kSafeOutputPins, kSafeOutputPinCount);
  Serial.print("Configured GPIO ");
  Serial.print(kDefaultOutputPin);
  Serial.println(" as OUTPUT and set it LOW.");

  bool wifiConnected = connectToWifi(kHostname, ssid, password, kWifiRetryDelayMs, true, kWifiConnectTimeoutMs);
  if (!wifiConnected) {
    return;
  }

  startMdns(kHostname);

  registerShortcutRoutes(
      server,
      kBoardLabel,
      kHostname,
      kSafeOutputPins,
      kSafeOutputPinCount,
      kDefaultOutputPin,
      false,
      bleInitialized,
      kBleScanDurationMs);

  server.begin();
  Serial.print("HTTP server started on port ");
  Serial.println(kHttpPort);

  printUsageExamples(kHostname, kSafeOutputPins, kSafeOutputPinCount, kDefaultOutputPin, false);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
}

/*
  How to use

  Browser or Shortcut examples:
  - http://192.168.1.50/gpio/23/on
  - http://192.168.1.50/gpio/23/off
  - http://192.168.1.50/gpio/23/status
  - http://192.168.1.50/system
  - http://esp32-gpio.local/wifi/scan

  Notes:
  - Replace 192.168.1.50 with the IP shown in Serial Monitor.
  - In Apple Shortcuts, use "Get Contents of URL" with GET requests.
  - BLE scanning requires the Huge APP partition scheme.
*/
