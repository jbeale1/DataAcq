// simple Teensy4 test program for AD7124
// J.Beale 8-Sep-2022

#include <SPI.h>  // include the SPI library

const int slaveSelectPin = 10;  
const int clockspeed = 1'000'000;

void setup() {
  Serial.begin(115200);
  pinMode (slaveSelectPin, OUTPUT);
  SPI.begin(); 
  SPI.beginTransaction(SPISettings(clockspeed, MSBFIRST, SPI_MODE3));
}

// length in bytes of each register in AD7124
uint8_t reglen[] = { 
  1,1,2,3,3,2,1,3,3,1,
  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
  2,2,2,2, 2,2,2,2,
  3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3
};
  

void loop() {
  long adc_raw;
  for (int reg=0; reg<0x38; reg++) {
    readReg(reg, reglen[reg], &adc_raw);
    Serial.print(reg, HEX);
    Serial.print(" : ");
    Serial.println(adc_raw, HEX);
    // delay(1000);
  }
  Serial.println(" -------------------- ");
  Serial.println();
  delay(5000);
}

void writeReg(int address, int value) {
  digitalWrite(slaveSelectPin,LOW);
  SPI.transfer(address);
  SPI.transfer(value);
  digitalWrite(slaveSelectPin,HIGH); 
}

// read up to 4 bytes from <address> register in AD7124
void readReg(int address, int bytes, long *word) {
  *word = 0;
  digitalWrite(slaveSelectPin,LOW);
  SPI.transfer(address | 0x40);
  for (int i=0; i<bytes; i++) {      
      uint8_t ret = SPI.transfer(0);
      *word = (*word)<<8 | ret;      
  }
  digitalWrite(slaveSelectPin,HIGH); 

}
