// Read Plantower PMS5003 air quality sensor serial port
// Based on learn.adafruit.com/pm25-air-quality-sensor/arduino-code
// averages together ACOUNT separate readings
// J.Beale 5-July-2020

#include <SoftwareSerial.h>

//#define pmsSerial Serial1  // use the hardware serial port
SoftwareSerial pmsSerial(2, 3); // (Rx, Tx)
 
const int ledPin =  13;      // the number of the LED pin

void setup() {
  // our debugging output
  Serial.begin(115200);
  delay(2000);
  pinMode(ledPin, OUTPUT);
  Serial.println("PM1,PM25,PM100,0p3,0p5,1p0,2p5,5p0,10p0");
  Serial.println("# PMS5003 Sensor Reading v2  2020-07-05 JPB");
 
  // sensor baud rate is 9600
  pmsSerial.begin(9600);
}
 
struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
 
struct pms5003data data;

#define ACOUNT 16      // how many readings to average together
#define DCOUNT 9       // how many separate data elements we're tracking
uint32_t adat[DCOUNT];
    
void loop() {
int i;

  for (i=0;i<DCOUNT;i++) {
    adat[i]=0;
  }

  for (i=0; i<ACOUNT; i++) {
    digitalWrite(ledPin, HIGH);  
    delay(10); // wait for a new updated reading
    digitalWrite(ledPin, LOW);  
    delay(2990); // wait for a new updated reading
    do {} while (!readPMSdata(&pmsSerial));  // read until we get a valid sentence
    adat[0] += data.pm10_standard;
    adat[1] += data.pm25_standard;
    adat[2] += data.pm100_standard;
    adat[3] += data.particles_03um;
    adat[4] += data.particles_05um;
    adat[5] += data.particles_10um;
    adat[6] += data.particles_25um;
    adat[7] += data.particles_50um;
    adat[8] += data.particles_100um;
  }

  for (i=0; i<DCOUNT;i++) {
    Serial.print(int(adat[i]/ACOUNT));
    if (i < (DCOUNT-1)) Serial.print(",");
  }
  Serial.println();

} // end main loop()

 // ==================================================================
 
boolean readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }
  
  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }
 
  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }
    
  uint8_t buffer[32];    
  uint16_t sum = 0;
  s->readBytes(buffer, 32);
 
  // get checksum ready
  for (uint8_t i=0; i<30; i++) {
    sum += buffer[i];
  }
 
  
  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i=0; i<15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }
 
  // put it into a nice struct :)
  memcpy((void *)&data, (void *)buffer_u16, 30);
 
  if (sum != data.checksum) {
    // Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

/*
    Serial.println("Concentration Units (standard)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_standard);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (environmental)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_env);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_env);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_env);
    Serial.println("---------------------------------------");
    Serial.print("Particles > 0.3um / 0.1L air:"); Serial.println(data.particles_03um);
    Serial.print("Particles > 0.5um / 0.1L air:"); Serial.println(data.particles_05um);
    Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
    Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
    Serial.print("Particles > 5.0um / 0.1L air:"); Serial.println(data.particles_50um);
    Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
    Serial.println("---------------------------------------");
*/
