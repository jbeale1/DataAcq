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


void loop() { 
unsigned int dmax;  // delay interval, msec

// rainbow is about 0..192
int pOffset = 18;  // LED spacing between pulses
int hOffset = 32;  // hue delta between pulse colors 
int fRate = 45;    // fade rate per frame, % as fraction of 255
uint8_t B;         // overall LED brightness setting (out of 255)
int k;             // index for LED array


hue = 0;   // 0 = red,64=yellow, 96 = green,160=blue, 192 = violet

B = 50;
LEDS.setBrightness(B);

for (int n=1; n<8; n++) { // loop over # of groups
  for (int i=0; i<(NUM_LEDS + n*pOffset); i++) { // over pixels
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    for (int j=0; j<n; j++) {  // for each color group
      if (n%2==0) // is n even?
        k = (NUM_LEDS + j*pOffset) - i;
      else // n is odd   
        k = i - j*pOffset;
        
      if ((k >= 0) && (k < NUM_LEDS)) 
        leds[k] = CHSV(hue + j*hOffset, 255, 255);
    }
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

}
// ======================================

// red pulses from ends meet in middle
 for (int i=0; i<(NUM_LEDS/2 - 1); i++) { // over 1/2 of pixels
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    leds[i] = CHSV(hue, 255, 255);  // start from beginning
    leds[(NUM_LEDS-1)-i] = CHSV(hue, 255, 255); // start from end
    FastLED.show();
    FastLED.delay(4000/FRAMES_PER_SECOND); 
 }

int d=16; // diameter of sparks blob
// bright pulsing blob in center
int br=245; // brightness of main spark
for (int i=0; i<(FRAMES_PER_SECOND/3); i++) {
  fadeToBlackBy( leds, NUM_LEDS, fRate);
  int off = beatsin8(240, 0, 4);  // BPM, min, max
  int mid = NUM_LEDS/2;
  br += (random8(10)-8);
  br = max(0,br);
  leds[mid+off] += CRGB(br,br,br);
  leds[mid-off] += CRGB(br,br,br);
  if (random8(6)==0)
    leds[mid+d/2-random8(d)] += CHSV(hue+random8(64),20+random8(200),128+random8(64));
  if (random8(6)==0)
    leds[mid+d/4-random8(d/2)] += CHSV(hue+random8(64),20+random8(200),180+random8(75));
  FastLED.show();
  FastLED.delay(1000/FRAMES_PER_SECOND); 
}

// yellow pulses escape from middle
hue = 64; // supposedly yellow in color
for (int i=(NUM_LEDS/2 - 1); i >= 0; i--) { // over 1/2 of pixels
    fadeToBlackBy( leds, NUM_LEDS, fRate);
    leds[i] = CHSV(hue, 255, 255);  // start from beginning
    leds[(NUM_LEDS-1)-i] = CHSV(hue, 255, 255); // start from end
    FastLED.show();
    FastLED.delay(4000/FRAMES_PER_SECOND); 
 }




// wait for LED trail to fade away at end
  dmax = 250;
  for(int i=0; i<dmax; i++) {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND);     
  }

} // end loop()
