#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 150 

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806, define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 7
#define CLOCK_PIN 13
#define FRAMES_PER_SECOND  20
#define COLOR_ORDER GRB

// 0=Red, 32=Orange, 64=Yellow

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
	Serial.begin(115200);
	Serial.println("resetting");
	LEDS.addLeds<WS2812,DATA_PIN,COLOR_ORDER>(leds,NUM_LEDS);
	LEDS.setBrightness(60);
  delay(1000); // traditional, for unknown reasons
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }
unsigned int dwait=20;
uint8_t wc=0;
uint8_t hue = 0;


void loop() { 
unsigned int dmax;  // delay interval, msec

// rainbow is about 0..192
int pOffset = 18;  // LED spacing between pulses
int hOffset = 32;  // hue delta between pulse colors 
int fRate = 45;    // fade rate per frame, % as fraction of 255
uint8_t B;         // overall LED brightness setting (out of 255)
int k;             // index for LED array


hue = 20;   // 0 = red,64=yellow, 96 = green,160=blue, 192 = violet

B = 50;
LEDS.setBrightness(B);

int d=150; // diameter of sparks blob
// bright pulsing blob in center
int br=245; // brightness of main spark
int imax=FRAMES_PER_SECOND*2;

for (int i=0; i<imax; i++) {
  fadeToBlackBy( leds, NUM_LEDS, fRate/2);
  for (int j=0;j<3;j++) {
    leds[random8(d)] += CHSV(hue+random8(hue),255,128+random8(64));
    leds[random8(d)] += CHSV(hue/2+random8(hue/2),255,180+random8(75));
  }
  FastLED.show();
  FastLED.delay(1000/FRAMES_PER_SECOND); 
}


/*

// wait for LED trail to fade away at end
  dmax = 50;
  for(int i=0; i<dmax; i++) {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND);     
  }
*/

} // end loop()
