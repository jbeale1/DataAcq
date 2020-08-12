// generate ~7 kHz pulses for sonar experiment
// output pulse is differential drive between pOut1 and pOut2
// cheap 2.5kHz - 45kHz piezo tweeter: Kemo Electronic P5123
// www.kemo-electronic.de/en/Car/Speaker/P5123-Mini-piezoelectric-tweeter-for-M094N.php
// resonant frequency depends on mounting and enclosure details
// use non-polar capacitor to avoid DC bias on transducer
// 11-AUG-2020 J.Beale

#include <digitalWriteFast.h>

// my particular tweeter resonance: 7.225 kHz = 138 usec period 

const int tQuiet=200; // how many milliseconds before next pulse

const int pOut1 = 8;  // to transducer + lead
const int pOut2 = 9;  // to transducer - lead

const uint8_t pMax = 6;
const uint8_t pMin = 1; // 1,10

uint8_t pwidth;  // microseconds of each high or low pulse
uint8_t pcount;  // how many drive pulses to send

uint8_t dwidth;  // duration of damping pulse
uint8_t dcount;       // how many damping pulses (@ 180 phase) to send
uint8_t d2width;       // delay before final damping pulse
uint8_t d3width;       // duration of final damping pulse

uint8_t di;    // index into duration array dur[]

boolean run = true;  // should we generate pulses right now?

#define dHi  digitalWriteFast(pOut1, HIGH); digitalWriteFast(pOut2, LOW)   // positive side
#define dLo  digitalWriteFast(pOut1, LOW); digitalWriteFast(pOut2, HIGH)   // positive side

#define DM delayMicroseconds
#define Wait delayMicroseconds(pwidth)
#define WaitD delayMicroseconds(dwidth)
#define WaitD2 delayMicroseconds(d2width)
#define WaitD3 delayMicroseconds(d3width)

// ======================================================================

void setup() {
  Serial.begin(9600);
  pinMode(pOut1, OUTPUT); // outputs to transducer
  pinMode(pOut2, OUTPUT);

// hand-tweaked values to minimize ring amplitude after train of 6 pulses
  pwidth = 69;  // drive pulse width
  dwidth = 74;  // damping pulse width
  d2width = 106;  // damping pulse width
  d3width = 31;  // damping pulse width
  pcount = 6;  // how many drive pulses to start with
  dcount = 2;     // how many damping pulses
  
  dLo;  // prepare output for max swing
  delay(1000);
  
  Serial.println("7 kHz pulse test 11-AUG-2020");
  Serial.println("u/d change drive pulse width");
  Serial.println("+/- change # of damping pulses");
  Serial.println("q/w change damping2 pulse width");
  Serial.println("e/r change damping3 pulse width");
  varPrint();
  
} // end setup()

// ======================================================================

void loop() {

    if (run)  // generate pulses if currently enabled
     { 
     noInterrupts();
     for (uint8_t i=0;i<pcount;i+=1) {
       dHi; Wait; dLo; Wait;
     }
     for (uint8_t i=0;i<dcount;i+=1) {
       WaitD; dHi; WaitD; dLo; // same freq, 180 out of phase to damp pulse faster
     }
     DM(d2width); dHi; DM(d3width); dLo; // another damping pulse
     DM(d2width+10); dHi; DM(d3width/3); dLo; // final damping pulse
     interrupts();
    } // if (run)
    
    delay(tQuiet);               // wait for a while

    KeyInput();                  // update parameters as requested
  
} // end loop()

// ======================================================================

void KeyInput() {  // provide for hand-tweaking of pulse params
  
byte Byte1;  // input char

  //process any keystrokes available
  if (Serial.available()>0) {
    //read the incoming byte
    Byte1 = Serial.read();
    if (Byte1>0x20) {
      switch (Byte1) {
      case 'u':  // up : increase delay period
        pwidth += 1;
        Serial.print("Drive period (us): ");
        Serial.println(pwidth);            
        break;
      case 'd':  // down : decrease delay period
        pwidth -= 1;
        Serial.println(pwidth);             
        break;
      case '+':  // more damping pulses
        dcount += 1;
        Serial.print("Damping pulses: ");
        Serial.println(dcount);             
        break;
      case '-':  // fewer damping pulses
        dcount -= 1;
        Serial.println(dcount);             
        break;
      case 'q':  // decrease damp delay
        d2width += 1;
        Serial.print("Damping period (us): ");
        Serial.println(d2width);             
        break;
      case 'w':  // increase damp delay
        d2width -= 1;
        Serial.println(d2width);             
        break;
      case 'e':  // decrease damp delay
        d3width += 1;
        Serial.print("Damping period (us): ");
        Serial.println(d3width);             
        break;
      case 'r':  // increase damp delay
        d3width -= 1;
        Serial.println(d3width);             
        break;
      case 'p': // print various variables
        varPrint();
        break;
      case 's': // stop/start
        run = ! run;
        break;
     }
    }
  }
}

void varPrint() {  // display various variables
        Serial.print("Drive (us):");
        Serial.print(pwidth);            
        Serial.print(" Damp1 (us):");
        Serial.print(dwidth);             
        Serial.print("  D2 (us):");
        Serial.print(d2width);             
        Serial.print("  D3 (us):");
        Serial.print(d3width);             
        Serial.print("  D pulses:");
        Serial.println(dcount);             
}
