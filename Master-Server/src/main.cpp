#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include "pageindex.h" // Include the HTML file
#include <esp_wifi.h>

// Network Credentials
const char* wifi_network_ssid = "Man2";  // Wi-Fi network SSID
const char* wifi_network_password = "manman2024"; // Wi-Fi network password

// Access Point Configuration
const char* soft_ap_ssid = "ESP32_WS";  // AP SSID
const char* soft_ap_password = "helloesp32WS"; // AP Password
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ESP-NOW Communication
uint8_t soundSensorSlaveMAC[] = {0xd8, 0xbc, 0x38, 0xfb, 0xa5, 0x7c}; // sound sensor slave MAC
uint8_t motionSensorSlaveMAC[] = {0xc0, 0x5d, 0x89, 0xb1, 0x93, 0xa0}; // motion sensor slave MAC
uint8_t smokeSensorSlaveMAC[] = {0xfc, 0xe8, 0xc0, 0x74, 0x50, 0x14}; // smoke sensor slave MAC
uint8_t lightSensorSlaveMAC[] = {0xa8, 0x42, 0xe3, 0xc8, 0x36, 0x88}; // light sensor slave MAC

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Data Structure for ESP-NOW
typedef struct struct_message {
  int sensorValue;
  char sensorStatus[20];
} struct_message;

typedef struct struct_message_light {
  int lightLevel;
  char brightnessPercentage[10];
} struct_message_light;

struct_message receivedDataSound;  // Data received from sound sensor
struct_message receivedDataMotion; // Data received from motion sensor
struct_message receivedDataSmoke;  // Data received from smoke sensor
struct_message_light receivedDataLight; // Data received from light sensor

// Get Wi-Fi channel for the specified SSID
int32_t get_wifi_channel(const char* ssid) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (String(ssid) == WiFi.SSID(i)) {
      return WiFi.channel(i);
    }
  }
  return 0;
}

// Set Wi-Fi channel for ESP32
void scan_and_set_wifi_channel() {
  Serial.printf("\nScanning for SSID: %s\n", wifi_network_ssid);
  int master_channel = get_wifi_channel(wifi_network_ssid);

  if (master_channel == 0) {
    Serial.println("SSID not found!");
  } else {
    Serial.printf("SSID found. Channel: %d\n", master_channel);
    if (WiFi.channel() != master_channel) {
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_channel(master_channel, WIFI_SECOND_CHAN_NONE);
      esp_wifi_set_promiscuous(false);
    }
  }
  Serial.printf("Current Wi-Fi channel: %d\n", WiFi.channel());
}

// Initialize ESP-NOW and add peers
void initESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Add Sound Sensor Slave as a peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, soundSensorSlaveMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Sound Sensor Slave");
  } else {
    Serial.println("Sound Sensor Slave added successfully");
  }

  // Add Motion Sensor Slave as a peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, motionSensorSlaveMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Motion Sensor Slave");
  } else {
    Serial.println("Motion Sensor Slave added successfully");
  }

  // Add Smoke Sensor Slave as a peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, smokeSensorSlaveMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Smoke Sensor Slave");
  } else {
    Serial.println("Smoke Sensor Slave added successfully");
  }

  // Add Light Sensor Slave as a peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, lightSensorSlaveMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Light Sensor Slave");
  } else {
    Serial.println("Light Sensor Slave added successfully");
  }
}

// Handle Received Data
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.print("Data received from: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Check the length of the received data
  if (memcmp(mac, soundSensorSlaveMAC, 6) == 0) {
    if (len == sizeof(struct_message)) {
      memcpy(&receivedDataSound, incomingData, sizeof(receivedDataSound));

      // Add the logic for loudness check
      if (receivedDataSound.sensorValue > 1) {  // Set threshold value for loudness
        strcpy(receivedDataSound.sensorStatus, "Loud");
      } else {
        strcpy(receivedDataSound.sensorStatus, "Normal");
      }

      Serial.printf("Sound Level: %d, Sound Status: %s\n", receivedDataSound.sensorValue, receivedDataSound.sensorStatus);
    }
  } else if (memcmp(mac, motionSensorSlaveMAC, 6) == 0) {
    if (len == sizeof(struct_message)) {
      memcpy(&receivedDataMotion, incomingData, sizeof(receivedDataMotion));
      Serial.printf("Motion Level: %d, Motion Status: %s\n", receivedDataMotion.sensorValue, receivedDataMotion.sensorStatus);
    }
  } else if (memcmp(mac, smokeSensorSlaveMAC, 6) == 0) {
    if (len == sizeof(struct_message)) {
      memcpy(&receivedDataSmoke, incomingData, sizeof(receivedDataSmoke));
      Serial.printf("Smoke Level: %d, Smoke Status: %s\n", receivedDataSmoke.sensorValue, receivedDataSmoke.sensorStatus);
    }
  } else if (memcmp(mac, lightSensorSlaveMAC, 6) == 0) {
    if (len == sizeof(struct_message_light)) {
      memcpy(&receivedDataLight, incomingData, sizeof(receivedDataLight));
      Serial.printf("Light Level: %d, Brightness Percentage: %s\n", receivedDataLight.lightLevel, receivedDataLight.brightnessPercentage);
    }
  } else {
    Serial.println("Received data length mismatch.");
  }
}


// Serve the webpage with sensor data
void serveWebpage(AsyncWebServerRequest *request) {
  String html = PAGEINDEX;  // HTML page from external file

  html.replace("%STATUS%", String(receivedDataSound.sensorStatus));
  html.replace("%MOTION%", String(receivedDataMotion.sensorStatus));
  html.replace("%SMOKE%", String(receivedDataSmoke.sensorStatus));
  html.replace("%LIGHT%", String(receivedDataLight.brightnessPercentage));

  request->send(200, "text/html", html);
}

// Serve the light sensor data as JSON
void serveLightData(AsyncWebServerRequest *request) {
  String json = "{\"lightLevel\": " + String(receivedDataLight.lightLevel) + ", ";
  json += "\"brightnessPercentage\": \"" + String(receivedDataLight.brightnessPercentage) + "\"}";
  request->send(200, "application/json", json);
}

// Setup Function
void setup() {
  Serial.begin(115200);

  // Set Wi-Fi mode to AP+STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.channel(1);  // Set initial channel

  // Configure Access Point
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  delay(1000);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Connect to Wi-Fi network (STA)
  WiFi.begin(wifi_network_ssid, wifi_network_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi!");

  scan_and_set_wifi_channel();
  initESPNow();

  // Register the callback for receiving data
  esp_now_register_recv_cb(OnDataRecv);

  // Serve the webpage with sensor data
  server.on("/", HTTP_GET, serveWebpage);

  // Serve the light sensor data as JSON
  server.on("/status/light", HTTP_GET, serveLightData);

  // Serve the sound sensor data
  server.on("/status/sound", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String response = String(receivedDataSound.sensorStatus);
    request->send(200, "text/plain", response);
  });

  // Serve the motion sensor data
  server.on("/status/motion", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String response = String(receivedDataMotion.sensorStatus);
    request->send(200, "text/plain", response);
  });

  // Serve the smoke sensor data
  server.on("/status/smoke", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String response = String(receivedDataSmoke.sensorStatus);
    request->send(200, "text/plain", response);
  });

  // Start the server
  server.begin();
}

// Loop Function
void loop() {
  // Add any repeated tasks here if needed
}
