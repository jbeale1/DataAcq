/* ADS1256, datasheet: http://www.ti.com/lit/ds/sbas288j/sbas288j.pdf
See also: https://github.com/Flydroid/ADS12xx-Library/blob/master/ads12xx.cpp
better style: https://github.com/baettigp/ADS12xx-Library/blob/master/ads12xx.cpp

connections to Atmega328 (UNO)

    CLK  - pin 13
    DIN  - pin 11 (MOSI)
    DOUT - pin 12 (MISO)
    CS   - pin 10 (or tie LOW)
    DRDY - pin 9
    RESET- pin 8 (or tie HIGH?)
    DVDD - 3V3

    Analog ch0 (input 0,1)
    Analog ch1 (input 2,3)
    Analog ch2 (input 4,5)
    (Analog ch3 (input 6,7))
*/

// #include "ADS1256.h"
// #include "ADS1256.cpp"

#include <SPI.h>

#define ADS_SPISPEED 1250000

#define ADS_RST_PIN    8 //ADS1256 reset pin
#define ADS_RDY_PIN    9 //ADS1256 data ready
#define ADS_CS_PIN    10 //ADS1256 chip select
// 11, 12 and 13 are taken by the SPI

//#define cs 10 // chip select
//#define rdy 9 // data ready, input
//#define rst 8 // may omit

#define U8 unsigned char
int nAvg = 10;
int i; // general use

#define VREF (2.495)   // measured voltage of ADC Vref signal

void initADS(){
  pinMode(ADS_CS_PIN, OUTPUT);
  digitalWrite(ADS_CS_PIN, HIGH); //unselect ADS

  pinMode(ADS_RDY_PIN, INPUT);
  pinMode(ADS_RST_PIN, OUTPUT);
  digitalWrite(ADS_RST_PIN, LOW);
  delay(10); // LOW at least 4 clock cycles of onboard clock. 100 microseconds is enough
  digitalWrite(ADS_RST_PIN, HIGH); // now reset to default values
  delay(150);

  digitalWrite(ADS_CS_PIN, LOW); // select ADS
  delayMicroseconds(50);
  while (digitalRead(ADS_RDY_PIN)) {}  // wait for ready_line to go low
  SPI.beginTransaction(SPISettings(ADS_SPISPEED, MSBFIRST, SPI_MODE1));
  delayMicroseconds(10);

  //Reset to Power-Up Values (FEh)
  SPI.transfer(0xFE);
  delayMicroseconds(100);

  byte status_reg = 0 ;  // STATUS Reg address (datasheet p. 30)
  // DRDY/ = b0 (read-only)
  // BUFF = b1  (0=no, 1=yes)
  // ACAL = b2  (0=no, 1=yes)
  // ORDER = b3  (0=MSB first, 1=LSB first)
  // byte status_data = 0x01; //status: Most Significant Bit First, Auto-Calibration Disabled, Analog Input Buffer Disabled
  // byte status_data = 0x05; //status: Most Significant Bit First, Auto-Calibration Enabled Analog Input Buffer Disabled
  byte status_data = 0x06; //status: MSB First, Auto-Calibration Enabled, Analog Input Buffer Enabled

  while (digitalRead(ADS_RDY_PIN)) {}  // wait for ready_line to go low
  SPI.transfer(0x50 | status_reg);
  SPI.transfer(0x00);   // 2nd command byte, write one register only
  SPI.transfer(status_data);   // write the databyte to the register
  delayMicroseconds(10);

  //PGA SETTING
  //1 ±5V        000 (1)
  //2 ±2.5V      001 (2)
  //4 ±1.25V     010 (3)
  //8 ±0.625V    011 (4)
  //16 ±312.5mV  100 (5)
  //32 ±156.25mV 101 (6)
  //64 ±78.125mV 110 (7) OR 111 (8)
  byte adcon_reg = 2; //A/D Control Register (Address 02h)
  byte adcon_data = 0x20; // 0 01 00 000 => Clock Out Frequency = fCLKIN, Sensor Detect OFF, gain 1
  //0x25 for setting gain to 32, 0x27 to 64
  while (digitalRead(ADS_RDY_PIN)) {}  // wait for ready_line to go low
  SPI.transfer(0x50 | adcon_reg);
  SPI.transfer(0x00);   // 2nd command byte, write one register only
  SPI.transfer(adcon_data);   // write the databyte to the register
  delayMicroseconds(10);
   
  //Set sampling rate
  byte drate_reg = 0x03; // Choosing Data Rate register = third register.
  // byte drate_data = 0b11000000; // 11000000 = 3,750SPS
  byte drate_data = 0b00000011; // 00000011 = 2.5SPS
  while (digitalRead(ADS_RDY_PIN)) {}  // wait for ready_line to go low
  SPI.transfer(0x50 | drate_reg);
  SPI.transfer(0x00);   // 2nd command byte, write one register only
  SPI.transfer(drate_data);   // write the databyte to the register
  delayMicroseconds(10);

  //done with settings, can close SPI transaction now
  digitalWrite(ADS_CS_PIN, HIGH); //unselect ADS
  SPI.endTransaction();
  delayMicroseconds(50);

  Serial.println("# ADS1256 configured");
}

