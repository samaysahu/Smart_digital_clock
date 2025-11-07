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
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ==================== WIFI & SERVER SETUP ====================
const char* ssid = "Nisha 4g";
const char* password = "khush292009";
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
</script>

</body>
</html>
)rawliteral";

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  myDisplay.begin();
  myDisplay.setIntensity(5);
  myDisplay.displayClear();
  myDisplay.displayScroll("Start", PA_CENTER, PA_SCROLL_LEFT, 100);
  textActive = true;
  scrollMode = true;

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  // Routes
  server.on("/", []() { server.send(200, "text/html", index_html); });

  // ====== TEXT ROUTES ======
  server.on("/text", []() {
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
    textActive = false;
    myDisplay.displayClear();
    Serial.println("Text stopped");
    server.send(200, "text/plain", "Stopped");
  });

  // ====== TIMER ROUTES ======
  server.on("/starttimer", []() {
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
    timerRunning = false;
    myDisplay.displayClear();
    Serial.println("Timer stopped");
    server.send(200, "text/plain", "Timer stopped");
  });

  server.begin();
  Serial.println("Web Server Started!");
}

// ==================== LOOP ====================
void loop() {
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
