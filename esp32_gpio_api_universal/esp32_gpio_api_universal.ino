/*
  ESP32 GPIO HTTP API - Universal Version

  This sketch auto-detects supported ESP32 families at runtime and selects a
  conservative safe-pin allowlist for the detected chip.

  Wi-Fi credentials:
  - Copy `secrets.example.h` to `secrets.h` at the repo root
  - Fill in your Wi-Fi name and password there

  Uploading the sketch:
  - Install the ESP32 Arduino board package
  - Select any supported ESP32 board family in Arduino IDE
  - Use the Huge APP partition scheme so the discovery features fit

  Apple Shortcuts:
  - Use "Get Contents of URL"
  - Call endpoints like `/gpio/23/on`, `/system`, `/wifi/scan`, or `/info`
*/

#include "../secrets.h"
#include "../esp32_shortcut_api.h"
#include <esp_chip_info.h>

const char* kBoardLabel = "ESP32 GPIO HTTP API - Universal";
const char* kHostname = "esp32-gpio";
const uint16_t kHttpPort = 80;
const unsigned long kWifiRetryDelayMs = 500;
const unsigned long kWifiConnectTimeoutMs = 30000;
const uint32_t kBleScanDurationMs = 3000;

enum ChipModel {
  UNKNOWN,
  ESP32_CLASSIC,
  ESP32_S3,
  ESP32_C3,
  ESP32_S2
};

struct DeviceConfig {
  ChipModel detectedChip;
  const char* chipName;
  const int* allowedPins;
  size_t pinCount;
  bool detectionSuccessful;
};

