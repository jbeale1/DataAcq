// Read 2 channels on JeeLabs Analog Plug (LM34 temp sensor)
// and print out with 4:1 ratio.  July 14 2017 j.beale
// 3.75 Hz sample rate = 267 msec period

#include <JeeLib.h>

PortI2C myI2C (1);  // was Port 3 for some reason
DeviceI2C adc (myI2C, 0x69);  // was 0x68 with A0,A1 open
                              // 0x69 with A0 shorted to left pad

#define DTIME 280    // milliseconds to wait for ADC
#define NAVG 8     // how many readings to average

static void AP2init (DeviceI2C& dev, byte mode =0x1C) {
    // default mode is channel 1, continuous, 18-bit, gain x1
    dev.send();
    dev.write(mode);
    dev.stop();
}

static long AP2read (DeviceI2C& dev) {
    dev.receive();
    long raw = (long) dev.read(0) << 16;
    raw |= (word) dev.read(0) << 8;
    raw |= dev.read(0);
    byte status = adc.read(1);
    return (raw * 1000) / 64;  // This in microvolt units
}

long snum = 0;  // sample number
long result;
long r1old = 0;
long r2old = 0;
//const byte c1mode = 0x1C + 0x02;  // ch1, cont, 18 bit, gain = x4
//const byte c2mode = 0x3C + 0x02;  // ch2, cont, 18 bit, gain = x4
const byte c1mode = 0x1C ;  // ch1, cont, 18 bit, gain = x1
const byte c2mode = 0x3C ;  // ch2, cont, 18 bit, gain = x1
const int samples = NAVG;   // how many readings to average
// ========================================================================

void setup () {
 Serial.begin(57600);
 Serial.println();
 Serial.println("snum, T1, dT1, T2, dT2");
 Serial.print("# AVG:");
 Serial.print(NAVG);
 Serial.println("  18-bit ADC, v1.0 15-JUL-2017 J.Beale");
 AP2init(adc, 0x1C); // chan 1, continuous, 18-bit, gain x1
 delay(300);
 r1old = rADC(c1mode, NAVG);
 r2old = rADC(c2mode, NAVG);
} // end setup()

// ========================================================================
// average together "samples" readings from ADC channel x
// Globals: adc, dtime
long rADC(byte mode, int samples) {

long raw;

 AP2init(adc, mode); // channel, continuous/oneshot, resolution, gain 
 long val = 0;
 for (int i=0; i < samples; i++) {
   delay(DTIME);
   raw = AP2read(adc);
   // Serial.print(ii/gain); Serial.print(" ");
   // val += (raw >> 2);  // because of the gain of 4
   val += (raw);  // add to running sum
 }
 return (val / samples);  // account for # of samples and return average
} // end rAADC1()


// ========================================================================
void loop () {
long val;
int i;

 Serial.print(snum); Serial.print(", ");
 result = rADC(c1mode, samples);
 Serial.print(result);  Serial.print(", ");  
 Serial.print(result-r1old);  Serial.print(", ");  
 r1old = result;
 
 result = rADC(c2mode, samples);
 Serial.print(result);  Serial.print(", ");
 Serial.print(result-r2old);  Serial.print(", ");  
 r2old = result;
 snum++;  // sample number
 
 Serial.println();
} // end loop()
