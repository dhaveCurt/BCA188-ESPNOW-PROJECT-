#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Definitions
#define LIGHT_SENSOR_PIN 34 // ESP32 pin GPIO36 (ADC0)

// Define LED pins
#define LED1_PIN 13 // GPIO13 for LED1
#define LED2_PIN 12 // GPIO12 for LED2
#define LED3_PIN 14 // GPIO14 for LED3

// Define PWM settings
#define pwm_freq 35000 // 35 kHz frequency
#define pwm_res 8      // 8-bit resolution

// Low-pass filter smoothing factor
#define SMOOTHING_FACTOR 0.05 // Smaller value for smoother adjustments (closer to 0 for slower, smoother)

// Previous brightness to apply smoothing
int prevBrightness = 255; // Start with maximum brightness
int loopState = 0;
unsigned long lastReadingTime = 10000;
int lightLevel;
int brightness;
int percentage;
// Structure for ESP-NOW data
typedef struct struct_message
{
  int lightLevel;
  char brightnessPercentage[10]; // Add a field to send the brightness percentage
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

  // Print sent data
  Serial.print("Sent Light Level: ");
  Serial.print(myData.lightLevel);
  Serial.print(", Brightness Percentage: ");
  Serial.println(myData.brightnessPercentage);
}
// Callback for received data
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  char receivedMessage[len + 1];
  memcpy(receivedMessage, incomingData, len);
  receivedMessage[len] = '\0';

  Serial.print("Received message: ");
  Serial.println(receivedMessage);

  if (memcmp(mac, masterMAC, 6) == 0)
  {
    if (strcmp(receivedMessage, "disable1") == 0)
    {
      // Turn off all LEDs
      ledcWrite(0, 0);
      ledcWrite(1, 0);
      ledcWrite(2, 0);
      lightLevel = 0;
      brightness = 0;
      myData.lightLevel = lightLevel;
      myData.brightnessPercentage[0] = '\0';
      // Print the current light level
      Serial.print("Light Level: ");
      Serial.println(lightLevel);
      Serial.print("LED Brightness: ");
      Serial.println(brightness);
      sendDataToMaster();
      loopState =0;
      Serial.println("Disable1 command received, entering deep sleep.");
      // Enter deep sleep if intended
      esp_deep_sleep_start();
    }
    else if (strcmp(receivedMessage, "disable") == 0)
    {
      
      ledcWrite(0, 0);
      ledcWrite(1, 0);
      ledcWrite(2, 0);
      lightLevel = 0;
      brightness = 0;
      myData.lightLevel = lightLevel;
      myData.brightnessPercentage[0] = '\0';
      // Print the current light level
      Serial.print("Light Level: ");
      Serial.println(lightLevel);
      Serial.print("LED Brightness: ");
      Serial.println(brightness);
      sendDataToMaster();
      Serial.println("Disable command received, turning off LEDs.");
      loopState = 0;
      
    }
    else if (strcmp(receivedMessage, "turn on") == 0)
    {
      unsigned long currentTime = millis();
      const int delayState = 10000;
      
      if(currentTime-lastReadingTime >= delayState)
      {
      Serial.println("Turn on command received, enabling light control.");
      loopState = 1;
      lastReadingTime = currentTime;
      }
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
}



void setup()
{
  Serial.begin(115200);

  // Setup PWM for each LED pin using the correct ledcAttachPin function
  ledcSetup(0, pwm_freq, pwm_res); // Set up PWM for LED1 (GPIO13)
  ledcAttachPin(LED1_PIN, 0);      // Attach LED1_PIN to PWM channel 0

  ledcSetup(1, pwm_freq, pwm_res); // Set up PWM for LED2 (GPIO12)
  ledcAttachPin(LED2_PIN, 1);      // Attach LED2_PIN to PWM channel 1

  ledcSetup(2, pwm_freq, pwm_res); // Set up PWM for LED3 (GPIO14)
  ledcAttachPin(LED3_PIN, 2);      // Attach LED3_PIN to PWM channel 2

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

  // Register ESP-NOW receive callback
  esp_now_register_recv_cb(OnDataRecv);

  // Print starting message
  Serial.println("Light sensor and brightness control started...");
}

void loop()
{

  if (loopState == 1)
  {
    // Read the light sensor value (0 to 4095)
    lightLevel = analogRead(LIGHT_SENSOR_PIN);

    // Print the current light level
    Serial.print("Light Level: ");
    Serial.println(lightLevel);

    // Determine the target brightness based on light level ranges
    int targetBrightness = 0;

    if (lightLevel <= 100)
    {
      targetBrightness = 255;
    }
    else if (lightLevel <= 300)
    {
      targetBrightness = 230;
    }
    else if (lightLevel <= 800)
    {
      targetBrightness = 200;
    }
    else if (lightLevel <= 1500)
    {
      targetBrightness = 150;
    }
    else if (lightLevel <= 2500)
    {
      targetBrightness = 100;
    }
    else
    {
      targetBrightness = 0;
    }

    // Convert the targetBrightness into a percentage of 255
    percentage = round((float)prevBrightness / 255.0 * 100);

    // Set the brightnessPercentage string (this is the data to send)
    snprintf(myData.brightnessPercentage, sizeof(myData.brightnessPercentage), "%d%%", percentage);

    // Gradually adjust brightness towards the target using a low-pass filter
    brightness = prevBrightness + (targetBrightness - prevBrightness) * SMOOTHING_FACTOR;

    // Ensure brightness stays within valid range (0 to 255)
    brightness = constrain(brightness, 0, 255);

    // Update previous brightness for next loop
    prevBrightness = brightness;

    // Set the LED brightness for all LEDs
    ledcWrite(0, brightness); // Set brightness for LED1 (GPIO13)
    ledcWrite(1, brightness); // Set brightness for LED2 (GPIO12)
    ledcWrite(2, brightness); // Set brightness for LED3 (GPIO14)

    // Print the brightness
    Serial.print("LED Brightness: ");
    Serial.println(brightness);

    // Send light sensor data to master
    myData.lightLevel = lightLevel;
    sendDataToMaster();

    // Add a small delay to allow the change to be visible
    delay(100);
  }
  else
  {
    ledcWrite(0, 0); // Set brightness for LED1 (GPIO13)
    ledcWrite(1, 0); // Set brightness for LED2 (GPIO12)
    ledcWrite(2, 0); // Set brightness for LED3 (GPIO14)
    lightLevel = 0;
    percentage = 0;
    snprintf(myData.brightnessPercentage, sizeof(myData.brightnessPercentage), "%d%%", percentage);
    // Print the current light level
    Serial.print("Light Level: ");
    Serial.println(lightLevel);
    Serial.print("LED Brightness: ");
    Serial.println(brightness);
    myData.lightLevel = lightLevel;
    sendDataToMaster();

    delay(100);
  }
}
