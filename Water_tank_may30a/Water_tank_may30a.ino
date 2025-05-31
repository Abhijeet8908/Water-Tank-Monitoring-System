#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// --- Flask Server Details ---
// IMPORTANT: Replace with the IP address of the computer running your Flask app.
// You can find this by running 'ipconfig' (Windows) or 'ifconfig' (Linux/macOS)
// in your computer's terminal. Look for your local IP address (e.g., 192.168.1.100).
const char* flaskServerIp = "192.168.29.119";
const int flaskServerPort = 5000; // Default Flask port

// --- WiFi Credentials ---
// IMPORTANT: Replace with your actual WiFi SSID and password
const char* ssid = "Mishras";
const char* password = "Nopass@123";

// Define GPIO pins for float sensors
const int SENSOR_PIN_25 = D1;   // GPIO5
const int SENSOR_PIN_50 = D2;   // GPIO4
const int SENSOR_PIN_75 = D5;   // GPIO14
const int SENSOR_PIN_100 = D6;  // GPIO12

// Update interval (milliseconds)
const long UPDATE_INTERVAL = 10000;
unsigned long previousMillis = 0;
int waterLevel;

int getWaterLevel() {
  if (digitalRead(SENSOR_PIN_100) == LOW) return 100;
  if (digitalRead(SENSOR_PIN_75) == LOW) return 75;
  if (digitalRead(SENSOR_PIN_50) == LOW) return 50;
  if (digitalRead(SENSOR_PIN_25) == LOW) return 25;
  return 0;
}

// In your NodeMCU sketch (e.g., in loop() or a dedicated function)

// Function to connect to Wi-Fi
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return; // Already connected
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) { // Try for 10 seconds (20 * 500ms)
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("NodeMCU IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Please check credentials and try again.");
  }
}

// Add this function to check for reset command
void checkForResetCommand() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://";
    url += flaskServerIp;
    url += ":";
    url += flaskServerPort;
    url += "/get_nodemcu_command"; // New endpoint for NodeMCU to poll
    Serial.printf("%s",url);

    WiFiClient wifiClient;

    http.begin(wifiClient,url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Parse the JSON response, e.g., {"reset": true} or {"reset": false}
        // You might need a JSON parsing library like ArduinoJson for robust parsing
        // For simplicity, we'll do a basic string check here.
        if (payload.indexOf("\"reset\":true") != -1) {
          Serial.println("Received RESET command from server! Restarting NodeMCU...");
          delay(1000); // Give some time for serial output
          ESP.restart(); // This function restarts the ESP8266
        }
      }
    } else {
      Serial.printf("[HTTP] GET /get_nodemcu_command failed, error: %s\n", http.errorToString(httpCode).c_str());
      Serial.printf("%d",httpCode);
    }
    http.end();
  }
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

    WiFiClient wifiClient;

    http.begin(wifiClient, url); // Specify the URL

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
  delay(100);
  connectToWiFi();
  
  // Configure sensor pins
  pinMode(SENSOR_PIN_25, INPUT_PULLUP);
  pinMode(SENSOR_PIN_50, INPUT_PULLUP);
  pinMode(SENSOR_PIN_75, INPUT_PULLUP);
  pinMode(SENSOR_PIN_100, INPUT_PULLUP);
  
}

void loop() {

  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
    previousMillis = currentMillis;
    
    // Update cloud variable
    waterLevel = getWaterLevel();
    Serial.printf("Water level: %d%%\n", waterLevel);
    sendWaterLevelToWebpage(waterLevel);
    
  }  
  delay(2000);  
}