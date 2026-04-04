/*
  ESP32-C3 Super Mini GPIO HTTP API for Apple Shortcuts

  Wi-Fi credentials:
  - Paste your home Wi-Fi name into `ssid`
  - Paste your Wi-Fi password into `password`

  Uploading the sketch:
  - In Arduino IDE, install the ESP32 board package if needed
  - Open this sketch folder, select an ESP32-C3 Super Mini compatible board,
    choose the correct serial port, then upload

  Testing in a browser:
  - Open the Serial Monitor after boot to find the assigned local IP address
  - Visit the listed URLs from a device on the same LAN

  Apple Shortcuts:
  - Use the "Get Contents of URL" action
  - Call endpoints like `/gpio/4/on`, `/gpio/21/off`, or `/pins`
  - Read the JSON response to confirm the result
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>  // Optional mDNS support for esp32-gpio.local

const char* ssid = "thanos lives forever";
const char* password = "ntwatwa1990";

const char* kHostname = "esp32-gpio";
const uint16_t kHttpPort = 80;
const int kDefaultOutputPin = 4;
const unsigned long kWifiRetryDelayMs = 500;

// Conservative "safe" pins for common ESP32-C3 Super Mini boards.
// GPIO8 is commonly the built-in LED, and GPIO9 is commonly the boot button.
const int kSafeOutputPins[] = {0, 1, 2, 3, 4, 5, 6, 7, 10, 20, 21};
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

String pinStateName(int pin) {
  return digitalRead(pin) == HIGH ? "on" : "off";
}

bool isSafeOutputPin(int pin) {
  for (size_t i = 0; i < kSafeOutputPinCount; ++i) {
    if (kSafeOutputPins[i] == pin) {
      return true;
    }
  }

  return false;
}

void sendPinJson(bool success, int pin, const char* state, int httpStatus = 200, const char* message = nullptr) {
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

void sendPinsJson() {
  String json = "{";
  json += "\"success\":true";
  json += ",\"ip\":\"";
  json += localIpString();
  json += "\",\"pins\":[";

  for (size_t i = 0; i < kSafeOutputPinCount; ++i) {
    if (i > 0) {
      json += ",";
    }

    int pin = kSafeOutputPins[i];
    json += "{";
    json += "\"pin\":";
    json += String(pin);
    json += ",\"state\":\"";
    json += pinStateName(pin);
    json += "\"";
    json += "}";
  }

  json += "]}";
  server.send(200, "application/json", json);
}

void setPinState(int pin, uint8_t level) {
  if (!isSafeOutputPin(pin)) {
    sendPinJson(false, pin, "unknown", 400, "Pin is not in the safe output allowlist.");
    return;
  }

  digitalWrite(pin, level);
  sendPinJson(true, pin, level == HIGH ? "on" : "off");
}

void sendPinStatus(int pin) {
  if (!isSafeOutputPin(pin)) {
    sendPinJson(false, pin, "unknown", 400, "Pin is not in the safe output allowlist.");
    return;
  }

  String state = pinStateName(pin);
  sendPinJson(true, pin, state.c_str());
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
  message += "ESP32-C3 Super Mini GPIO HTTP API\n";
  message += "Available endpoints:\n";
  message += "GET /\n";
  message += "GET /health\n";
  message += "GET /pins\n";
  message += "GET /gpio/<pin>/on\n";
  message += "GET /gpio/<pin>/off\n";
  message += "GET /gpio/<pin>/status\n";
  message += "\n";
  message += "Safe pins: 0, 1, 2, 3, 4, 5, 6, 7, 10, 20, 21\n";
  message += "Example: /gpio/4/on\n";
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

void initializeSafePins() {
  for (size_t i = 0; i < kSafeOutputPinCount; ++i) {
    int pin = kSafeOutputPins[i];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void registerSafePinRoutes() {
  for (size_t i = 0; i < kSafeOutputPinCount; ++i) {
    registerPinRoutes(kSafeOutputPins[i]);
  }
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
  Serial.println("/pins");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/gpio/0/on");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/gpio/4/off");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/gpio/21/status");
  Serial.print("  http://");
  Serial.print(kHostname);
  Serial.println(".local/gpio/4/on");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-C3 Super Mini GPIO HTTP API starting...");
  Serial.println("Update the ssid and password constants before uploading.");

  initializeSafePins();
  Serial.println("Configured all safe GPIO pins as OUTPUT and set them LOW.");

  connectToWifi();
  startMdns();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/health", HTTP_GET, handleHealth);
  server.on("/pins", HTTP_GET, sendPinsJson);
  registerSafePinRoutes();
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.print("HTTP server started on port ");
  Serial.println(kHttpPort);
  Serial.print("Default example pin: ");
  Serial.println(kDefaultOutputPin);

  printUsageExamples();
}

void loop() {
  server.handleClient();
}

/*
  How to use

  Browser or Shortcut examples:
  - http://192.168.1.50/pins
  - http://192.168.1.50/gpio/0/on
  - http://192.168.1.50/gpio/4/off
  - http://192.168.1.50/gpio/21/status
  - http://esp32-gpio.local/gpio/4/on   (if mDNS works on your network)

  Notes:
  - Replace 192.168.1.50 with the IP shown in Serial Monitor.
  - In Apple Shortcuts, use "Get Contents of URL" with GET requests.
  - GPIO8 is commonly the built-in LED and GPIO9 is commonly the boot button.
    This sketch leaves both out of the default safe allowlist.
*/
