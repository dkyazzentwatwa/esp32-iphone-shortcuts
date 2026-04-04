#include <Arduino.h>
#line 1 "/Users/cypher/Documents/GitHub/esp32-iphone-shortcuts/esp32_gpio_api_s3/esp32_gpio_api_s3.ino"
/*
  ESP32-S3 GPIO HTTP API for Apple Shortcuts

  Wi-Fi credentials:
  - Paste your home Wi-Fi name into `ssid`
  - Paste your Wi-Fi password into `password`

  Uploading the sketch:
  - In Arduino IDE, install the ESP32 board package if needed
  - Open this sketch folder, select an ESP32-S3 Dev Module / ESP32-S3 DevKitC-compatible board,
    choose the correct serial port, then upload

  Testing in a browser:
  - Open the Serial Monitor after boot to find the assigned local IP address
  - Visit the listed URLs from a device on the same LAN

  Apple Shortcuts:
  - Use the "Get Contents of URL" action
  - Call endpoints like `/gpio/4/on` or `/gpio/4/off`
  - Read the JSON response to confirm the result
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>  // Optional mDNS support for esp32-gpio.local

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

const char* kHostname = "esp32-gpio";
const uint16_t kHttpPort = 80;
const int kDefaultOutputPin = 4;
const unsigned long kWifiRetryDelayMs = 500;

const int kSafeOutputPins[] = {4, 5, 6, 7, 15, 16, 17, 18, 21, 35, 36, 37, 38, 39, 40, 41, 42};
const size_t kSafeOutputPinCount = sizeof(kSafeOutputPins) / sizeof(kSafeOutputPins[0]);

WebServer server(kHttpPort);

String localIpString() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }

  return "0.0.0.0";
}

String escapeJson(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];

    if (c == '\\' || c == '"') {
      escaped += '\\';
    }

    escaped += c;
  }

  return escaped;
}

bool isSafeOutputPin(int pin) {
  for (size_t i = 0; i < kSafeOutputPinCount; ++i) {
    if (kSafeOutputPins[i] == pin) {
      return true;
    }
  }

  return false;
}

void sendJson(bool success, int pin, const char* state, int httpStatus = 200, const char* message = nullptr) {
  String json = "{";
  json += "\"success\":";
  json += success ? "true" : "false";
  json += ",\"pin\":";
  json += String(pin);
  json += ",\"state\":\"";
  json += state;
  json += "\",\"ip\":\"";
  json += localIpString();
  json += "\"";

  if (message != nullptr) {
    json += ",\"message\":\"";
    json += escapeJson(String(message));
    json += "\"";
  }

  json += "}";

  server.send(httpStatus, "application/json", json);
}

void setPinState(int pin, uint8_t level) {
  if (!isSafeOutputPin(pin)) {
    sendJson(false, pin, "unknown", 400, "Pin is not in the safe output allowlist.");
    return;
  }

  digitalWrite(pin, level);
  sendJson(true, pin, level == HIGH ? "on" : "off");
}

void sendPinStatus(int pin) {
  if (!isSafeOutputPin(pin)) {
    sendJson(false, pin, "unknown", 400, "Pin is not in the safe output allowlist.");
    return;
  }

  int pinState = digitalRead(pin);
  sendJson(true, pin, pinState == HIGH ? "on" : "off");
}

void registerPinRoutes(int pin) {
  String basePath = "/gpio/" + String(pin);

  server.on((basePath + "/on").c_str(), HTTP_GET, [pin]() {
    setPinState(pin, HIGH);
  });

  server.on((basePath + "/off").c_str(), HTTP_GET, [pin]() {
    setPinState(pin, LOW);
  });

  server.on((basePath + "/status").c_str(), HTTP_GET, [pin]() {
    sendPinStatus(pin);
  });
}

void handleRoot() {
  String message;
  message += "ESP32-S3 GPIO HTTP API\n";
  message += "Available endpoints:\n";
  message += "GET /\n";
  message += "GET /health\n";
  message += "GET /gpio/4/on\n";
  message += "GET /gpio/4/off\n";
  message += "GET /gpio/4/status\n";
  message += "\n";
  message += "Use Apple Shortcuts with 'Get Contents of URL'.\n";

  server.send(200, "text/plain", message);
}

void handleHealth() {
  server.send(200, "text/plain", "OK");
}

void handleNotFound() {
  String json = "{";
  json += "\"success\":false";
  json += ",\"path\":\"";
  json += escapeJson(server.uri());
  json += "\"";
  json += ",\"message\":\"Route not found.\"";
  json += ",\"ip\":\"";
  json += localIpString();
  json += "\"";
  json += "}";

  server.send(404, "application/json", json);
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(kWifiRetryDelayMs);
    ++attempt;
    Serial.print(".");

    if (attempt % 10 == 0) {
      Serial.print(" retry ");
      Serial.println(attempt);
    }
  }

  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Local IP: ");
  Serial.println(localIpString());
}

void startMdns() {
  Serial.print("Starting mDNS at http://");
  Serial.print(kHostname);
  Serial.println(".local");

  if (MDNS.begin(kHostname)) {
    Serial.println("mDNS started successfully.");
    return;
  }

  Serial.println("mDNS failed to start. Continuing with IP address only.");
}

void printUsageExamples() {
  String ip = localIpString();

  Serial.println();
  Serial.println("HTTP API is ready.");
  Serial.print("Server port: ");
  Serial.println(kHttpPort);
  Serial.println("Usage examples:");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/health");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/gpio/4/on");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/gpio/4/off");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/gpio/4/status");
  Serial.print("  http://");
  Serial.print(kHostname);
  Serial.println(".local/gpio/4/on");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 GPIO HTTP API starting...");
  Serial.println("Update the ssid and password constants before uploading.");

  pinMode(kDefaultOutputPin, OUTPUT);
  digitalWrite(kDefaultOutputPin, LOW);
  Serial.print("Configured GPIO ");
  Serial.print(kDefaultOutputPin);
  Serial.println(" as OUTPUT and set it LOW.");

  connectToWifi();
  startMdns();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/health", HTTP_GET, handleHealth);
  registerPinRoutes(kDefaultOutputPin);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("HTTP server started on port ");
  Serial.println(kHttpPort);

  printUsageExamples();
}

void loop() {
  server.handleClient();
}

/*
  How to use

  Browser or Shortcut examples:
  - http://192.168.1.50/gpio/4/on
  - http://192.168.1.50/gpio/4/off
  - http://192.168.1.50/gpio/4/status
  - http://esp32-gpio.local/gpio/4/on   (if mDNS works on your network)

  Notes:
  - Replace 192.168.1.50 with the IP shown in Serial Monitor.
  - In Apple Shortcuts, use "Get Contents of URL" with GET requests.
  - Some ESP32-S3 pins are input-only, connected to flash, USB, or boot logic.
    Double-check pin safety before exposing additional GPIO routes.
*/

