// Analog input test for Teensy 3.0    Oct 4 2012 - Feb 26 2014 J.Beale
// mods for testing Teensy 4 Beta 1  14-Feb-2019 J.Beale

#define VREF (3.292)         // ADC reference voltage (= power supply)
#define VINPUT (1.328)       // ADC input voltage from NiCd AA battery
//#define VINPUT (0.2991)       // ADC input voltage from resistive divider to VREF
// #define VINPUT (2.176)       // ADC input voltage from resistive divider to VREF
#define ADCMAX (4095)        // maximum possible reading from ADC
#define EXPECTED (ADCMAX*(VINPUT/VREF))     // expected ADC reading
#define SAMPLES (1000)      // how many samples to combine for pp, std.dev statistics

const int analogInPin = A0;  // Analog input is AIN0 (Teensy3 pin 14, next to LED)
const int LED1 = 13;         // output LED connected on Arduino digital pin 13

int sensorValue = 0;        // value read from the ADC input
long oldT;
long readingCount = 0;      // how many samples delivered so far

void setup() {    // ==============================================================
      pinMode(LED1,OUTPUT);       // enable digital output for turning on LED indicator
      // analogReference(EXTERNAL);  // set analog reference to internal ref
      analogReadRes(12);          // Teensy 3.0: set ADC resolution to this many bits
      analogReadAveraging(16);    // average this many readings
     
      Serial.begin(115200);       // baud rate is ignored with Teensy USB i/o
      digitalWrite(LED1,HIGH);   
      delay(1000);                // LED on for 1 second
      digitalWrite(LED1,LOW);    
      // delay(3000);             // wait in case serial monitor still opening
     
      Serial.println("# Teensy T4Beta1 ADC test 2019-02-15: ");
      Serial.println("count, avg, StDev, degC");
} // ==== end setup() ===========

void loop() {  // ================================================================ 
     
      long datSum = 0;  // reset our accumulated sum of input values to zero
      int sMax = 0;
      int sMin = 65535;
      long n;            // count of how many readings so far
      double x,mean,delta,sumsq,m2,variance,stdev;  // to calculate standard deviation
     
      oldT = millis();   // record start time in milliseconds

      sumsq = 0; // initialize running squared sum of differences
      n = 0;     // have not made any ADC readings yet
      mean = 0; // start off with running mean at zero
      m2 = 0;
     
      for (int i=0;i<SAMPLES;i++) {
        x = analogRead(analogInPin);
        datSum += x;
        if (x > sMax) sMax = x;
        if (x < sMin) sMin = x;
              // from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
        n++;
        delta = x - mean;
        mean += delta/n;
        m2 += (delta * (x - mean));
      } 
      variance = m2/(n-1);  // (n-1):Sample Variance  (n): Population Variance
      stdev = sqrt(variance);  // Calculate standard deviation


      long durT = millis() - oldT;
      float datAvg = (1.0*datSum)/n;
      readingCount++;
 /*
      Serial.print("# Samples/sec: ");
      Serial.print((1000.0*n/durT),2);
      Serial.print(" Avg: ");     Serial.print(datAvg,2);
      Serial.print(" Offset: ");  Serial.print(datAvg - EXPECTED,2);
      Serial.print(" P-P noise: ");  Serial.print(sMax-sMin);
      Serial.print(" St.Dev: ");  Serial.print(stdev,3);
      Serial.print(" T(deg.C): "); Serial.print(tempmonGetTemp(),1);
      */
      Serial.print(readingCount);  Serial.print(",");
      Serial.print(datAvg,2);      Serial.print(",");
      Serial.print(stdev,3);       Serial.print(",");
      Serial.print(tempmonGetTemp(),1);
      Serial.println();
} // end main()  =====================================================
