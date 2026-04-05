#pragma once

#include <BLEDevice.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

static const char* wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
    default:
      return "WL_UNKNOWN_STATUS";
  }
}

static const char* wifiEncryptionTypeName(wifi_auth_mode_t type) {
  switch (type) {
    case WIFI_AUTH_OPEN:
      return "open";
    case WIFI_AUTH_WEP:
      return "wep";
    case WIFI_AUTH_WPA_PSK:
      return "wpa_psk";
    case WIFI_AUTH_WPA2_PSK:
      return "wpa2_psk";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "wpa_wpa2_psk";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "wpa2_enterprise";
    case WIFI_AUTH_WPA3_PSK:
      return "wpa3_psk";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "wpa2_wpa3_psk";
    case WIFI_AUTH_WAPI_PSK:
      return "wapi_psk";
    default:
      return "unknown";
  }
}

static String localIpString() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }

  return "0.0.0.0";
}

static String escapeJson(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];

    if (c == '\\') { escaped += '\\'; escaped += '\\'; continue; }
    if (c == '"')  { escaped += '\\'; escaped += '"';  continue; }
    if (c == '\n') { escaped += '\\'; escaped += 'n';  continue; }
    if (c == '\r') { escaped += '\\'; escaped += 'r';  continue; }
    if (c == '\t') { escaped += '\\'; escaped += 't';  continue; }

    escaped += c;
  }

  return escaped;
}

static String jsonBool(bool value) {
  return value ? "true" : "false";
}

static void sendJsonResponse(WebServer& server, const String& json, int httpStatus = 200) {
  server.send(httpStatus, "application/json", json);
}

static bool isSafeOutputPin(const int* pins, size_t pinCount, int pin) {
  for (size_t i = 0; i < pinCount; ++i) {
    if (pins[i] == pin) {
      return true;
    }
  }

  return false;
}

static void sendPinJson(WebServer& server, bool success, int pin, const char* state, int httpStatus = 200, const char* message = nullptr) {
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
  sendJsonResponse(server, json, httpStatus);
}

static void sendPinsJson(WebServer& server, const int* pins, size_t pinCount) {
  String json = "{";
  json += "\"success\":true";
  json += ",\"ip\":\"";
  json += localIpString();
  json += "\",\"pins\":[";

  for (size_t i = 0; i < pinCount; ++i) {
    if (i > 0) {
      json += ",";
    }

    int pin = pins[i];
    json += "{";
    json += "\"pin\":";
    json += String(pin);
    json += ",\"state\":\"";
    json += digitalRead(pin) == HIGH ? "on" : "off";
    json += "\"";
    json += "}";
  }

  json += "]}";
  sendJsonResponse(server, json);
}

static void sendWifiStatusJson(WebServer& server, const char* hostname) {
  bool connected = WiFi.status() == WL_CONNECTED;
  String json = "{";
  json += "\"success\":true";
  json += ",\"connected\":";
  json += jsonBool(connected);
  json += ",\"ssid\":\"";
  json += escapeJson(connected ? WiFi.SSID() : "");
  json += "\",\"ip\":\"";
  json += localIpString();
  json += "\",\"hostname\":\"";
  json += hostname;
  json += "\",\"mac\":\"";
  json += WiFi.macAddress();
  json += "\"";

  if (connected) {
    json += ",\"rssi\":";
    json += String(WiFi.RSSI());
    json += ",\"channel\":";
    json += String(WiFi.channel());
  }

  json += "}";
  sendJsonResponse(server, json);
}

