#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include "text_timer.h"

// Touch sensor pin
#define TOUCH_PIN D2   // TTP223B SIG on D2

// DHT11 Temperature sensor
#define DHTPIN D1      // DHT11 on D1 (GPIO 5)
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Define LED Matrix type and pins
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D6

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Replace with your WiFi credentials
const char* ssid = "Nisha 4g";
const char* password = "khush292009";

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);  // 19800 = UTC +5:30

String lastTime = "";
bool showTemp = false;  // false = show time, true = show temperature
bool lastTouchState = LOW;
unsigned long tempDisplayTime = 0;
const unsigned long TEMP_DISPLAY_DURATION = 5000;  // Show temp for 5 seconds
bool was_text_or_timer_active = false;

void setup() {
  Serial.begin(115200);
  
  // Setup touch sensor
  pinMode(TOUCH_PIN, INPUT);
  
  // Initialize DHT sensor
  dht.begin();
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());
  
  timeClient.begin();
  
  myDisplay.begin();
  myDisplay.setIntensity(5);
  myDisplay.displayClear();
  
  text_timer_setup(); // Initialize the web server and text/timer functions
  
  lastTouchState = digitalRead(TOUCH_PIN);
  
  Serial.println("Smart Clock Ready");
  Serial.println("Default: Time | Touch: Temperature");
}

void loop() {
  text_timer_loop(); // Handle web server and text/timer display

  bool is_active = is_text_or_timer_active();

  // Check if the mode has just changed from active to inactive
  if (was_text_or_timer_active && !is_active) {
    lastTime = ""; // Force an immediate time update
    myDisplay.displayClear();
  }

  if (!is_active) {
    // Handle touch sensor
    bool currentTouchState = digitalRead(TOUCH_PIN);
    
    // Detect rising edge (LOW to HIGH)
    if (currentTouchState == HIGH && lastTouchState == LOW) {
      showTemp = true;
      tempDisplayTime = millis();  // Record when temp was requested
      
      // Read and display temperature immediately
      float temperature = dht.readTemperature();
      
      if (isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        myDisplay.displayClear();
        myDisplay.displayText("ERR", PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
      } else {
        char tempStr[10];
        sprintf(tempStr, "%.1fC", temperature);  // Format: 28.2C
        
        myDisplay.displayClear();
        myDisplay.displayText(tempStr, PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);
        myDisplay.displayAnimate();
        
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" °C");
      }
      
      delay(50); // Debounce delay
    }
    
    lastTouchState = currentTouchState;
    
    // Auto return to time after 5 seconds
    if (showTemp && (millis() - tempDisplayTime > TEMP_DISPLAY_DURATION)) {
      showTemp = false;
      myDisplay.displayClear();
      lastTime = "";  // Force time update
      Serial.println("Returning to time display");
    }
    
    // Update time display when not showing temperature
    if (!showTemp) {
      timeClient.update();
      int hours = timeClient.getHours();
      int minutes = timeClient.getMinutes();
      
      // Format time as HH:MM (no seconds)
      char timeStr[6];
      sprintf(timeStr, "%02d:%02d", hours, minutes);
      
      // Only update when the minute changes
      if (lastTime != timeStr) {
        lastTime = timeStr;
        myDisplay.displayClear();
        myDisplay.displayText(timeStr, PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
        myDisplay.displayAnimate();
        Serial.println(timeStr);
      }
    }
  }
  
  was_text_or_timer_active = is_active; // Update the state for the next loop
  delay(10);
}
