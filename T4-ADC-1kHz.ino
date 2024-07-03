// Analog input Teensy 4     July 3 2024  J.Beale

#define VREF (3.292)         // ADC reference voltage (= power supply)
#define ADCMAX (4095)        // maximum possible reading from ADC
#define SAMPLES (177)        // for 1 kHz output rate

// 177 => 1000 output values / second
// 888 => 200 output values / second

const int analogInPin = A0;  // Analog input is AIN0 (Teensy3 pin 14, next to LED)
const int LED1 = 13;         // output LED connected on Arduino digital pin 13

// int sensorValue = 0;        // value read from the ADC input
long oldT;

void setup() {    // ==============================================================
      pinMode(LED1,OUTPUT);       // enable digital output for turning on LED indicator
      analogReadRes(12);          // set ADC resolution to this many bits
      analogReadAveraging(1);    // average this many readings
     
      Serial.begin(115200);       // baud rate is ignored with Teensy USB ACM i/o
      digitalWrite(LED1,HIGH);   delay(100);
      digitalWrite(LED1,LOW);    delay(300);
     
      Serial.println("# Teensy ADC test start. ");
      Serial.println("ADC, stdev");  // CSV file column headers
} // ==== end setup() ===========

void loop() {  // ================================================================ 
     
      long datSum = 0;  // reset our accumulated sum of input values to zero
      int sMax = 0;
      int sMin = 65535;
      long n;            // count of how many readings so far
      double x,mean,delta,m2,variance,stdev;  // to calculate standard deviation
     
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

      float datAvg = (1.0*datSum)/n;

      Serial.print(datAvg,1);
      Serial.print(",");
      Serial.println(stdev,2);
	  
} // end main()  =====================================================