static void sendWifiScanJson(WebServer& server) {
  int networkCount = WiFi.scanNetworks();
  String json = "{";
  json += "\"success\":true";
  json += ",\"count\":";
  json += String(networkCount < 0 ? 0 : networkCount);
  json += ",\"ip\":\"";
  json += localIpString();
  json += "\",\"networks\":[";

  if (networkCount > 0) {
    for (int i = 0; i < networkCount; ++i) {
      if (i > 0) {
        json += ",";
      }

      json += "{";
      json += "\"ssid\":\"";
      json += escapeJson(WiFi.SSID(i));
      json += "\",\"rssi\":";
      json += String(WiFi.RSSI(i));
      json += ",\"channel\":";
      json += String(WiFi.channel(i));
      json += ",\"encryption\":\"";
      json += wifiEncryptionTypeName(WiFi.encryptionType(i));
      json += "\",\"open\":";
      json += jsonBool(WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
      json += "}";
    }
  }

  json += "]}";
  WiFi.scanDelete();
  sendJsonResponse(server, json);
}

static void sendSystemJson(WebServer& server, const char* hostname) {
  String json = "{";
  json += "\"success\":true";
  json += ",\"ip\":\"";
  json += localIpString();
  json += "\",\"hostname\":\"";
  json += hostname;
  json += "\",\"uptime_ms\":";
  json += String(millis());
  json += ",\"free_heap\":";
  json += String(ESP.getFreeHeap());
  json += ",\"min_free_heap\":";
  json += String(ESP.getMinFreeHeap());
  json += ",\"chip_model\":\"";
  json += ESP.getChipModel();
  json += "\",\"chip_revision\":";
  json += String(ESP.getChipRevision());
  json += ",\"cpu_mhz\":";
  json += String(ESP.getCpuFreqMHz());
  json += ",\"flash_size\":";
  json += String(ESP.getFlashChipSize());
  json += ",\"sdk_version\":\"";
  json += ESP.getSdkVersion();
  json += "\"";
  json += "}";
  sendJsonResponse(server, json);
}

static bool ensureBleInitialized(bool& bleInitialized) {
  if (bleInitialized) {
    return true;
  }

  BLEDevice::init("");
  bleInitialized = true;
  return true;
}

static void sendBleScanJson(WebServer& server, bool& bleInitialized, unsigned long scanDurationMs) {
  if (!ensureBleInitialized(bleInitialized)) {
    String json = "{";
    json += "\"success\":false";
    json += ",\"supported\":false";
    json += ",\"message\":\"BLE initialization failed.\"";
    json += ",\"ip\":\"";
    json += localIpString();
    json += "\"";
    json += "}";
    sendJsonResponse(server, json, 500);
    return;
  }

  BLEScan* bleScan = BLEDevice::getScan();
  bleScan->setActiveScan(true);

  BLEScanResults* scanResults = bleScan->start(scanDurationMs / 1000, false);
  int count = scanResults == nullptr ? 0 : scanResults->getCount();

  String json = "{";
  json += "\"success\":true";
  json += ",\"supported\":true";
  json += ",\"duration_ms\":";
  json += String(scanDurationMs);
  json += ",\"count\":";
  json += String(count);
  json += ",\"devices\":[";

  for (int i = 0; i < count; ++i) {
    if (i > 0) {
      json += ",";
    }

    BLEAdvertisedDevice device = scanResults->getDevice(i);
    json += "{";
    json += "\"name\":\"";
    json += escapeJson(device.haveName() ? String(device.getName().c_str()) : "");
    json += "\",\"address\":\"";
    json += String(device.getAddress().toString().c_str());
    json += "\",\"rssi\":";
    json += String(device.getRSSI());

    if (device.haveTXPower()) {
      json += ",\"tx_power\":";
      json += String(device.getTXPower());
    }

    if (device.haveServiceUUID()) {
      json += ",\"service_uuid\":\"";
      json += String(device.getServiceUUID().toString().c_str());
      json += "\"";
    }

    json += "}";
  }

  json += "]}";
  bleScan->clearResults();
  sendJsonResponse(server, json);
}

static void setPinState(WebServer& server, const int* pins, size_t pinCount, int pin, uint8_t level) {
  if (!isSafeOutputPin(pins, pinCount, pin)) {
    sendPinJson(server, false, pin, "unknown", 400, "Pin is not in the safe output allowlist.");
    return;
  }

  digitalWrite(pin, level);
  sendPinJson(server, true, pin, level == HIGH ? "on" : "off");
}

static void sendPinStatus(WebServer& server, const int* pins, size_t pinCount, int pin) {
  if (!isSafeOutputPin(pins, pinCount, pin)) {
    sendPinJson(server, false, pin, "unknown", 400, "Pin is not in the safe output allowlist.");
    return;
  }

  sendPinJson(server, true, pin, digitalRead(pin) == HIGH ? "on" : "off");
}

static void registerPinRoutes(WebServer& server, const int* pins, size_t pinCount, int pin) {
  String basePath = "/gpio/" + String(pin);

  server.on((basePath + "/on").c_str(), HTTP_GET, [&, pin]() {
    setPinState(server, pins, pinCount, pin, HIGH);
  });

  server.on((basePath + "/off").c_str(), HTTP_GET, [&, pin]() {
    setPinState(server, pins, pinCount, pin, LOW);
  });

  server.on((basePath + "/status").c_str(), HTTP_GET, [&, pin]() {
    sendPinStatus(server, pins, pinCount, pin);
  });
}

static void initializeSafePins(const int* pins, size_t pinCount) {
  for (size_t i = 0; i < pinCount; ++i) {
    int pin = pins[i];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

static void registerSafePinRoutes(WebServer& server, const int* pins, size_t pinCount) {
  for (size_t i = 0; i < pinCount; ++i) {
    registerPinRoutes(server, pins, pinCount, pins[i]);
  }
}

static void handleRoot(
    WebServer& server,
    const char* boardLabel,
    const char* hostname,
    const int* safePins,
    size_t safePinCount,
    int defaultOutputPin,
    bool includePinsEndpoint) {
  String message;
  message += boardLabel;
  message += "\nAvailable endpoints:\n";
  message += "GET /\n";
  message += "GET /health\n";
  message += "GET /system\n";
  message += "GET /wifi/status\n";
  message += "GET /wifi/scan\n";
  message += "GET /ble/scan\n";

  if (includePinsEndpoint) {
    message += "GET /pins\n";
  }

  message += "GET /gpio/<pin>/on\n";
  message += "GET /gpio/<pin>/off\n";
  message += "GET /gpio/<pin>/status\n";
  message += "\nSafe pins: ";

  for (size_t i = 0; i < safePinCount; ++i) {
    if (i > 0) {
      message += ", ";
    }

    message += String(safePins[i]);
  }

  message += "\nExample: /gpio/";
  message += String(defaultOutputPin);
  message += "/on\n";
  message += "Use Apple Shortcuts with 'Get Contents of URL'.\n";
  message += "mDNS: http://";
  message += hostname;
  message += ".local\n";

  server.send(200, "text/plain", message);
}

static void handleHealth(WebServer& server) {
  server.send(200, "text/plain", "OK");
}

static void handleNotFound(WebServer& server) {
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

  sendJsonResponse(server, json, 404);
}

static bool connectToWifi(
    const char* hostname,
    const char* ssid,
    const char* password,
    unsigned long retryDelayMs,
    bool useTimeout = false,
    unsigned long connectTimeoutMs = 30000) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi SSID: ");
  Serial.println(ssid);
  Serial.print("Hostname: ");
  Serial.println(hostname);
  Serial.print("Wi-Fi MAC: ");
  Serial.println(WiFi.macAddress());

  int attempt = 0;
  wl_status_t lastStatus = static_cast<wl_status_t>(255);
  unsigned long startTime = millis();

  while (true) {
    wl_status_t currentStatus = WiFi.status();

    if (currentStatus != lastStatus) {
      Serial.print("Wi-Fi status changed to ");
      Serial.print(wifiStatusName(currentStatus));
      Serial.print(" (");
      Serial.print(static_cast<int>(currentStatus));
      Serial.println(")");
      lastStatus = currentStatus;
    }

    if (currentStatus == WL_CONNECTED) {
      Serial.println("Wi-Fi connected.");
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Local IP: ");
      Serial.println(localIpString());
      return true;
    }

    if (useTimeout && (millis() - startTime >= connectTimeoutMs)) {
      wl_status_t finalStatus = WiFi.status();
      Serial.println("Wi-Fi connection timed out.");
      Serial.print("Final Wi-Fi status: ");
      Serial.print(wifiStatusName(finalStatus));
      Serial.print(" (");
      Serial.print(static_cast<int>(finalStatus));
      Serial.println(")");
      Serial.println("HTTP server will not start until Wi-Fi connects.");
      return false;
    }

    delay(retryDelayMs);
    ++attempt;

    unsigned long elapsedSeconds = (millis() - startTime) / 1000;
    Serial.print("Wi-Fi retry ");
    Serial.print(attempt);
    Serial.print(" after ");
    Serial.print(elapsedSeconds);
    Serial.print("s, status=");
    Serial.print(wifiStatusName(WiFi.status()));
    Serial.print(" (");
    Serial.print(static_cast<int>(WiFi.status()));
    Serial.println(")");
  }
}

static void startMdns(const char* hostname) {
  Serial.print("Starting mDNS at http://");
  Serial.print(hostname);
  Serial.println(".local");

  if (MDNS.begin(hostname)) {
    Serial.println("mDNS started successfully.");
    return;
  }

  Serial.println("mDNS failed to start. Continuing with IP address only.");
}

static void printUsageExamples(
    const char* hostname,
    const int* safePins,
    size_t safePinCount,
    int defaultOutputPin,
    bool includePinsEndpoint) {
  String ip = localIpString();

  Serial.println();
  Serial.println("HTTP API is ready.");
  Serial.println("Usage examples:");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/health");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/system");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/wifi/status");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/wifi/scan");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/ble/scan");

  if (includePinsEndpoint) {
    Serial.print("  http://");
    Serial.print(ip);
    Serial.println("/pins");
  }

  Serial.print("  http://");
  Serial.print(ip);
  Serial.print("/gpio/");
  Serial.print(defaultOutputPin);
  Serial.println("/on");

  if (safePinCount > 1) {
    Serial.print("  http://");
    Serial.print(ip);
    Serial.print("/gpio/");
    Serial.print(safePins[1]);
    Serial.println("/off");
  }

  Serial.print("  http://");
  Serial.print(hostname);
  Serial.println(".local/gpio/<pin>/status");
  Serial.println();
}

