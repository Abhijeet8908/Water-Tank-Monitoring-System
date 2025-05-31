#include "arduino_secrets.h"
#include <ESP8266HTTPClient.h>
#include "thingProperties.h"
#include <ESP8266WiFi.h>

// --- Flask Server Details ---
// IMPORTANT: Replace with the IP address of the computer running your Flask app.
// You can find this by running 'ipconfig' (Windows) or 'ifconfig' (Linux/macOS)
// in your computer's terminal. Look for your local IP address (e.g., 192.168.1.100).
const char* flaskServerIp = "192.168.29.119";
const int flaskServerPort = 5000; // Default Flask port

// Define GPIO pins for float sensors
const int SENSOR_PIN_25 = D1;   // GPIO5
const int SENSOR_PIN_50 = D2;   // GPIO4
const int SENSOR_PIN_75 = D5;   // GPIO14
const int SENSOR_PIN_100 = D6;  // GPIO12

// Update interval (milliseconds)
const long UPDATE_INTERVAL = 10000;
unsigned long previousMillis = 0;

int getWaterLevel() {
  if (digitalRead(SENSOR_PIN_100) == LOW) return 100;
  if (digitalRead(SENSOR_PIN_75) == LOW) return 75;
  if (digitalRead(SENSOR_PIN_50) == LOW) return 50;
  if (digitalRead(SENSOR_PIN_25) == LOW) return 25;
  return 0;
}


// Function to send water level to the Flask server
void sendWaterLevelToWebpage(int waterLevel) {

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Construct the URL with the water level as a query parameter
    String url = "http://";
    url += flaskServerIp;
    url += ":";
    url += flaskServerPort;
    url += "/update_waterlevel?level=";
    url += String(waterLevel); // Convert int waterLevel to String

    Serial.print("Sending water level: ");
    Serial.println(url);

    http.begin(url); // Specify the URL

    // Send the GET request
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end(); // Free resources
  } else {
    Serial.println("Cannot send data: WiFi not connected.");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Configure sensor pins
  pinMode(SENSOR_PIN_25, INPUT_PULLUP);
  pinMode(SENSOR_PIN_50, INPUT_PULLUP);
  pinMode(SENSOR_PIN_75, INPUT_PULLUP);
  pinMode(SENSOR_PIN_100, INPUT_PULLUP);
  
  // Initialize IoT Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
}

void loop() {
  ArduinoCloud.update();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
    previousMillis = currentMillis;
    
    // Update cloud variable
    waterLevel = getWaterLevel();
    Serial.printf("Water level: %d%%\n", waterLevel);
    sendWaterLevelToWebpage(waterLevel);
    // Serial.printf("Data sent successfully");
  }
  delay(2000);
  // Serial.printf("----------------------------");
  // Serial.printf("%d\n", WiFi.status());  
  // Serial.printf("----------------------------");
}