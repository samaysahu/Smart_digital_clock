#include "text_timer.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// ==================== LED MATRIX SETUP ====================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D6
extern MD_Parola myDisplay;

// ==================== WIFI & SERVER SETUP ====================
extern const char* ssid;
extern const char* password;
ESP8266WebServer server(80);

// ==================== VARIABLES ====================
String currentText = "Start";
bool textActive = false;
bool scrollMode = false;

bool timerRunning = false;
unsigned long previousMillis = 0;
int totalSeconds = 0;
int remainingSeconds = 0;

// ==================== HTML PAGE ====================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Matrix & Timer Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body{font-family:Arial;text-align:center;background:#f4f4f4;margin:0;padding:20px;}
    h1{color:#333;}
    .block{background:white;padding:20px;margin:20px auto;width:90%;max-width:400px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,0.1);}
    input,button{font-size:1.2em;padding:10px;margin:5px;}
    input[type=number]{width:70px;text-align:center;}
    button{cursor:pointer;border:none;border-radius:5px;}
    .start{background:#4CAF50;color:white;}
    .stop{background:#f44336;color:white;}
    .back{background:#2196F3;color:white;}
  </style>
</head>
<body>

<h1>LED Matrix Control</h1>

<div class="block">
  <h2>Text Display</h2>
  <input id="txt" type="text" placeholder="Enter text..."><br>
  <button class="start" onclick="sendText()">Send</button>
  <button class="stop" onclick="stopText()">Stop</button>
</div>

<div class="block">
  <h2>Timer</h2>
  <input id="m" type="number" min="0" max="59" value="0"> :
  <input id="s" type="number" min="0" max="59" value="10"><br>
  <button class="start" onclick="startTimer()">Start</button>
  <button class="stop" onclick="stopTimer()">Stop</button>
</div>

<div class="block">
  <h2>Mode</h2>
  <button class="back" onclick="backToTime()">Back to Time</button>
</div>

<script>
function sendText(){
  let txt=document.getElementById('txt').value;
  fetch('/text?msg='+encodeURIComponent(txt));
}
function stopText(){
  fetch('/stoptext');
}
function startTimer(){
  let m=document.getElementById('m').value;
  let s=document.getElementById('s').value;
  fetch('/starttimer?m='+m+'&s='+s);
}
function stopTimer(){
  fetch('/stoptimer');
}
function backToTime(){
  fetch('/backtotime');
}
</script>

</body>
</html>
)rawliteral";

// ==================== SETUP ====================
void text_timer_setup() {
  // Routes
  server.on("/", []() { server.send(200, "text/html", index_html); });

  // ====== TEXT ROUTES ======
  server.on("/text", []() {
    // If timer is running, don't accept new text
    if (timerRunning) {
      server.send(409, "text/plain", "Timer is active. Please stop it first."); // 409 Conflict
      return;
    }
    if (server.hasArg("msg")) {
      currentText = server.arg("msg");
      currentText.trim();
      if (currentText == "") currentText = "EMPTY";
      textActive = true;
      timerRunning = false;

      myDisplay.displayClear();
      if (currentText.length() > 6) {
        myDisplay.displayScroll(currentText.c_str(), PA_CENTER, PA_SCROLL_LEFT, 100);
        scrollMode = true;
      } else {
        myDisplay.displayText(currentText.c_str(), PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
        scrollMode = false;
      }

      Serial.println("Displaying text: " + currentText);
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/stoptext", []() {
    if (!textActive) { // If text isn't active, do nothing.
      server.send(200, "text/plain", "Already stopped");
      return;
    }
    textActive = false;
    if (!timerRunning) { // Only clear if the timer isn't also running
      myDisplay.displayClear();
    }
    Serial.println("Text stopped");
    server.send(200, "text/plain", "Stopped");
  });

  // ====== TIMER ROUTES ======
  server.on("/starttimer", []() {
    // If text is active, don't start the timer
    if (textActive) {
      server.send(409, "text/plain", "Text is active. Please stop it first."); // 409 Conflict
      return;
    }
    int m = server.arg("m").toInt();
    int s = server.arg("s").toInt();
    totalSeconds = (m * 60) + s;
    remainingSeconds = totalSeconds;
    timerRunning = true;
    textActive = false;
    myDisplay.displayClear();
    Serial.println("Timer started: " + String(m) + "m " + String(s) + "s");
    server.send(200, "text/plain", "Timer started");
  });

  server.on("/stoptimer", []() {
    if (!timerRunning) { // If timer isn't running, do nothing.
      server.send(200, "text/plain", "Already stopped");
      return;
    }
    timerRunning = false;
    if (!textActive) { // Only clear if text isn't also active
      myDisplay.displayClear();
    }
    Serial.println("Timer stopped");
    server.send(200, "text/plain", "Timer stopped");
  });

  server.on("/backtotime", []() {
    // If timer is running, require it to be stopped first
    if (timerRunning) {
      server.send(409, "text/plain", "Timer is active. Please stop it first.");
      return;
    }

    // If text is active, stop it and return to time mode
    if (textActive) {
      textActive = false;
      myDisplay.displayClear();
      Serial.println("Returning to time display");
      server.send(200, "text/plain", "OK");
      return;
    }

    // If neither is active, we are already in time mode
    server.send(200, "text/plain", "Already in time mode");
  });

  server.begin();
  Serial.println("Web Server Started!");
}

// ==================== LOOP ====================
void text_timer_loop() {
  server.handleClient();

  // ====== TEXT DISPLAY ======
  if (textActive) {
    if (scrollMode) {
      if (myDisplay.displayAnimate()) myDisplay.displayReset(); // continuous scroll
    } else {
      myDisplay.displayAnimate();
    }
  }

  // ====== TIMER LOGIC ======
  if (timerRunning) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;
      if (remainingSeconds > 0) {
        remainingSeconds--;
        int min = remainingSeconds / 60;
        int sec = remainingSeconds % 60;
        char buffer[6];
        sprintf(buffer, "%02d:%02d", min, sec);
        myDisplay.displayClear();
        myDisplay.displayText(buffer, PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
        myDisplay.displayAnimate();
        Serial.println(buffer);
      } else {
        myDisplay.displayClear();
        myDisplay.displayScroll("TIME UP", PA_CENTER, PA_SCROLL_LEFT, 100);
        myDisplay.displayAnimate();
        textActive = true;   // scroll TIME UP continuously
        scrollMode = true;
        currentText = "TIME UP";
        timerRunning = false;
      }
    }
  }
}

bool is_text_or_timer_active() {
  return textActive || timerRunning;
}
