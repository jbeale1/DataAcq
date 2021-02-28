#include <Wire.h>
#include <MCP342x.h>

// Use Microchip MCP3421 to read TI LM35 temp sensor, average results, report reading in degrees C
// Works on Teensy 3.2
// J.Beale 2021-02-27

// 2^17 = 131072 = +2.048 V => 15.63 uV/count, 64000 counts/volt (Gain:1)
// Max temp @ Gain 1 = 204.8 C   (2: 102.4 C)  (4: 51.2 C) (8: 25.6 C)
// 18 bit mode, LM35:  counts per degree C

// 0x68 is the default address for all MCP342x devices
uint8_t address = 0x68;
MCP342x adc = MCP342x(address);

const int Gain=8;      // ADC input PGA gain
const float vpc = 1.0/(Gain * 64.0);  // mV per count
const float Offset = 1.219;  // measured sensor offset in deg.C

void setup(void)
{
  Serial.begin(9600);
  Wire.begin();

  MCP342x::generalCallReset();
  delay(1); // MC342x needs 300us to settle, wait 1ms
  
  Wire.requestFrom(address, (uint8_t)1);
  if (!Wire.available()) {
    Serial.print("No device found at address ");
    Serial.println(address, HEX);
    while (1)
      ;
  }

}

#define BUFSIZE 10  // how many readings to average
int Tbuf[BUFSIZE];  // holds temperature readings in raw ADC counts
int bp = 0;         // pointer into temperature buffer Tbuf[]
int printCounter = 0;  // how many samples since output
const int decimation = 3;    // print out every 'decimation' samples

void loop(void)
{
  long value = 0;
  MCP342x::Config status;

  uint8_t err = adc.convertAndRead(MCP342x::channel1, MCP342x::oneShot,
           MCP342x::resolution18, MCP342x::gain8,
           1000000, value, status);
  if (err) {
    Serial.print("Convert error: ");
    Serial.println(err);
  }
  else {
    Tbuf[bp] = value;
    bp = (bp+1) % BUFSIZE;
    float degC = 0;
    for (int i=0;i<BUFSIZE;i++)
      degC += Tbuf[i];

    printCounter = (printCounter+1) % decimation;
    if (printCounter == 0) {
      degC /= BUFSIZE; // convert sum to average
      degC = (degC * vpc/(10)) + Offset;  // convert ADC units to degrees
      Serial.println(degC,5);
    }
  }
  
  delay(1000);
}
