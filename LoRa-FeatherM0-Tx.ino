// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

// Testing with higher spreading factor. 8-SEP-2021 J.Beale
// myconfig = { 0x72, 0x84, 0x00 }; // 125 kHz BW, SF=8
//    tested working all around the block, 1/4 wave vert.  11-SEP-2021

#include <SPI.h>
#include <RH_RF95.h>

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  pinMode(LED_BUILTIN, OUTPUT);  // enable onboard LED
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  Serial.begin(115200);  
  //while (!Serial) {
  //  delay(1);
  //}

  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH); 
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  
  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Registers 0x1D, 0x1E, 0x26 are the three "Modem Configuration" regs
  // default working: {0x72, 0x74, 0x00}
  // RH_RF95::ModemConfig myconfig = { RH_RF95_BW_125KHZ | RH_RF95_CODING_RATE_4_5, RH_RF95_SPREADING_FACTOR_1024CPS, 0x04};
  //RH_RF95::ModemConfig myconfig = { 0x72, 0xa4, 0x00 }; // default, but SF=10 (works)
  //RH_RF95::ModemConfig myconfig = { 0x62, 0x84, 0x00 }; // 62.5 kHz BW, SF=8 (spec.test)
  RH_RF95::ModemConfig myconfig = { 0x72, 0x84, 0x00 }; // 125 kHz BW, SF=8 (spec.test)
  

  //RH_RF95::ModemConfig myconfig = { 0x62, 0xa4, 0x00 }; // default, but BW=62.5 kHz, SF=10
  rf95.setModemRegisters(&myconfig);  // setup LoRa to optimal values for our current purposes
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(18, false);
  //rf95.setTxPower(0, false);
  delay(500);  // DEBUG delay for serial port ready
  rf95.printRegisters();  // DEBUG see what current config is
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop()
{
  delay(1500); // Wait x seconds between transmits, could also 'sleep' here!
  Serial.println("Transmitting..."); // Send a message to rf95_server

  const int pktlen = 10; // total bytes in packet
  const int ph1 = 4;  // bytes in first part of packet
  //                           1234567890123456789
  char radiopacket[pktlen] = "JPB#     ";
  itoa(packetnum++, radiopacket+ph1, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[pktlen-1] = 0;  // is null-termination required?
  
  Serial.print("Sending: ");
  Serial.println(radiopacket);
  delay(10);
  rf95.send((uint8_t *)radiopacket, pktlen);

  Serial.println("Waiting for packet to complete..."); 
  delay(10);
  rf95.waitPacketSent();
  
  digitalWrite(LED_BUILTIN, HIGH); 
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

}
