// Use MCP3424 via I2C to read output voltage on MQ-137 ammonia gas sensor
//   Using original-style ATmega328p Arduino
//   J.Beale  Sunday March 31 2024

#include <Wire.h>
#include <MCP342x.h>

// 0x68 is the default address for all MCP342x devices
uint8_t address = 0x68;
MCP342x adc = MCP342x(address);

#define MQ_ON  digitalWrite(9, LOW)
#define MQ_OFF  digitalWrite(9, HIGH)


void setup(void)
{
  Serial.begin(115200);
  Wire.begin();

  // Enable power for MQ-137 gas sensor
  pinMode(9, OUTPUT);
  MQ_OFF;
  
  // Reset devices
  MCP342x::generalCallReset();
  delay(1); // MC342x needs 300us to settle, wait 1ms
  
  // Check device present
  Wire.requestFrom(address, (uint8_t)1);
  if (!Wire.available()) {
    Serial.print("No device found at address ");
    Serial.println(address, HEX);
    while (1)
      ;
  }

}

// average together 'avg' readings from ADC channel
float readMQ(MCP342x::Channel ch, int avg) {
  float sum = 0;
  long value = 0;

  for (int i=0;i<avg;i++) {
    MCP342x::Config status;
    // Initiate a conversion; convertAndRead() will wait until it can be read
    uint8_t err = adc.convertAndRead(ch, MCP342x::oneShot,
            MCP342x::resolution18, MCP342x::gain1,
            1000000, value, status);
    if (err) {
      Serial.print("Convert error: ");
      Serial.println(err);
    }
    else {
      float mV = value * (40.0 / 2564);
      sum += mV;
      // Serial.print("Value: ");
    }
  }
  return (sum/avg);
}

void loop(void)
{
  float mV;  // millivolts from sensor
  int avg = 40;  // how many readings to average over
  MCP342x::Channel ch = MCP342x::channel1;

  MQ_ON;
  delay(19000); // preheat for 20 sec

  mV = readMQ(ch, avg);  // dummy read
  long int tNow = millis();
  float min = tNow / 60000.0;
  Serial.print(min,1);
  Serial.print(",");
  mV = readMQ(ch, avg);
  Serial.println(mV,4);
  MQ_OFF;

  int minI = min;
  long int d1;
  d1 = ((minI+5)*60000) - millis();
  if (d1 > 0) delay(d1);
}
