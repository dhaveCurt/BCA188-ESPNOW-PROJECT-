#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>

#define button_pin 5
#define HOUR 3600000
#define MINUTE 60000
#define SECOND 1000

const int ledPin = 2;
const int inputPin = 4;

bool sensorState = LOW;
bool lastSensorState = LOW;

int buttonState = LOW;  // Current state of the button
int lastButtonState = LOW;  // Previous state of the button
unsigned long lastButtonDebounceTime = 0;  // Last time button state was changed
const unsigned long debounceButtonDelay = 50;  // Debounce delay in milliseconds

unsigned long buttonPressStartTime = 0;  // Timer for button press duration
bool buttonPressed = false;  // Flag to track if the button is pressed
unsigned long buttonPressDuration = 0; // Duration of button press
unsigned long currentTime;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 500;
unsigned long lastSentTime = 0;
const unsigned long sendInterval = 5 * MINUTE;

typedef struct struct_message {
  int motionValue;  // 1 for motion detected, 0 for no motion
  char motionStatus[20];  // "Motion Detected" or "No Motion Detected"
} struct_message;

struct_message myData;

uint8_t masterMAC[] = {0x10, 0x06, 0x1c, 0xb5, 0x3e, 0x84};

const char* wifi_network_ssid = "ESP32_WS";
const char* wifi_network_password = "helloesp32WS";

// Function to get Wi-Fi channel for the specified SSID
int32_t get_wifi_channel(const char* ssid) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (String(ssid) == WiFi.SSID(i)) {
      return WiFi.channel(i);
    }
  }
  return 0;
}

// Function to scan and set Wi-Fi channel
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

// Callback function for data sent status
void OnDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Function to initialize ESP-NOW
void initESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

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

// Function to send data to master
void sendDataToMaster() {
  esp_err_t result = esp_now_send(masterMAC, (uint8_t*)&myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Data sent successfully!");
  } else {
    Serial.println("Error sending data");
  }

  Serial.print("Motion Value: ");
  Serial.println(myData.motionValue);
  Serial.print("Motion Status: ");
  Serial.println(myData.motionStatus);
}

// Function to log events
void logEvent(const char* message) {
  currentTime = millis();
  unsigned long hours = currentTime / HOUR;
  unsigned long minutes = (currentTime % HOUR) / MINUTE;
  unsigned long seconds = (currentTime % MINUTE) / SECOND;

  char printBuffer[128];
  sprintf(printBuffer, "%lu hr %lu min %lu sec: %s", hours, minutes, seconds, message);
  Serial.println(printBuffer);
}

// Function to send motion data
void sendMotionData() {
  logEvent("Motion detected! Sending data to the other ESP32...");
  myData.motionValue = 1;  // Motion detected
  strcpy(myData.motionStatus, "Motion Detected");
  sendDataToMaster();
}
void sendDisableData() {
  logEvent("DISABLED! Sending data to the other ESP32...");
  myData.motionValue = 0;  // Motion detected
  strcpy(myData.motionStatus, "DISABLED");
  sendDataToMaster();
}
// Function to send turn off data
void sendTurnOffData() {
  logEvent("Turn Off! Sending data to the other ESP32...");
  myData.motionValue = 2;  // No motion
  sendDataToMaster();
}
// Function to send no motion data
void sendNoMotionData() {
  logEvent("No motion detected! Sending data to the other ESP32...");
  strcpy(myData.motionStatus, "No Motion Detected");
  sendDataToMaster();
}
void setup() {
  Serial.begin(115200);
  pinMode(button_pin, INPUT_PULLUP); // Enable internal pull-up resistor
  pinMode(inputPin, INPUT);
  pinMode(ledPin, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.channel(1);

  scan_and_set_wifi_channel();
  initESPNow();
  esp_now_register_send_cb(OnDataSent);
}

void loop() {
  int reading = digitalRead(inputPin);
  // Debouncing logic
  if (reading != lastSensorState) {
    lastDebounceTime = millis();
    lastSensorState = reading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != sensorState) {
      sensorState = reading;

      if (sensorState == HIGH) {  // Motion detected
        digitalWrite(ledPin, HIGH);  // Turn on LED
        sendMotionData();
        digitalWrite(ledPin, LOW);  // Turn off LED
      } else {
        sendNoMotionData();
      }
    }
  }

  // Button debouncing logic
  int buttonReading = digitalRead(button_pin);
  if (buttonReading != lastButtonState) {
    lastButtonDebounceTime = millis();
    lastButtonState = buttonReading;
  }

  if ((millis() - lastButtonDebounceTime) > debounceButtonDelay) {
    if (buttonReading != buttonState) {
      buttonState = buttonReading;

      if (buttonState == LOW) { // Button pressed
        buttonPressStartTime = millis();
        buttonPressed = true;
      } else if (buttonPressed) { // Button released
        unsigned long buttonPressDuration = millis() - buttonPressStartTime;
        buttonPressed = false;

        if (buttonPressDuration >= 5000 && lastSensorState == LOW) {
          sendTurnOffData();
        }
      }
    }
  }
}
