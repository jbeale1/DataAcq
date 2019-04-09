#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;   // TI 16-bit 4-ch ADC

const int LED = 13;  // output LED indicator

void setup(void)
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  // signal to user we are starting
  
  Serial.begin(115200);
  delay(3000);
  // Serial.println("  ADC1, range1, ADC2, range2");
  Serial.println("# ADS1115 Diff 01 channel 2019-03-31 JPB");
  digitalWrite(LED, LOW);
   
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

//  Config register bits 7:5  Data rate: 3 bits control the data rate setting.
// 000 : 8 SPS
// 001 : 16 SPS
// 010 : 32 SPS
// 011 : 64 SPS
// 100 : 128 SPS (default)
// 101 : 250 SPS
// 110 : 475 SPS
// 111 : 860 SPS

  
  ads.begin();
  ads.setGain(GAIN_ONE);
  // ads.startComparator_Differential(0, 1000);
  //  #define ADS1115_CONVERSIONDELAY         (8) // in milliseconds
}

void loop(void)
{
  int32_t sum1, amax1, amin1;
  int32_t sum2, amax2, amin2;
  int32_t samples = 25;      // 12 = 4.692 Hz, 25 = 2.252 Hz
  
  sum1 = 0;
  sum2 = 0;
  amin1 = 2<<16;
  amax1 = -amin1;
  amin2 = 2<<16;
  amax2 = -amin2;
  for (int i=0; i< samples; i++) {
    int16_t raw1 = ads.readADC_Differential_0_1();  
    if (raw1 > amax1) amax1 = raw1;
    if (raw1 < amin1) amin1 = raw1;
    sum1 += raw1;

    int16_t raw2 = ads.readADC_Differential_2_3();  
    if (raw2 > amax2) amax2 = raw2;
    if (raw2 < amin2) amin2 = raw2;
    sum2 += raw2;

  }
  float avg1 = (float) sum1 / samples; 
  float avg2 = (float) sum2 / samples; 
  digitalWrite(LED, HIGH);  // signal to user
  Serial.print(avg1,1);   Serial.print(",");
  Serial.print(amax1 - amin1);   Serial.print(",");
  Serial.print(avg2,1);   Serial.print(",");
  Serial.print(amax2 - amin2);
  Serial.println();
  digitalWrite(LED, LOW);
}
