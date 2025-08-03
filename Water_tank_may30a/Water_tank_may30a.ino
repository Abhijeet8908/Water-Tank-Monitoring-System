#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h> // Required for HTTPS

// --- WiFi Credentials ---
// IMPORTANT: Replace with your actual WiFi SSID and password
const char* ssid = ""; // wifi name
const char* password = ""; //wifi pass

// --- Server Details ---
// IMPORTANT: Replace with the URL of your Google Cloud Run service
// Example: "https://my-flask-app-xyz-uc.a.run.app"
const char* serverUrl = ""; //replace with ur run app url

// Define GPIO pins for float sensors
const int SENSOR_PIN_25 = D1; // GPIO5
const int SENSOR_PIN_50 = D2; // GPIO4
const int SENSOR_PIN_75 = D5; // GPIO14
const int SENSOR_PIN_100 = D6; // GPIO12

// Update interval (milliseconds)
const long UPDATE_INTERVAL = 10000;
unsigned long previousMillis = 0;

// Function Prototypes
void connectToWiFi();
int getWaterLevel();
void sendWaterLevelToWebpage(int waterLevel);
void checkForResetCommand();

void setup() {
  Serial.begin(115200);
  delay(100);

  // Configure sensor pins with internal pull-up resistors
  pinMode(SENSOR_PIN_25, INPUT_PULLUP);
  pinMode(SENSOR_PIN_50, INPUT_PULLUP);
  pinMode(SENSOR_PIN_75, INPUT_PULLUP);
  pinMode(SENSOR_PIN_100, INPUT_PULLUP);

  connectToWiFi();
}

void loop() {
  connectToWiFi(); // Ensure WiFi is connected

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
    previousMillis = currentMillis;

    int waterLevel = getWaterLevel();
    Serial.printf("Current Water Level: %d%%\n", waterLevel);

    sendWaterLevelToWebpage(waterLevel);
    checkForResetCommand();
  }
  delay(1000);
}

// Function to connect to Wi-Fi
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully!");
    Serial.print("NodeMCU IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

// Function to get the current water level
int getWaterLevel() {
  if (digitalRead(SENSOR_PIN_100) == LOW) return 100;
  if (digitalRead(SENSOR_PIN_75) == LOW) return 75;
  if (digitalRead(SENSOR_PIN_50) == LOW) return 50;
  if (digitalRead(SENSOR_PIN_25) == LOW) return 25;
  return 0;
}

// Function to send water level to the Cloud Run server
void sendWaterLevelToWebpage(int waterLevel) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure wifiClient;
    HTTPClient http;

    // This is needed for HTTPS connections to cloud services from ESP8266
    // It skips certificate validation. For production, consider certificate pinning.
    wifiClient.setInsecure();

    // Construct the full URL for the request
    String fullUrl = String(serverUrl) + "/update_waterlevel?level=" + String(waterLevel);
    Serial.print("Sending request to: ");
    Serial.println(fullUrl);

    if (http.begin(wifiClient, fullUrl)) {
      int httpCode = http.GET();
      if (httpCode > 0) {
        Serial.printf("[HTTPS] GET... response code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Server response: " + payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.println("[HTTPS] Unable to connect");
    }
  } else {
    Serial.println("Cannot send data: WiFi is not connected.");
  }
}

// Function to check for a reset command from the Cloud Run server
void checkForResetCommand() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure wifiClient;
    HTTPClient http;

    wifiClient.setInsecure();

    String fullUrl = String(serverUrl) + "/get_nodemcu_command";
    Serial.print("Checking for commands from: ");
    Serial.println(fullUrl);

    if (http.begin(wifiClient, fullUrl)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Command response: " + payload);
        if (payload.indexOf("\"reset\":true") != -1) {
          Serial.println("RESET command received! Restarting NodeMCU...");
          delay(1000);
          ESP.restart();
        }
      } else {
        Serial.printf("[HTTPS] Command check failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.println("[HTTPS] Unable to connect for command check");
    }
  }
}
