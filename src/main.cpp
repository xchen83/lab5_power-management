#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WiFi and Firebase Configuration
const char* ssid = "YourSSID";
const char* password = "YourPassword";
#define DATABASE_URL "YourDatabaseURL"
#define API_KEY "YourAPIKey"

// Ultrasonic Sensor Pins
const int trigPin = D2;
const int echoPin = D3;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Timing and Policy Variables
unsigned long lastMovementTime = 0;
const unsigned long movementTimeout = 5000; // 5 seconds
const unsigned long deepSleepDuration = 45e6; // 45 seconds in microseconds
bool movementDetected = false;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// HC-SR04 Pins
const int trigPin = D2;
const int echoPin = D3;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Function prototypes
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  connectToWiFi();
  initFirebase();
}

void loop() {
  float distance = measureDistance();

  // Check for movement
  if (distance <= 40) {
    // Movement detected
    movementDetected = true;
    lastMovementTime = millis();
    sendDataToFirebase(distance);
  } else if (movementDetected && millis() - lastMovementTime > movementTimeout) {
    // No movement detected for 30 seconds, enter deep sleep
    Serial.println("No movement detected, going to deep sleep.");
    WiFi.disconnect(true);
    esp_sleep_enable_timer_wakeup(deepSleepDuration);
    esp_deep_sleep_start();
  }

  delay(100); // Short delay to reduce power consumption
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (!Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up failed");
    return;
  }
  Firebase.begin(&config, &auth);
}

void sendDataToFirebase(float distance) {
  if (Firebase.ready()) {
    if (!Firebase.RTDB.pushFloat(&fbdo, "/sensor/distance", distance)) {
      Serial.println("Failed to send data to Firebase");
    }
  }
}