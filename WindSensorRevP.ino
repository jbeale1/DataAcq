// Read 4 channels on JeeLabs Analog Plug (LM34 temp sensor)
// 3.75 Hz sample rate = 267 msec period

// Sensor is ModernDevice Wind Sensor rev.P
// https://moderndevice.com/uncategorized/calibrating-rev-p-wind-sensor-new-regression/
// Temp_C = ( Vtemp - 0.400 ) / 0.0195
// ZeroWind_V = 1.35 V
// WS_MPH = (((Vout - ZeroWind_V) / (3.038517 * (Temp_C ^ 0.115157 ))) / 0.087288 ) ^ 3.009364

#include <JeeLib.h>

PortI2C myI2C (1);  // was Port 3 for some reason
DeviceI2C adc (myI2C, 0x69);  // was 0x68 with A0,A1 open
                              // 0x69 with A0 shorted to left pad

#define DTIME 280    // milliseconds to wait for ADC
#define NAVG 1     // how many single-channel readings to average at once
#define GAVG 4     // how many groups of CH channels to average (per channel)
#define CH 4       // how many analog channels to read

const float ZeroWind_V = 1.33;  // roughly, based on two sensors

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
long r1old = 0;
long r2old = 0;
//const byte c1mode = 0x1C + 0x02;  // ch1, cont, 18 bit, gain = x4
//const byte c2mode = 0x3C + 0x02;  // ch2, cont, 18 bit, gain = x4
// const byte c1mode = 0x1C ;  // ch1, cont, 18 bit, gain = x1

byte cmode[] = {0x1C, 0x3C, 0x5C, 0x7C};  // channel & mode select
const int samples = NAVG;   // how many readings to average
float fres[CH];  // CH channels of analog data
float fresOld[CH];  // previous reading of channel

//float fres1, fres2, fres1old, fres2old;
//float fres3, fres4, fres3old, fres4old;
// ========================================================================

void setup () {
 long res;
  
 Serial.begin(57600);
 Serial.println("snum, mph1, degC1, mph2, degC2");
 // Serial.print("# AVG:");
 // Serial.print(NAVG);
 // Serial.print(", GAVG:");
 // Serial.print(GAVG);
 // Serial.println("  18-bit ADC, v1.1 03-OCT-2018 J.Beale");
 
 AP2init(adc, cmode[0]); // chan 1, continuous, 18-bit, gain x1
 delay(300);

 for (int i=0;i<CH;i++) {
   res = rADC(cmode[i], samples);
   fresOld[i] = (float)res/1000.0;  // convert microvolts to mV
 }
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
long res;
int i,j;
float Vout, Vtemp, tempC, mph;

 for(i=0; i<CH; i++) { fres[i]= 0.0; }  // initialize sums to 0
 
 for(j=0; j<GAVG; j++) {   // average together this many loops
  for(i=0; i<CH; i++) {    // read each active channel
   res = rADC(cmode[i], samples);
   fres[i] += (float)res/1000.0;  // convert microvolts to mV
  }
 }

 Serial.print(snum);  // start line with index number

 for(i=0; i<CH; i+=2) {
   Vout = fres[i] / (GAVG * 1000);    // raw windspeed, volts
   Vtemp = fres[i+1] / (GAVG * 1000);  // raw temperature, volts
   tempC = ( Vtemp - 0.400 ) / 0.0195; // equation to get degrees C
   if (Vout < ZeroWind_V) {  // without noise/drift this should never happen
    mph=0.0;
   } else {
     mph = (((Vout - ZeroWind_V) / (3.038517 * (pow(tempC, 0.115157) ))) / 0.087288 );
     mph = pow(mph, 3.009364);
   }

   Serial.print(", ");
   Serial.print(mph,3);  Serial.print(", ");  
   Serial.print(tempC);   
 }
  
 snum++;  // sample number
 Serial.println();
} // end loop()
