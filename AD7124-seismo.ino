// Continuous read mode on Analog Devices AD7124-8 24-bit ADC
// Teensy 4.0 and EVAL-AD7124-8-PMDZ board
// Signal from seismometer coil on ADC (AIN0,AIN1) pair
//
// RMS noise @ full power, Gain = 1, SINC4 filter, 10 ohm source impedance, measured:
// FS: 1284  15  Hz: 1.6 counts (23.3 bits RMS resolution)
// FS: 321   60  Hz: 1.9 counts (23 bits RMS resolution)
// FS: 64   301  Hz: 4.0 counts (22 bits RMS resolution)
// FS: 15  1.28 kHz: 8.3 counts (21 bits RMS resolution)
// FS: 1   19.2 kHz: 233 counts (16 bits RMS resolution)
//
// by J.Beale 10-Sep-2022

#include <SPI.h>  // uses the SPI library

const int CSPin = 10;              // goes to AD7124 CS/ pin
const int DRDY = 8;                // connect Teensy4 pin D8 to pin D12 
const int clockspeed = 2'000'000;  // SPI clock in Hz

// Size in bytes of each of the 57 addressable on-chip registers
uint8_t reglen[] = {
  1,2,3,3,2,1,3,3,1,  // control, data, and status regs
  2,2,2,2,2,2,2,2,                    // Channel_0 to _7
  2,2,2,2,2,2,2,2,                    // Channel_8 to _15
  2,2,2,2,2,2,2,2,                    // Config_0 to _7
  3,3,3,3,3,3,3,3,                    // Filter_0 to _7
  3,3,3,3,3,3,3,3,                    // Offset_0 to _7
  3,3,3,3,3,3,3,3                     // Gain_0   to _7
};

// ------------------------------------------------------------------
void resetChip();                             // software reset of AD7124 chip
void showReg();                               // print out all chip registers
void setFilterRate(int chan, int fs);         // set Ch0..7 sampling frequency
void readADReg(int addr, int n, long *word);  // read AD7124 register
void readCont(long *word);      // read in continuous mode, waits on DOUT/RDY*

// ------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode (CSPin, OUTPUT);
  pinMode(DRDY, INPUT);  // also SPI MISO; monitor for data ready
  SPI.begin(); 
  SPI.beginTransaction(SPISettings(clockspeed, MSBFIRST, SPI_MODE3));
  digitalWrite(CSPin,HIGH); 
  delay(1);
  resetChip();
  delay(1000);

  digitalWrite(CSPin,LOW);  // seg PGA gain for Ch.0 (at reset: CONFIG_n = 0x0860)
  SPI.transfer(0x19); // CONFIG_0 ADC register (write)
  SPI.transfer(0x08); // default Bipolar, Burnout, REF_BUFP
  SPI.transfer(0x62); // default REF_BUFM,AIN_BUF,REF_SEL, PGA=4x
  digitalWrite(CSPin,HIGH);

}  

void loop() {
  double lpfilt = -200; // low-pass filtered signal
  double lp2 = 0;    // 2nd low-pass filtered signal
  double lp3 = 0;    // 2nd low-pass filtered signal
  double f = 0.0001;  // low-pass filter averaging fraction
  double f2 = 0.1;   // second low-pass filter fraction
  double f3 = 0.1;   // third low-pass filter fraction
  long adc_raw;      // raw ADC reading

  // AD7124 register FILTER_0 (0x21) low 11 bits controls sampling rate for Ch.0
  //  FS:1=19.2 kHz 15=1.28 kHz 64=301 Hz 2047 = 9.41 Hz
  int channel = 0;  // ADC channel [0..7]
  // int fs = 321;  // sampling rate word  (1 = 19.2 kHz, 321 = 60 Hz)
  int fs = 193;   // FS:193 => ~100 Hz
  // int fs = 15;      // FS:15 => 1.28 kHz sample rate
  
  setFilterRate(channel, fs);  // set sampling rate for this channel
  

  // set bits for "Continuous Read" and "2.5V Internal Reference Enable"
  digitalWrite(CSPin,LOW);
  SPI.transfer(0x01); // ADC control register (write)
  SPI.transfer(0x08 | 0x01); // (CONT_READ bit set, REF_EN bit set)
  SPI.transfer(0xc0); // low byte of ADC control register (high power mode)
  digitalWrite(CSPin,HIGH);

  delayMicroseconds(10);
  digitalWrite(CSPin,LOW);
  delayMicroseconds(5);

  // read out data at maximum possible speed
  while (true) {
    readCont(&adc_raw);
    adc_raw &= 0xffffff;
    long int adc = adc_raw - 0x800000; // 24-bit 2's complement value
    lpfilt = (1.0-f)*lpfilt + (f*adc);
    double  cVal = adc - lpfilt;  // long-term avg value is 0
    lp2 = (1.0-f2)*lp2 + (f2 * cVal);
    lp3 = (1.0-f3)*lp3 + (f3 * lp2);
    long sigProc = lp3;
    Serial.println(sigProc);
      

    digitalWrite(CSPin,HIGH);
    delayMicroseconds(2);
    digitalWrite(CSPin,LOW);
    delayMicroseconds(2);
  }
    
} // end loop()

// =================================================================

void resetChip() {  // send 64 clocks for software reset
  digitalWrite(CSPin,LOW);
  for (int i=0; i<8; i++) {      
      SPI.transfer(0xFF);
  }
  digitalWrite(CSPin,HIGH); 
}

// print out all the registers on the chip
void showReg() {
  long adc_raw;

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
} 

void setFilterRate(int chan, int fs) {
  uint8_t b1,b0; // bytes to send to ADC

  b0 = fs & 0xff;
  fs = fs>>8;
  b1 = fs & 0xff;
  digitalWrite(CSPin,LOW);

  chan &= 0x07;  // force channel number to valid range [0..7]
  SPI.transfer(0x21+chan); // FILTER_0 register
  SPI.transfer(0x06); // B2
  SPI.transfer(b1); // B1
  SPI.transfer(b0); // B0
  digitalWrite(CSPin,HIGH);
}

// read up to 4 bytes from register <address> in AD7124
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

// assumes that CS is already low to select chip
// read AD7124 Data in "ContinuousRead" mode, as soon as DRDY/ pin goes low
void readCont(long *word) {
  *word = 0;
  while (digitalRead(DRDY) == LOW) {  // wait if chip still ready from last cycle
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