const int kClassicSafePins[] = {18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
const size_t kClassicSafePinCount = sizeof(kClassicSafePins) / sizeof(kClassicSafePins[0]);

const int kS3SafePins[] = {4, 5, 6, 7, 15, 16, 17, 18, 21, 35, 36, 37, 38, 39, 40, 41, 42};
const size_t kS3SafePinCount = sizeof(kS3SafePins) / sizeof(kS3SafePins[0]);

const int kC3SafePins[] = {0, 1, 2, 3, 4, 5, 6, 7, 10, 20, 21};
const size_t kC3SafePinCount = sizeof(kC3SafePins) / sizeof(kC3SafePins[0]);

const int kFallbackSafePins[] = {18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
const size_t kFallbackSafePinCount = sizeof(kFallbackSafePins) / sizeof(kFallbackSafePins[0]);

DeviceConfig gConfig;
WebServer server(kHttpPort);
bool bleInitialized = false;

const char* chipModelName(ChipModel model) {
  switch (model) {
    case ESP32_CLASSIC:
      return "ESP32";
    case ESP32_S3:
      return "ESP32-S3";
    case ESP32_C3:
      return "ESP32-C3";
    case ESP32_S2:
      return "ESP32-S2";
    default:
      return "Unknown";
  }
}

ChipModel detectChipModel() {
  esp_chip_info_t chipInfo;
  esp_chip_info(&chipInfo);

  switch (chipInfo.model) {
    case CHIP_ESP32:
      return ESP32_CLASSIC;
    case CHIP_ESP32S3:
      return ESP32_S3;
    case CHIP_ESP32C3:
      return ESP32_C3;
    case CHIP_ESP32S2:
      return ESP32_S2;
    default:
      return UNKNOWN;
  }
}

void loadConfig() {
  Serial.println("Detecting chip model...");

  gConfig.detectedChip = detectChipModel();
  gConfig.detectionSuccessful = gConfig.detectedChip != UNKNOWN;
  gConfig.chipName = chipModelName(gConfig.detectedChip);

  switch (gConfig.detectedChip) {
    case ESP32_CLASSIC:
      gConfig.allowedPins = kClassicSafePins;
      gConfig.pinCount = kClassicSafePinCount;
      break;
    case ESP32_S3:
      gConfig.allowedPins = kS3SafePins;
      gConfig.pinCount = kS3SafePinCount;
      break;
    case ESP32_C3:
      gConfig.allowedPins = kC3SafePins;
      gConfig.pinCount = kC3SafePinCount;
      break;
    case ESP32_S2:
      gConfig.allowedPins = kFallbackSafePins;
      gConfig.pinCount = kFallbackSafePinCount;
      Serial.println("ESP32-S2 detected. Using conservative fallback pins.");
      break;
    default:
      gConfig.allowedPins = kFallbackSafePins;
      gConfig.pinCount = kFallbackSafePinCount;
      Serial.println("Chip detection failed. Using fallback pins.");
      break;
  }

  Serial.print("Detected chip: ");
  Serial.println(gConfig.chipName);
  Serial.print("Safe pins: ");
  for (size_t i = 0; i < gConfig.pinCount; ++i) {
    if (i > 0) {
      Serial.print(", ");
    }
    Serial.print(gConfig.allowedPins[i]);
  }
  Serial.println();
}

void sendInfoJson() {
  String json = "{";
  json += "\"success\":true";
  json += ",\"ip\":\"";
  json += localIpString();
  json += "\",\"chip_model\":\"";
  json += chipModelName(gConfig.detectedChip);
  json += "\",\"detection_ok\":";
  json += jsonBool(gConfig.detectionSuccessful);
  json += ",\"allowed_pins\":[";

  for (size_t i = 0; i < gConfig.pinCount; ++i) {
    if (i > 0) {
      json += ",";
    }

    json += String(gConfig.allowedPins[i]);
  }

  json += "],\"pin_count\":";
  json += String(gConfig.pinCount);
  json += "}";

  sendJsonResponse(server, json);
}

void handleRoot() {
  String message;
  message += kBoardLabel;
  message += "\nDetected chip: ";
  message += gConfig.chipName;
  message += "\nAvailable endpoints:\n";
  message += "GET /\n";
  message += "GET /health\n";
  message += "GET /info\n";
  message += "GET /system\n";
  message += "GET /wifi/status\n";
  message += "GET /wifi/scan\n";
  message += "GET /ble/scan\n";
  message += "GET /pins\n";
  message += "GET /gpio/<pin>/on\n";
  message += "GET /gpio/<pin>/off\n";
  message += "GET /gpio/<pin>/status\n";
  message += "\nSafe pins: ";

  for (size_t i = 0; i < gConfig.pinCount; ++i) {
    if (i > 0) {
      message += ", ";
    }

    message += String(gConfig.allowedPins[i]);
  }

  message += "\nExample: /gpio/";
  message += String(gConfig.pinCount > 0 ? gConfig.allowedPins[0] : 23);
  message += "/on\n";
  message += "Use Apple Shortcuts with 'Get Contents of URL'.\n";
  message += "mDNS: http://";
  message += kHostname;
  message += ".local\n";

  server.send(200, "text/plain", message);
}

void printUsageExamples() {
  String ip = localIpString();

  Serial.println();
  Serial.println("HTTP API is ready.");
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/info");
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
  Serial.print("  http://");
  Serial.print(ip);
  Serial.println("/pins");

  if (gConfig.pinCount > 0) {
    Serial.print("  http://");
    Serial.print(ip);
    Serial.print("/gpio/");
    Serial.print(gConfig.allowedPins[0]);
    Serial.println("/on");
  }

  Serial.print("  http://");
  Serial.print(kHostname);
  Serial.println(".local/gpio/<pin>/status");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 GPIO HTTP API - Universal starting...");
  Serial.println("Update the ssid and password constants before uploading.");

  loadConfig();
  initializeSafePins(gConfig.allowedPins, gConfig.pinCount);
  Serial.println("Configured safe GPIO pins as OUTPUT and set them LOW.");

  bool wifiConnected = connectToWifi(kHostname, ssid, password, kWifiRetryDelayMs, true, kWifiConnectTimeoutMs);
  if (!wifiConnected) {
    return;
  }

  startMdns(kHostname);

  server.on("/", HTTP_GET, []() {
    handleRoot();
  });
  server.on("/health", HTTP_GET, []() {
    handleHealth(server);
  });
  server.on("/info", HTTP_GET, []() {
    sendInfoJson();
  });
  server.on("/pins", HTTP_GET, [&, allowedPins = gConfig.allowedPins, pinCount = gConfig.pinCount]() {
    sendPinsJson(server, allowedPins, pinCount);
  });
  server.on("/system", HTTP_GET, [&, hostname = kHostname]() {
    sendSystemJson(server, hostname);
  });
  server.on("/wifi/status", HTTP_GET, [&, hostname = kHostname]() {
    sendWifiStatusJson(server, hostname);
  });
  server.on("/wifi/scan", HTTP_GET, []() {
    sendWifiScanJson(server);
  });
  server.on("/ble/scan", HTTP_GET, [&, bleScanDurationMs = kBleScanDurationMs]() {
    sendBleScanJson(server, bleInitialized, bleScanDurationMs);
  });
  registerSafePinRoutes(server, gConfig.allowedPins, gConfig.pinCount);
  server.onNotFound([&]() {
    handleNotFound(server);
  });

  server.begin();
  Serial.print("HTTP server started on port ");
  Serial.println(kHttpPort);
  Serial.print("Default example pin: ");
  Serial.println(gConfig.pinCount > 0 ? gConfig.allowedPins[0] : 23);

  printUsageExamples();
}

void loop() {
  server.handleClient();
}
