// simple continuous-read test program for AD7124
// tested on Teensy 4.0
// J.Beale 9-Sep-2022

#include <SPI.h>  // include the SPI library

const int slaveSelectPin = 10;  
const int DRDY = 8;
const int clockspeed = 2'000'000;

void resetChip();  // software reset of AD7124 chip

void setup() {
  Serial.begin(115200);
  delay(5000);
  pinMode (slaveSelectPin, OUTPUT);
  pinMode(DRDY, INPUT);  // also SPI MISO; monitor for data ready
  SPI.begin(); 
  SPI.beginTransaction(SPISettings(clockspeed, MSBFIRST, SPI_MODE3));
  digitalWrite(slaveSelectPin,HIGH); 
  delay(1);
  resetChip();
  delay(1000);
}

uint8_t reglen[] = {  // size in bytes of each on-chip register
  1,2,3,3,2,1,3,3,1,
  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
  2,2,2,2, 2,2,2,2,
  3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3
};
  

void loop() {
  long adc_raw;

  // set FILTER_0 (0x21) which controls sampling rate
  digitalWrite(slaveSelectPin,LOW);
  SPI.transfer(0x21); // FILTER_0 register
  SPI.transfer(0x06); // B2
  SPI.transfer(0x00); // B1
  SPI.transfer(0x01); // B0  FS:0x001 = 19.2 kHz
  digitalWrite(slaveSelectPin,HIGH);

  for (int reg=0; reg<0x38; reg++) {  // read out all registers
    readReg(reg, reglen[reg], &adc_raw);
    Serial.print(reg, HEX);
    Serial.print(" : ");
    Serial.println(adc_raw, HEX);
    // delay(1000);
  }
  Serial.println(" -------------------- ");
  Serial.println();


// set "Continuous Read" bit 
  digitalWrite(slaveSelectPin,LOW);
  SPI.transfer(0x01); // ADC control register (write)
  SPI.transfer(0x08); // high byte of ADC control register (CONT_READ bit set)
  SPI.transfer(0xc0); // low byte of ADC control register (high power mode)
  digitalWrite(slaveSelectPin,HIGH);

  delayMicroseconds(10);
  digitalWrite(slaveSelectPin,LOW);
  delayMicroseconds(5);
  
  for (int i=0;i<100;i++) {
    readCont(&adc_raw);
    Serial.println(adc_raw, HEX);
    digitalWrite(slaveSelectPin,HIGH);
    delayMicroseconds(2);
    digitalWrite(slaveSelectPin,LOW);
    delayMicroseconds(2);
  }
  resetChip();
  delay(10000);
  
} // end loop()

// =================================================================

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

void resetChip() {  // send 64 clocks
  digitalWrite(slaveSelectPin,LOW);
  for (int i=0; i<8; i++) {      
      SPI.transfer(0xFF);
  }
  digitalWrite(slaveSelectPin,HIGH); 
}


// assuming CS is already low,
// read AD7124 Data in "ContinuousRead" mode, as soon as DRDY/ pin goes low
void readCont(long *word) {
  *word = 0;
  while (digitalRead(DRDY) == LOW) {  // wait if chip still ready from last time
      delayMicroseconds(1);
    }
  while (digitalRead(DRDY) == HIGH) {  // wait for chip to be ready
      delayMicroseconds(1);
    }
  
  for (int i=0; i<3; i++) {      
      uint8_t ret = SPI.transfer(0);
      *word = (*word)<<8 | ret;      
  }
}
