// Receive & Decode data from TX6U wireless temperature sensor for Arduino Uno
// Uses Bill Greiman's DigitalIO library https://github.com/greiman/DigitalIO
// sensor RF data format: http://www.f6fbb.org/domo/sensors/tx3_th.php

// Data signal is pair of 44-bit packets every ~57 seconds, 30 msec between the pair
// Uses 5V 433Mhz Superregeneration ASK Receiver ($8 on Amazon for Tx / Rx pair)
// Current draw of receiver is 6 mA from +5V
// Note: cannot use USB +5V power directly, due to high RF noise

#include "DigitalIO.h"

DigitalPin<2> SigIn;  // connect logic-level signal from 433.92 MHz receiver here
DigitalPin<13> LED;   // an output to signal pkg received

uint32_t tnow;  // current microsecond counter
uint32_t tlast; // timestamp of previous edge
uint32_t dt0, dt1;    // duration of the pulse just finished
uint32_t dt0_temp, dt1_temp;    // temp var before we've checked if this was just noise
uint32_t tooShortL = 25000;  // ignore low pulses shorter than this (microseconds)
uint32_t tooShortH = 1000;  // ignore high pulses shorter than this (microseconds)
uint32_t pCount = 0;         // total number of valid high pulses

uint64_t binpkt = 0x0LL;     // 64-bit integer is long enough to hold 44-bit packet

float ptOld = 0;              // time of last valid packet reception
// ===================================

// scan lowest 44 bits of 'var' and save as hex chars into string
void pHex44(uint64_t var, char * str) {
  byte scnt = 40;
  int i=0;
  uint64_t test = ((0x0fLL)<<40);
  for (; test>0; test >>= 4) {
    uint64_t slice = (var & test);
    byte b = slice >> scnt;
    byte ac; // ASCII character for hex value
    if (b < 0x0a) ac = b + 48;
    else  ac = b + 55;  // 0x0a + 55 = 65 = ASCII "A"
    str[i++] = ac;  // add this char to ASCII string
    scnt-= 4;  // step to next nybble in word
  }
}


void setup() {
//  Serial.begin(115200);
  Serial.begin(1000000);
  SigIn.mode(INPUT);
  LED.mode(OUTPUT);
  delay(500);
  
  Serial.println("# 433.92 MHz Temp Sensor RF data monitor 12-Nov-2018 jpb");
  while (SigIn == false);  // if input currently low, wait until it's high
  tlast = micros();
}


#define BUFSIZE 44   // bits in packet = # of pulses

boolean goodEdge;
uint8_t durL[BUFSIZE];  // low pulswidths
uint8_t durH[BUFSIZE];  // high pulsewidths
uint8_t bptr = 0;       // index into array of edge times dur[]
uint32_t goodpkts = 0;  // how many good packets

void loop() {

    // one pass through loop() corresponds to one bit of data (or one noise pulse)

    while (SigIn == true);  // spin until input becomes 0 (falling edge)
    tnow = micros();        // timestamp of falling edge
    dt1 = (tnow - tlast)/10;  // duration of high pulse
    durH[bptr] = min(dt1,255); // clamp value to fit in a byte
    tlast = tnow;

    while (SigIn == false);  // spin until input becomes 1 (rising edge)
    tnow = micros();         // timestamp of rising edge
    dt0 = (tnow - tlast)/10;    // duration of low pulse
    durL[bptr] = min(dt0,255);
    tlast = tnow;

    if (dt0 > 2800) {  // a long low interval is an end-of-packet signal
      readbin();
    }

    if (++bptr == BUFSIZE) bptr = 0;  // wrap around
  
}  // end main loop()  ==============================================

// =========================================================================
// readbin() : Scan through buffer of edge timings to interpret as data bits
// LOW time of 85-115 is valid spacing  (0.85 to 1.15 milliseconds)
// HIGH time 35-89 = logic "1", 90-150 = logic "0",  other = invalid

void readbin() {  // display interpretation of data in buffer as decoded bits
  boolean valid = true;  // start off optimistically
  byte good = 0;  // how many good bits in this run
  byte gmax = 0;  // max run of good bits
  byte b;         // used to hold each bit in packet
  binpkt = 0x0LL;     // 64-bit integer is long enough to hold entire binary packet
  
  for (int i=0; i < BUFSIZE; i++) {
    byte ip = i + bptr + 1;
    if (ip >= BUFSIZE) ip -= BUFSIZE;  // wrap around as needed
    
    byte dL = durL[ip];
    byte dH = durH[ip];
    if ((dL <= 115) && (dL >= 85) && (dH >= 35) && (dH <= 150)) {
      valid = true;
      good++;
      if (dH < 90) b=1; else b=0;
      binpkt = binpkt<<1 | b;      // shift bits into data word from LSB
    } else {
      if (valid) {   // last bit in valid packet?
        if ( (dL >= 85) && (dH >= 35) && (dH <= 150) && (good > 30) ) {  // valid high period?
          if (dH < 90) b=1; else b=0;
          binpkt = binpkt<<1 | b;      // shift bits into data word from LSB
          good++;  // count a valid final bit
        }
        valid = false;
        if (good > gmax) gmax = good;   // remember max good count
        good = 0;
      }  // if the previous bit was bad, no need for another note
    }
  }
  
  if (good > gmax) gmax = good;   // remember max good count
  if (gmax < 40)return;  // not looking like valid data
  
  goodpkts++;  // this was a good packet
  LED = true;  // visual signal
  float ptNow = millis()/1000.0;
  float ptDelta = ptNow - ptOld;
  ptOld = ptNow;
  Serial.print(ptNow,1);
  Serial.print(", ");  
  Serial.print(ptDelta,3);
  Serial.print(", ");  
  Serial.print(goodpkts);
  Serial.print(", ");
  Serial.print(gmax);  // number of valid bits in this packet
  Serial.print(", ");
  
  char str[12];   // buffer to hold received data as ASCII hex string
  pHex44(binpkt,str);
  str[11]='\0';  // don't forget to terminate string
  char deg[4];
  strncpy(deg,&str[5],3);  // these three digits = (degC + 50) * 10
  deg[3]='\0';  // null-terminate the string
  float degC = (atoi(deg) - 500) / 10.0;  // convert sensor's BDC digits to degrees C
  float degF = (degC * 1.8) + 32;
  // Serial.print(str);  // raw packet data
  // Serial.print(", ");
  Serial.print(degC,1);  // temperature in deg. C
  Serial.print(", ");
  Serial.print(degF,1);  // temperature in deg. F
  
  Serial.println();  
  LED = false;
}
