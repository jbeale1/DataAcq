// simple continuous-read test program for AD7124
// tested on Teensy 4.0
// J.Beale 9-Sep-2022

#include <SPI.h>  // uses the SPI library

const int CSPin = 10;              // goes to AD7124 CS/ pin
const int DRDY = 8;                // connect pin D8 to D12 externally
const int clockspeed = 2'000'000;  // SPI clock in Hz

// Size in bytes of each AD7124 on-chip register
uint8_t reglen[] = {
  1,2,3,3,2,1,3,3,1,  // control, data, and status regs
  2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,   // Channel_0 to _15
  2,2,2,2,2,2,2,2,                    // Config_0 to _7
  3,3,3,3,3,3,3,3,                    // Filter_0 to _7
  3,3,3,3,3,3,3,3,                    // Offset_0 to _7
  3,3,3,3,3,3,3,3                     // Gain_0   to _7
};

// ------------------------------------------------------------------
void resetChip();  // software reset of AD7124 chip
void readADReg(int address, int bytes, long *word); // read AD712 register
void readCont(long *word); // read in continuous mode, waits on DOUT/RDY*

void setup() {
  Serial.begin(115200);
  delay(5000);
  pinMode (CSPin, OUTPUT);
  pinMode(DRDY, INPUT);  // also SPI MISO; monitor for data ready
  SPI.begin(); 
  SPI.beginTransaction(SPISettings(clockspeed, MSBFIRST, SPI_MODE3));
  digitalWrite(CSPin,HIGH); 
  delay(1);
  resetChip();
  delay(1000);
}  

void loop() {
  long adc_raw;

  // AD7124 register FILTER_0 (0x21) controls sampling rate for Ch.0
  digitalWrite(CSPin,LOW);
  SPI.transfer(0x21); // FILTER_0 register
  SPI.transfer(0x06); // B2
  SPI.transfer(0x00); // B1
  SPI.transfer(0x01); // B0  FS:0x001 = 19.2 kHz
  digitalWrite(CSPin,HIGH);

  Serial.println();
  for (int reg=0; reg<0x38; reg++) {  // read out all registers
    readADReg(reg, reglen[reg], &adc_raw);
    Serial.print(reg, HEX);
    Serial.print(" : ");
    Serial.println(adc_raw, HEX);
    // delay(1000);
  }
  Serial.println(" -------------------- ");
  Serial.println();

  // set "Continuous Read" bit 
  digitalWrite(CSPin,LOW);
  SPI.transfer(0x01); // ADC control register (write)
  SPI.transfer(0x08); // high byte of ADC control register (CONT_READ bit set)
  SPI.transfer(0xc0); // low byte of ADC control register (high power mode)
  digitalWrite(CSPin,HIGH);

  delayMicroseconds(10);
  digitalWrite(CSPin,LOW);
  delayMicroseconds(5);

  // read out data at maximum possible speed
  for (int i=0;i<100;i++) {
    readCont(&adc_raw);
    Serial.println(adc_raw, HEX);
    digitalWrite(CSPin,HIGH);
    delayMicroseconds(2);
    digitalWrite(CSPin,LOW);
    delayMicroseconds(2);
  }
  
  resetChip();
  delay(10000);
  
} // end loop()

// =================================================================

// read up to 4 bytes from <address> register in AD7124
void readADReg(int address, int bytes, long *word) {
  *word = 0;
  digitalWrite(CSPin,LOW);
  SPI.transfer(address | 0x40);
  for (int i=0; i<bytes; i++) {      
      uint8_t ret = SPI.transfer(0);
      *word = (*word)<<8 | ret;      
  }
  digitalWrite(CSPin,HIGH); 
}

void resetChip() {  // send 64 clocks for software reset
  digitalWrite(CSPin,LOW);
  for (int i=0; i<8; i++) {      
      SPI.transfer(0xFF);
  }
  digitalWrite(CSPin,HIGH); 
}

// assumes that CS is already low to select chip
// read AD7124 Data in "ContinuousRead" mode, as soon as DRDY/ pin goes low
void readCont(long *word) {
  *word = 0;
  while (digitalRead(DRDY) == LOW) {  // wait if chip still ready from last time
      delayMicroseconds(1);
    }
  while (digitalRead(DRDY) == HIGH) {  // wait for chip to be ready
      delayMicroseconds(1);
    }
  
  for (int i=0; i<3; i++) {      // read out 3-byte DATA register
      uint8_t ret = SPI.transfer(0);
      *word = (*word)<<8 | ret;      
  }
}
