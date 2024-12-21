#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Definitions
#define smokeSensorPin 34   // ESP32 analog pin, use an appropriate ADC-capable pin
#define LED_PIN 26           // Connect an LED to GPIO2 (built-in LED on most ESP32 boards)
#define maxSensorValue 1500 // Maximum ADC value for ESP32 (12-bit ADC)
#define BASELINE_SAMPLE_COUNT 100 // Number of samples to calculate baseline
#define THRESHOLD_OFFSET 50       // Offset above baseline for loud sound detection

int baselineLevel = 0; // Stores the calculated baseline noise level

// Structure for ESP-NOW data
typedef struct struct_message {
  int smokePercentage;
  char smokeStatus[20];
  bool blinkLED;  // Flag to trigger LED blinking
} struct_message;

struct_message myData;

// Master's MAC Address (Replace with actual MAC)
uint8_t masterMAC[] = {0x10, 0x06, 0x1c, 0xb5, 0x3e, 0x84}; // Replace with master MAC

// Wi-Fi Network SSID
const char* wifi_network_ssid = "ESP32_WS";  // Wi-Fi network SSID
const char* wifi_network_password = "helloesp32WS"; // Wi-Fi network password

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
  int slave_channel = get_wifi_channel(wifi_network_ssid);

  if (slave_channel == 0) {
    Serial.println("SSID not found!");
  } else {
    Serial.printf("SSID found. Channel: %d\n", slave_channel);

    if (WiFi.channel() != slave_channel) {
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_channel(slave_channel, WIFI_SECOND_CHAN_NONE);
      esp_wifi_set_promiscuous(false);
    }
  }
  Serial.printf("Current Wi-Fi channel: %d\n", WiFi.channel());
}

// Callback for send status
void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Initialize ESP-NOW
void initESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set the MAC address of the master device
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add master as a peer");
    return;
  }
  Serial.println("Master added as a peer!");
}

// Send data to master
void sendDataToMaster() {
  esp_err_t result = esp_now_send(masterMAC, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Data sent successfully!");
  } else {
    Serial.println("Error sending data");
  }

  Serial.print("Sent Smoke Percentage: ");
  Serial.print(myData.smokePercentage);
  Serial.print("%, Smoke Status: ");
  Serial.println(myData.smokeStatus);
}

void blinkLED(int pin, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);  // Turn on LED
    delay(500);               // Wait for half a second
    digitalWrite(pin, LOW);   // Turn off LED
    delay(500);               // Wait for half a second
  }
}

void setup() {
  Serial.begin(115200);

  // Setup sensor and LED pins
  pinMode(smokeSensorPin, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Setup WiFi (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.channel(1);  // Set initial channel

  // Set Wi-Fi channel based on the master's Wi-Fi network
  scan_and_set_wifi_channel();

  // Initialize ESP-NOW
  initESPNow();

  // Register ESP-NOW send callback
  esp_now_register_send_cb(OnDataSent);
}

void loop() {
  // Read the analog value from the smoke sensor
  int sensorValue = analogRead(smokeSensorPin);
  int smokePercentage = (sensorValue * 100) / maxSensorValue;

  // Print the sensor value to the Serial Monitor
  Serial.print("Sensor Value: ");
  Serial.print(smokePercentage);
  Serial.println("%");

  // Check if the sensor value indicates smoke presence
  if (smokePercentage >= 100) {
    strcpy(myData.smokeStatus, "SMOKE DETECTED");
    myData.blinkLED = true; // Signal to blink LEDs on all sensors
    blinkLED(LED_PIN, 3); // Blink LED on smoke sensor itself
  } else {
    strcpy(myData.smokeStatus, "NO SMOKE");
    myData.blinkLED = false; // Stop blinking LEDs
  }

  // Populate the structure with smoke percentage and status
  myData.smokePercentage = smokePercentage;

  // Send data to master
  sendDataToMaster();

  // Delay for readability
  delay(500);
}
