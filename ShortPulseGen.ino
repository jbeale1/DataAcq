// generate ~7 kHz pulses for sonar experiment
// output is differential between pOut1 and pOut2
// use non-polar capacitor to avoid DC bias on transducer
// 10-AUG-2020 J.Beale

#include <digitalWriteFast.h>

#define DCOUNT 5

// resonance with tweeter open to air: 7.11 kHz = 140 usec period 
// in pipe, resonant freq. goes up (closer to 7.8 kHz, 128 usec period)
const uint8_t dur[DCOUNT]={52, 56, 60, 64, 67};  // microseconds of half period

const int tQuiet=200; // how many milliseconds before next pulse

const int pOut1 = 8;  // to transducer + lead
const int pOut2 = 9;  // to transducer - lead

const uint8_t pMax = 6;
const uint8_t pMin = 1; // 1,10
uint8_t pcount;
uint8_t di;    // index into duration array dur[]

#define dHi  digitalWriteFast(pOut1, HIGH); digitalWriteFast(pOut2, LOW)   // positive side
#define dLo  digitalWriteFast(pOut1, LOW); digitalWriteFast(pOut2, HIGH)   // positive side
#define Wait delayMicroseconds(dur[di])

void setup() {
  // initialize the digital pin as an output.
  pinMode(pOut1, OUTPUT);
  pinMode(pOut2, OUTPUT);
  pcount = pMin;  // how many pulses to start with
  dLo;  // prepare output for max swing
} // end setup()


void loop() {

 for (di=0;di<DCOUNT;di++) {  // loop over pulse frequencies
  
    pcount = pMin;  // short pulse
    noInterrupts();
    for (uint8_t i=0;i<pcount;i+=1) {
      dHi; Wait; dLo; Wait;
    }
    Wait; dHi; Wait; dLo; // same freq, 180 out of phase to damp pulse faster
    interrupts();
    delay(tQuiet);               // wait for a while

    pcount = pMax; // longer pulse
    noInterrupts();
    for (uint8_t i=0;i<pcount;i+=1) {
      dHi; Wait; dLo; Wait;
    }
    Wait; dHi; Wait; dLo; // same freq, 180 out of phase to damp pulse faster
    Wait; dHi; Wait; dLo;
    interrupts();
    delay(tQuiet);               // wait for a while
  
 delay(tQuiet*1.5);               // wait for a while
 } // for(di...)

 delay(tQuiet*2);               // wait for a longer while

} // end loop()