static void registerShortcutRoutes(
    WebServer& server,
    const char* boardLabel,
    const char* hostname,
    const int* safePins,
    size_t safePinCount,
    int defaultOutputPin,
    bool includePinsEndpoint,
    bool& bleInitialized,
    unsigned long bleScanDurationMs) {
  server.on("/", HTTP_GET, [&, boardLabel, hostname, safePins, safePinCount, defaultOutputPin, includePinsEndpoint]() {
    handleRoot(server, boardLabel, hostname, safePins, safePinCount, defaultOutputPin, includePinsEndpoint);
  });

  server.on("/health", HTTP_GET, [&]() {
    handleHealth(server);
  });

  if (includePinsEndpoint) {
    server.on("/pins", HTTP_GET, [&, safePins, safePinCount]() {
      sendPinsJson(server, safePins, safePinCount);
    });
  }

  server.on("/system", HTTP_GET, [&, hostname]() {
    sendSystemJson(server, hostname);
  });

  server.on("/wifi/status", HTTP_GET, [&, hostname]() {
    sendWifiStatusJson(server, hostname);
  });

  server.on("/wifi/scan", HTTP_GET, [&]() {
    sendWifiScanJson(server);
  });

  server.on("/ble/scan", HTTP_GET, [&, bleScanDurationMs]() {
    sendBleScanJson(server, bleInitialized, bleScanDurationMs);
  });

  registerSafePinRoutes(server, safePins, safePinCount);

  server.onNotFound([&]() {
    handleNotFound(server);
  });
}
