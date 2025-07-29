#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_NeoPixel.h>

// WiFi Credentials 
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Firebase Credentials 
#define FIREBASE_HOST "https://led-connection-a5653-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "nEnhsKqxrl1wChCyvMJIox1IOZvFcY75Y8bvY7kc"
#define FIREBASE_API_KEY "AIzaSyAhMYfJDXjU6Pbhw2VnBz9hh5H205MGRF4"

// LED Strip Configuration
#define LED_PIN 33
#define NUM_LEDS 10

// Firebase Data object for Firebase_ESP_Client
FirebaseData firebaseData;

// Declare the FirebaseConfig and FirebaseAuth objects
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// Declare a FirebaseJson object 
FirebaseJson jsonProcessor;

// NeoPixel object
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Timing Variables 
unsigned long previousWifiCheckMillis = 0;
const long wifiCheckInterval = 5000;

// Global variables 
String current_colorHex = "#FFFFFF"; /
int current_brightness = 150;       


void connectToWiFi();

void setup() {
  Serial.begin(115200);
  Serial.println();

  connectToWiFi();

  // Configure FirebaseConfig 
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.api_key = FIREBASE_API_KEY;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  strip.begin();
  strip.show();

  if (!Firebase.RTDB.beginStream(&firebaseData, "/led_control")) {
    Serial.println("Failed to begin Firebase stream.");
    Serial.println(firebaseData.errorReason());
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousWifiCheckMillis >= wifiCheckInterval) {
    previousWifiCheckMillis = currentMillis;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected. Attempting reconnect...");
      connectToWiFi();
    }
  }

  // Polling for Firebase_ESP_Client 
  if (Firebase.RTDB.readStream(&firebaseData)) {
    // Check if the received data is JSON
    if (firebaseData.dataType() == "json") {
      String jsonStr = firebaseData.jsonString();
      Serial.println("Received Firebase Data: " + jsonStr);

      jsonProcessor.setJsonData(jsonStr);

      // Declare a FirebaseJsonData object to hold extracted values
      FirebaseJsonData jsonData;

      // Temporary variable for state, other values use global persistent ones
      String temp_state; 

      // Extract values from the FirebaseJson object using jsonData
      // Only update global variables if successfully retrieved from the current JSON payload
      if (jsonProcessor.get(jsonData, "/color")) {
        current_colorHex = jsonData.stringValue; 
      } else {
        Serial.println("JSON: /color not found in current payload. Using last known.");
      }

      if (jsonProcessor.get(jsonData, "/brightness")) {
        current_brightness = jsonData.intValue; 
      } else {
        Serial.println("JSON: /brightness not found in current payload. Using last known.");
      }

      if (jsonProcessor.get(jsonData, "/state")) {
        temp_state = jsonData.stringValue; 
      } else {
        Serial.println("JSON: /state not found in current payload.");
      }

      // Proceed only if state is valid
      if (temp_state == "ON") {
        long number = (long) strtol(current_colorHex.substring(1).c_str(), NULL, 16);
        int r = (number >> 16) & 0xFF;
        int g = (number >> 8) & 0xFF;
        int b = (number >> 0) & 0xFF;

        // Use current_brightness for mapping
        r = map(r, 0, 255, 0, current_brightness);
        g = map(g, 0, 255, 0, current_brightness);
        b = map(b, 0, 255, 0, current_brightness);

        for (int i = 0; i < NUM_LEDS; i++) {
          strip.setPixelColor(i, r, g, b);
        }
        strip.show();
        Serial.printf("LEDs ON. Color: %s, Brightness: %d (RGB: %d,%d,%d)\n", current_colorHex.c_str(), current_brightness, r, g, b);

      } else if (temp_state == "OFF") {
        for (int i = 0; i < NUM_LEDS; i++) {
          strip.setPixelColor(i, 0, 0, 0);
        }
        strip.show();
        Serial.println("LEDs OFF");
      }
    } else {
        Serial.println("Received non-JSON data type or other event, ignoring.");
    }
  }
}

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect to WiFi after timeout.");
    }
  }
}