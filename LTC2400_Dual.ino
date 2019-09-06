/*
 Interface Teensy 3.2 with two Linear Tech LTC2400 24-bit ADCs (7.5 Hz sample rate)
 by J.Beale  Sept 5, 2019

 Signal  ADC    Teensy3.2
 --------------------------------
 CS:     pin 5  pin 9,10 
 MISO:   pin 6  pin 12   
 SCK:    pin 7  pin 13   
 ---------------------
 */

#include <SPI.h> // standard SPI library

#define VREF1 (4.68)     // reference voltage
#define VREF2 (3.3)     // reference voltage
#define SAMPLES (1)   // how many samples to group together for stats

#define sign(x) ((x >= 0) ? 1 : -1)
#define ESIZE 7

const int CSpin1 = 10;  // CS for first device
const int CSpin2 = 9;   // CS for second device

const int SDO = 12;    // SPI Input line (high->low = data ready)

unsigned int cnt;    // simple loop counter
unsigned long int raw;  // raw ADC value
double v1,v2,v11,v22,offset1,offset2;  // ADC values and offsets
float soff = +0.01;    // offset of summed signal display
float doff = +0.02;    // offset of difference signal display
float sm1, sm2;        // low-pass-filtered versions of ch1, ch2
float ff = 0.01;       // low-pass filter smoothing constant
boolean act, event, last, last2;  //  if this sample crossed significant activity threshold
unsigned int ehist[ESIZE];  // sample history of events
unsigned int eptr = 0;       // pointer into ehist[]

void setup() {
  Serial1.begin(115200);
  SPI.begin();
  pinMode(CSpin1, OUTPUT);
  pinMode(CSpin2, OUTPUT);
  digitalWrite(CSpin1, HIGH);  // deselect device 1
  digitalWrite(CSpin2, HIGH);  // deselect device 2
  cnt = 0;
  
  // delay(1000);  // because Windows doesn't notice the serial port that fast
  raw = readWord(CSpin1);  // warmup reading
  delay(150);
  raw = readWord(CSpin2);  // warmup reading

  delay(150);
  const int offset_samples = 10;
  offset1 = 0;
  offset2 = 0;
  for (int i=0;i<offset_samples;i++) {
    delay(1);
    offset1 += (double) readWord(CSpin1) * VREF1 / (1<<28);  // analog reading in units of Volts
    delay(1);
    offset2 += (double) readWord(CSpin2) * VREF2 / (1<<28);  // analog reading in units of Volts
  }
  offset1 = -(offset1 / offset_samples);
  offset2 = -(offset2 / offset_samples);
  sm1 = 0;
  sm2 = 0;
  act = event = last = false;  //  if this sample crossed significant activity threshold
  
}


void loop() {  // **** main loop

unsigned long int raw1, raw2;    
float eOut;            
            
  v1 = readWord(CSpin1) * VREF1 / (1<<28) + offset1;  // analog reading in units of Volts
  v2 = readWord(CSpin2) * VREF2 / (1<<28) + offset2;  // analog reading in units of Volts

  sm1 = (1.0-ff)*sm1 + ff*v1;
  sm2 = (1.0-ff)*sm2 + ff*v2;
  v11 = v1 - sm1;
  v22 = v2 - sm2;

  float sum2 = (v11+v22);  // sum of the two sensors
  float diff2 = (v11-v22);  // difference of the two sensors
  float msum = sign(sum2) * max( 0, abs(sum2) - 1.75*abs(diff2) );  // "significant" amount by which sum exceeds difference
  if (abs(msum) > 0.0005) act = true; else act = false;  // significant activity threshold for this sample
  ehist[eptr] = floor(abs(msum)*2000);  // cast float to int

  unsigned int esum = 0;
  for (int i=0;i<ESIZE;i++) {  // add up ESIZE most recent samples
    esum += ehist[i];    
  }
  eOut = 0.01 + esum*.001;  // display scaling. reasonable threshold = 6..8 ?
  
  Serial1.print(v11,6);
  Serial1.print(",");
  Serial1.print(v22,6);
  //Serial1.print(",");
  //Serial1.print(sum2+soff,6);
  //Serial1.print(",");
  //Serial1.print(diff2+doff,6);
  //Serial1.print(",");
  //Serial1.print(msum+doff+soff,6);
  Serial1.print(",");
  Serial1.print(eOut,4);
  Serial1.println();

  if (++eptr >= ESIZE) eptr = 0;
  
  // last2 = last;
  last = act;  

} // end loop()

// ===========================================================================
// Read data from Linear Tech LTC2400  (24-bit ADC) using SPI
// The data word is actually 28 bits, although lowest 4 bits are noise
// parameter is the chip-select pin for the device to read

unsigned long int readWord(int CSpin) {
  byte inByte = 0;           // incoming byte from the SPI
  byte bytesToRead = 4;      // read this many bytes from SPI
  unsigned long int result = 0;   // result to return
  boolean EXR = 0;
  boolean SGN = 0;

  // LTC2400 SPI clock can go up to 2 MHz
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  delay(1);
  digitalWrite(CSpin, LOW);

  while (digitalRead(SDO) == 1) {}   // wait for MISO to fall low => conversion ready

  result = SPI.transfer(0x00);
  SGN = result & 0x20;           // Sign bit
  EXR = result & 0x10;           // Extended Range bit
  result &= 0x0f;                // mask off high 4 bits

  for (byte i=0; i<3; i++) {     // read remaining 3 bytes of 32-bit output word
    result <<= 8;
    result |= SPI.transfer(0x00);
  }
  digitalWrite(CSpin, HIGH);  // put chip in sleep mode
  SPI.endTransaction();  // release control of SPI port

  return(result);  // return 28-bit ADC result
}  // readWord()
