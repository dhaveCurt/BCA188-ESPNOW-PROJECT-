#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Definitions
#define SENSOR_PIN 34             // Connect A0 of the sound sensor to GPIO34 (ADC pin on ESP32)
#define LED_PIN 26                // Connect an LED to GPIO26
#define BASELINE_SAMPLE_COUNT 100 // Number of samples to calculate baseline
#define THRESHOLD_OFFSET 50       // Offset above baseline for loud sound detection
#define DEBOUNCE_DELAY 100        // Debounce delay for stable reading

int baselineLevel = 0;             // Stores the calculated baseline noise level
unsigned long lastReadingTime = 0; // Timestamp of the last valid reading
bool sensorEnabled = true;         // Flag to enable or disable the sensor
int soundLevel;

// Structure for ESP-NOW data
typedef struct struct_message
{
  int soundLevel;
  char soundStatus[20];
} struct_message;

struct_message myData;

// Master's MAC Address (Replace with actual MAC)
uint8_t masterMAC[] = {0x10, 0x06, 0x1c, 0xb5, 0x3e, 0x84}; // Replace with master MAC

// Wi-Fi Network SSID
const char *wifi_network_ssid = "ESP32_WS";           // Wi-Fi network SSID
const char *wifi_network_password = "helloesp32WS"; // Wi-Fi network password

// Get Wi-Fi channel for the specified SSID
int32_t get_wifi_channel(const char *ssid)
{
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i)
  {
    if (String(ssid) == WiFi.SSID(i))
    {
      return WiFi.channel(i);
    }
  }
  return 0;
}

// Set Wi-Fi channel for ESP32
void scan_and_set_wifi_channel()
{
  Serial.printf("\nScanning for SSID: %s\n", wifi_network_ssid);
  int slave_channel = get_wifi_channel(wifi_network_ssid);

  if (slave_channel == 0)
  {
    Serial.println("SSID not found!");
  }
  else
  {
    Serial.printf("SSID found. Channel: %d\n", slave_channel);

    if (WiFi.channel() != slave_channel)
    {
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_channel(slave_channel, WIFI_SECOND_CHAN_NONE);
      esp_wifi_set_promiscuous(false);
    }
  }
  Serial.printf("Current Wi-Fi channel: %d\n", WiFi.channel());
}

// Callback for send status
void OnDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
// Send data to master
void sendDataToMaster()
{
  esp_err_t result = esp_now_send(masterMAC, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK)
  {
    Serial.println("Data sent successfully!");
  }
  else
  {
    Serial.println("Error sending data");
  }

  Serial.print(", Sound Status: ");
  Serial.println(myData.soundStatus);
}
// Callback for received data
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  char receivedCommand[20];
  memcpy(receivedCommand, incomingData, len);
  receivedCommand[len] = '\0'; // Null-terminate the string

  Serial.print("Received data: ");
  Serial.println(receivedCommand);
  if (memcmp(mac, masterMAC, 6) == 0)
  {
    if (strcmp(receivedCommand, "disable1") == 0)
    { 
      soundLevel = 0;
      myData.soundLevel = soundLevel;
      strcpy(myData.soundStatus, "DISABLED");
      Serial.println(myData.soundStatus);
      sendDataToMaster();
      sensorEnabled = false;
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
      esp_deep_sleep_start();
    }
    else if (strcmp(receivedCommand, "turn on") == 0)
    {
      sensorEnabled = true;
      Serial.println("Sensor and LED enabled");
    }
    else if (strcmp(receivedCommand, "disable") == 0)
    {
      soundLevel = 0;
      myData.soundLevel = soundLevel;
      strcpy(myData.soundStatus, "DISABLED");
      Serial.println(myData.soundStatus);
      sensorEnabled = false;
      digitalWrite(LED_PIN, LOW); // Turn off the LED
      Serial.println("Sensor and LED disabled");
      sendDataToMaster();
    }
  }
}

// Initialize ESP-NOW
void initESPNow()
{
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set the MAC address of the master device
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add master as a peer");
    return;
  }
  Serial.println("Master added as a peer!");

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);
}

void setup()
{
  Serial.begin(115200);

  // Setup sensor and LED pins
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  // Calculate the baseline noise level
  Serial.println("Calibrating baseline noise level...");
  long total = 0;
  for (int i = 0; i < BASELINE_SAMPLE_COUNT; i++)
  {
    total += analogRead(SENSOR_PIN);
    delay(10); // Small delay between readings
  }
  baselineLevel = total / BASELINE_SAMPLE_COUNT;
  Serial.print("Baseline noise level: ");
  Serial.println(baselineLevel);

  // Setup WiFi (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.channel(1); // Set initial channel

  // Set Wi-Fi channel based on the master's Wi-Fi network
  scan_and_set_wifi_channel();

  // Initialize ESP-NOW
  initESPNow();

  // Register ESP-NOW send callback
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  if (sensorEnabled)
  {
    static unsigned long lastSendTime = 0;  // Last time data was sent
    static unsigned long ledOnTime = 0;     // Last time the LED was turned on
    const unsigned long sendInterval = 500; // Interval for sending data (milliseconds)
    const unsigned long ledDuration = 2000; // LED on duration when loud sound is detected

    unsigned long currentTime = millis();

    // Read sound level
    soundLevel = analogRead(SENSOR_PIN);

    // Filter unstable readings with debounce logic
    if (currentTime - lastReadingTime > DEBOUNCE_DELAY)
    {
      // Determine sound status based on the threshold
      if (soundLevel > baselineLevel + THRESHOLD_OFFSET)
      {
        strcpy(myData.soundStatus, "LOUD");
        digitalWrite(LED_PIN, HIGH); // Turn on the LED
        ledOnTime = currentTime;     // Record the time the LED was turned on
      }
      else if (currentTime - ledOnTime > ledDuration)
      {
        // Turn off the LED if the duration has passed
        strcpy(myData.soundStatus, "QUIET");

        digitalWrite(LED_PIN, LOW);
      }
      // Update last reading time
      lastReadingTime = currentTime;
    }

    // Send data to master at regular intervals
    if (currentTime - lastSendTime > sendInterval)
    {
      myData.soundLevel = soundLevel; // Update the sound level
      sendDataToMaster();             // Send data via ESP-NOW
      lastSendTime = currentTime;     // Update last send time
    }
  }
}
