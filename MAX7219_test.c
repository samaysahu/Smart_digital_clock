#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define the type of hardware and pins used
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4   // Number of 8x8 matrices connected (change if you have more)

// SPI connections
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D6

// Create a new Parola object
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void setup() {
  myDisplay.begin();
  myDisplay.setIntensity(5); // Brightness (0–15)
  myDisplay.displayClear();
  myDisplay.displayScroll("HELLO FROM ESP8266!", PA_CENTER, PA_SCROLL_LEFT, 100);
}

void loop() {
  if (myDisplay.displayAnimate()) {
    myDisplay.displayReset();
  }
}
