#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 150 

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806, define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 7
#define CLOCK_PIN 13
#define FRAMES_PER_SECOND  120
#define COLOR_ORDER GRB

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
	Serial.begin(115200);
	Serial.println("resetting");
	LEDS.addLeds<WS2812,DATA_PIN,COLOR_ORDER>(leds,NUM_LEDS);
	LEDS.setBrightness(60);
  delay(3000); // traditional, for unknown reasons
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }
unsigned int dwait=20;
uint8_t wc=0;
uint8_t hue = 0;

void hinc() {
    hue += random8(30);  // add a small increment to current hue
}

void loop() { 
unsigned int dmax;  // delay interval, msec

int pOffset = 18;  // LED spacing between pulses
int hOffset = 50;  // hue delta between pulse colors
int fRate = 45;    // fade rate per frame, % as fraction of 255
uint8_t B;         // overall LED brightness setting (out of 255)

  // hinc();
  Serial.println(hue);

  // B = 10+random8(80);
  B = 50;
  LEDS.setBrightness(B);


// initial single color pulse from start to end
	for(int i = 0; i < NUM_LEDS; i++) {
    fadeToBlackBy( leds, NUM_LEDS, fRate);
		leds[i] = CHSV(hue, 255, 255);
		FastLED.show(); 
    FastLED.delay(1000/FRAMES_PER_SECOND); 
	}

// wait for LED trail to fade away
  dmax = 60;
  for(int i=0; i<dmax; i++) {
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND);     
  }
  
// two colors come back
	for(int i = (NUM_LEDS)-1; i >= -pOffset; i--) {
    int j = i+pOffset;
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    if (i >= 0)
  		leds[i] = CHSV(hue, 255, 255);
    if (j < NUM_LEDS) 
      leds[j] += CHSV(hue+hOffset, 255, 255);
		FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND); 
	}

// wait for LED trail to fade away
  dmax = 60;
  for(int i=0; i<dmax; i++) {
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND);     
  }

// three colors go out
  for(int i = 0; i< (NUM_LEDS)+(pOffset*2); i++) {
    int j = i-pOffset;
    int k = i-(2*pOffset);
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    if ((i >= 0) && (i < NUM_LEDS))
      leds[i] = CHSV(hue, 255, 255);
    if ((j < NUM_LEDS) && (j >= 0)) 
      leds[j] += CHSV(hue+hOffset, 255, 255);
    if ((k < NUM_LEDS) && (k >= 0)) 
      leds[k] += CHSV(hue+(2*hOffset), 255, 255);
    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND); 
  }


// wait for LED trail to fade away
  dmax = 60;
  for(int i=0; i<dmax; i++) {
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND);     
  }


// four colors come back
  for(int i = (NUM_LEDS)-1; i >= -(pOffset*3); i--) {
    int j = i+pOffset;
    int k = i+2*pOffset;
    int l = i+3*pOffset;
    
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    if ((i >= 0) && (i < NUM_LEDS))
      leds[i] = CHSV(hue, 255, 255);
    if ((j >= 0) && (j < NUM_LEDS))
      leds[j] += CHSV(hue+hOffset, 255, 255);
    if ((k >= 0) && (k < NUM_LEDS))
      leds[k] += CHSV(hue+2*hOffset, 255, 255);
    if ((l >= 0) && (l < NUM_LEDS))
      leds[l] += CHSV(hue+3*hOffset, 255, 255);

    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND); 
  }

// wait for LED trail to fade away at end
  dmax = 400;
  for(int i=0; i<dmax; i++) {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND);     
  }

} // end loop()
