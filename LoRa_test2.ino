// Read LIS3DH, transmit data via LoRa
// 12-Sep-2021 JPB

#include <SPI.h>       // for RFM95 board
#include <RH_RF95.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
// ------------------------------------------------------------

/* for RFM95 on Feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 915.0  // MHz center freq. for Tx/Rx

#define PKTLEN 20        // bytes in RF packet to send

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// ------------------------------------------------------------
#define HBINS 8            // how many motion histogram bins
#define MMIN 0              // minimum valid motion-activity index
#define MMAX (MMIN+HBINS-1) // maximum valid motion-activity index

int mhist[HBINS];  // histogram to accumulate motion stats
char buf[PKTLEN];  // buffer to send out via LoRa

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

long int xsum,ysum,zsum;  // accumulated readings of X,Y,Z axes
const int acount = 2;  // how many readings to average
float norm = 80000;    // summed raw values that equals 1G
float xsm,ysm,zsm;     // smoothed accel values
const float f=0.35;          // low-pass filter value

int16_t packetnum = 0;  // packet counter, we increment per xmit

void setup(void) {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);  
  pinMode(LED_BUILTIN, OUTPUT);    // enable onboard LED
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(115200);
  delay(1000);  digitalWrite(LED_BUILTIN, HIGH); 
  delay(1000);  digitalWrite(LED_BUILTIN, LOW);  
  Serial.println("log-dmag,mIndex");

  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1) yield();
  }

  lis.setDataRate(LIS3DH_DATARATE_10_HZ); // [1,10,25,50,100,200,400]
  /*
    case LIS3DH_DATARATE_POWERDOWN: Serial.println("Powered Down"); break;
    case LIS3DH_DATARATE_LOWPOWER_5KHZ: Serial.println("5 Khz Low Power"); break;
    case LIS3DH_DATARATE_LOWPOWER_1K6HZ: Serial.println("16 Khz Low Power"); break;
  }
  */
  float rscale = 16000;  // roughly 1 G
  float xsm,ysm,zym;     // smoothed accel values
  lis.read();      // get X,Y,Z data at once
  xsm = lis.x/rscale;
  ysm = lis.y/rscale;
  zsm = lis.z/rscale;
  
  // --------------------------- RFM95 setup ---------------
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");    
    while (1);
  }
  Serial.println("LoRa radio init OK!");
  rf95.setFrequency(RF95_FREQ);
  RH_RF95::ModemConfig myconfig = { 0x72, 0x84, 0x00 }; // 125 kHz BW, SF=8
  rf95.setModemRegisters(&myconfig);  // setup LoRa to optimal values
  rf95.setTxPower(18, false);
  
} // end setup()

// ---------------------------------------------------------------------------
void loop() {
  for (int i=0;i<HBINS;i++){
    mhist[i]=0;   // reset all histogram bins to 0
  }
  for (int j=0;j<20;j++) {
    xsum=0; 
    ysum=0;
    zsum=0;
    float ax,ay,az;
    for (int i=0;i<acount;i++) {
      lis.read();      // get X,Y,Z data at once
      xsum += lis.x;
      ysum += lis.y;
      zsum += lis.z;
      delay(100);
    }
  
    // print out averaged (& scaled) data
    // 1 g ~ 80000 counts (if acount = 5)
    ax = xsum/norm;
    ay = ysum/norm;
    az = zsum/norm;
    xsm = ax*f + (xsm*(1-f));
    ysm = ay*f + (ysm*(1-f));
    zsm = az*f + (zsm*(1-f));
    float dx = ax-xsm;
    float dy = ay-ysm;
    float dz = az-zsm;
    // dmag is magnitude of change in direction from recent average
    // which is an estimate of overall motion
    float dmag = sqrt(dx*dx + dy*dy + dz*dz) + 0.0005;
    float ld = 2*log(dmag);
    int midx = min(max((ld+(1.2*HBINS)),MMIN),MMAX);
    mhist[midx]++;  // increment this bin in the histogram
  } // for (j...)
  strncpy(buf,"0000000000000000000",20);
  //Serial.println(buf);
  for (int i=0;i<HBINS;i++){  // insert HBINS chars into buf[]
    itoa(min(mhist[i],15),(buf + i),16);
    //Serial.print(mhist[i]); Serial.print(",");
  }
  Serial.println(buf);
  buf[PKTLEN-1] = 0;  // make sure it's null-terminated
  rf95.send((uint8_t *)buf, PKTLEN);
  digitalWrite(LED_BUILTIN, HIGH);   // blink onboard LED with each packet
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);

}
