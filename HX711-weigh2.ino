
// based on https://github.com/RobTillaart/HX711

// read two HX711 devices
// 23-July-2023 J.Beale

#include "HX711.h"

HX711 scale1;
HX711 scale2;

uint8_t dat2 = 2; // pins going to HX711 #1
uint8_t clk2 = 3;

uint8_t dat1 = 4; // pins connecting to HX711 #2
uint8_t clk1 = 5;

float scalefac2 = - 1.0/475.4; // grams per ADC count on 5kg load cell
float scalefac1 =  1.0/220.2; // grams per ADC count on 10kg load cell
float tempScale1 = 1/1E6;
float tempScale2 = 1/1E6;

uint32_t start, stop;
float tareA = 0;
float tareB = 0;
float tare2A = 0;
float tare2B = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  // Serial.print("LIBRARY VERSION: ");
  // Serial.println(HX711_LIB_VERSION);
  // Serial.println();

  scale1.begin(dat1, clk1);
  scale2.begin(dat2, clk2);

  float sum=0;
  int loops = 10;
  for (int i = 0; i < loops; i++)
  {
    sum += scale1.read_medavg(7);
  }
  tareA = sum / loops;
  Serial.print("Tare1A = ");
  Serial.println(tareA);

  scale1.set_gain(HX711_CHANNEL_B_GAIN_32, true);
  tareB = scale1.read_average(5); // average together this many readings

  sum=0;
  loops = 10;

  for (int i = 0; i < loops; i++)
  {
    sum += scale2.read_medavg(7);
  }
  tare2A = sum / loops;
  Serial.print("Tare2A = ");
  Serial.println(tare2A);

  scale2.set_gain(HX711_CHANNEL_B_GAIN_32, true);
  tare2B = scale2.read_average(5); // average together this many readings

}


void loop()
{
  float f,t,grams1,grams2;
  float temp1,temp2;

  scale1.set_gain(HX711_CHANNEL_A_GAIN_128, true);
  delay(200);
  f = scale1.read_average(25); // average together this many readings
  grams1 = (f - tareA) * scalefac1;
  Serial.print(grams1);
  Serial.print(", ");

  scale1.set_gain(HX711_CHANNEL_B_GAIN_32, true);
  delay(200);
  t = scale1.read_average(5); // average together this many readings
  temp1  =  - ((t-tareB) * tempScale1);
  Serial.print(temp1,4);
  Serial.print(", ");

  scale2.set_gain(HX711_CHANNEL_A_GAIN_128, true);
  delay(200);

  grams2 = (scale2.read_average(25) - tare2A) * scalefac2;
  Serial.print(grams2);
  Serial.print(", ");

  scale2.set_gain(HX711_CHANNEL_B_GAIN_32, true);
  delay(200);
  t = scale2.read_average(5); // average together this many readings
  temp2  =  - ((t-tare2B) * tempScale2);
  Serial.println(temp2,4);

}