long readADS(byte channel) {
  long adc_val = 0; // unsigned long is on 32 bits

  digitalWrite(ADS_CS_PIN, LOW);
  delayMicroseconds(50);
  SPI.beginTransaction(SPISettings(ADS_SPISPEED, MSBFIRST, SPI_MODE1)); // start SPI
  delayMicroseconds(10);
  //The most efficient way to cycle through the inputs is to
  //change the multiplexer setting (using a WREG command
  //to the multiplexer register MUX) immediately after DRDY
  //goes low. Then, after changing the multiplexer, restart the
  //conversion process by issuing the SYNC and WAKEUP
  //commands, and retrieve the data with the RDATA
  //command.
  while (digitalRead(ADS_RDY_PIN)) {} ;

  byte data = (channel << 4) | (1 << 3); //AIN-channel and AINCOM
  SPI.transfer(0x50 | 1); // write (0x50) MUX register (0x01)
  SPI.transfer(0x00);   // number of registers to be read/written − 1, write one register only
  SPI.transfer(data);   // write the data byte to the register
  delayMicroseconds(10);

  //SYNC command 1111 1100
  SPI.transfer(0xFC);
  delayMicroseconds(10);

  //WAKEUP 0000 0000
  SPI.transfer(0x00);
  delayMicroseconds(10);

  while (digitalRead(ADS_RDY_PIN)) {} ;

  SPI.transfer(0x01); // Read Data 0000  0001 (01h)
  delayMicroseconds(10);

  adc_val = SPI.transfer(0);
  adc_val <<= 8; //shift to left
  adc_val |= SPI.transfer(0);
  adc_val <<= 8;
  adc_val |= SPI.transfer(0);

  //The ADS1255/6 output 24 bits of data in Binary Two’s
  //Complement format. The LSB has a weight of
  //2VREF/(PGA(223 − 1)). A positive full-scale input produces
  //an output code of 7FFFFFh and the negative full-scale
  //input produces an output code of 800000h.
  if (adc_val > 0x7fffff) { //if MSB == 1
    adc_val = 16777216ul - adc_val; //do 2's complement, discard sign
  }
  
  delayMicroseconds(10);
   
  digitalWrite(ADS_CS_PIN, HIGH);
  delayMicroseconds(50);
  SPI.endTransaction();

  // Serial.print("Got measurement from ADS ");
  // Serial.println(adc_val);

  return adc_val;
}

long readADSDiff(byte positiveCh, byte negativeCh) {
  long adc_val = 0; // long is 32 bits

  digitalWrite(ADS_CS_PIN, LOW);
  delayMicroseconds(50);
  SPI.beginTransaction(SPISettings(ADS_SPISPEED, MSBFIRST, SPI_MODE1));
  delayMicroseconds(10);
  
  while (digitalRead(ADS_RDY_PIN)) {} ;

  byte data = (positiveCh << 4) | negativeCh; //xxxx1000 - AINp = positiveCh, AINn = negativeCh
  SPI.transfer(0x50 | 1); // write (0x50) MUX register (0x01)
  SPI.transfer(0x00);   // number of registers to be read/written − 1, write one register only
  SPI.transfer(data);   // write the databyte to the register
  delayMicroseconds(10);

  //SYNC command 1111 1100
  SPI.transfer(0xFC);
  delayMicroseconds(10);

  //WAKEUP 0000 0000
  SPI.transfer(0x00);
  delayMicroseconds(10);

  SPI.transfer(0x01); // Read Data 0000  0001 (01h)
  delayMicroseconds(10);

  adc_val = SPI.transfer(0);
  adc_val <<= 8; //shift to left
  adc_val |= SPI.transfer(0);
  adc_val <<= 8;
  adc_val |= SPI.transfer(0);

  delayMicroseconds(10);

  digitalWrite(ADS_CS_PIN, HIGH);
  SPI.endTransaction();
  delayMicroseconds(50);

  if (adc_val > 0x7fffff) { //if MSB == 1
    adc_val = adc_val - 0x01000000; //do 2's complement, keep the sign this time!
  }
  Serial.print("Got diff measurement from ADS ");
  Serial.println(adc_val);

  return adc_val;
}

#define NOP (0x00)
// for continuous reading, assumes config/setup already done
long readCont() {
  long adc_val = 0; // long is 32 bits
  
  while (digitalRead(ADS_RDY_PIN)) {} ;  // wait until data is ready
  // delayMicroseconds(10);

  digitalWrite(ADS_CS_PIN, LOW);
  SPI.transfer(0x01); // Read Data 0000  0001 (01h)

  U8 byte0=SPI.transfer(NOP); 
  U8 byte1=SPI.transfer(NOP);
  U8 byte2=SPI.transfer(NOP);
  adc_val = ((((long)byte0<<24) | ((long)byte1<<16) | ((long)byte2<<8)) >> 8); //as data from ADC comes in twos complement

  // delayMicroseconds(10);

  digitalWrite(ADS_CS_PIN, HIGH);
  SPI.endTransaction();
  delayMicroseconds(50);

  Serial.print("Got diff measurement from ADS ");
  Serial.println(adc_val);

  return adc_val;
} // readCont()

void setup()
{
  SPI.begin(); //start the spi-bus
  Serial.begin(115200);
  delay(500);
  Serial.println("sec,V"); // CSV header
  
  initADS();  // setup ADC registers
}

void loop()
{
  long adc_val; // store reading
  //Read x channels from ads1256
  int channel = 0;  // ADC channel
  float V = 0;  // measured signal in Volts
  for (i = 0; i < nAvg; i ++)
  {
    adc_val = readADS(channel);
    //Serial.print("   ");
    //Serial.println(adc_val);
    // Serial.print(i); Serial.print(": ");
    V += (float)adc_val * (VREF / 4194304.0 ); // 2^22 = 4194304
    // delay(500);
  }
  V /= (float) nAvg;
  Serial.print(millis()/1000,1);
  Serial.print(", ");
  Serial.print(V,6);
  Serial.println();
  // delay(1000);
}
