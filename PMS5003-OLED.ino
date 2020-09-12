// =========================================================================
// Scrolling PMS5003 air sensor plot on 128x64 OLED display
// OLED example: amazon.com/gp/product/B076PDVFQD/
// J.Beale 12-July-2020
// =========================================================================

#include <Arduino.h>
#include <U8g2lib.h>  // for OLED display
#include <TimeLib.h>  // for real-time clock display
#include <SoftwareSerial.h>

#define pmsSerial Serial1  // use the hardware serial port (for Teensy 3.x)
// SoftwareSerial pmsSerial(2, 3); // use software serial: (Rx, Tx)

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define TIME_HEADER  "T"   // Header tag for serial time sync message
// #define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const int ledPin =  13;      // the number of the LED pin
const int sensorPin = A0;  // analog input signal
const unsigned long DEFAULT_TIME = 1594591998; // 12-July-2020 when first compiled

#define FONTNAME u8g2_font_5x8_mr
u8g2_uint_t width;      // pixel width of the scrolling text (must be lesser than 128 unless U8G2_16BIT is defined

// DSIZE is how many elements our data buffer contains (eg. # columns we can display on graph)
#define DSIZE 128
unsigned int data[DSIZE];
unsigned int dmax, dmin;  // max and min elements of data[]

const unsigned int sxmax = 128;  // # of pixels across display horizontally
const unsigned int symax = 64;   // # of pixels down display vertically

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
 
struct pms5003data pdata;

uint8_t i=0;                          // loop counter in main()
char dstring[20]; // time/date string

#define ACOUNT 1      // how many air sensor readings to average together

// ===========================================================================

void u8g2_plot(uint16_t offset) {  // plot graph, auto-scaled vertically to fit display

  uint16_t ii;
  uint8_t yv,last;
  float scale;
  
  if (dmax > dmin)
    scale = (1.0*symax-10) / (dmax - dmin);
  else
    scale = 1;
    
  for (uint16_t i=1;i<DSIZE;i++) {
    ii = (i + offset) % sxmax;
    yv = (symax-1) - (((float)data[ii] - dmin) * scale);
    if (i==1) last=yv;
    u8g2.drawLine(i-1, last, i, yv);
    last = yv;
  }
} // end u8g2_plot()

// ==========================================================================
// find largest and smallest value in array

void minmaxData() {
dmax = 0;
dmin = 65535;

  for (int i=0;i<DSIZE;i++) {
    if (data[i]>dmax) dmax=data[i];
    if (data[i]<dmin) dmin=data[i];
  }
} // end minmaxData()


// ==========================================================================
// --- load in an initial waveform to data array
void loadData() {
  for (int i=0;i<DSIZE;i++) {
    // data[i] = 1000 + 2000.0 * (sin(i/6.0) + 1.0) * (sin(i/70.0)+1.0);
    data[i] = pdata.particles_03um;
   }
  minmaxData();
} // end loadData()

// ==========================================================================
// read new analog data, and insert into array

void dataUpdate(uint8_t offset) {  

  uint16_t a = 0;

  // for (uint8_t i=0;i<10;i++) {
  //   a += analogRead(sensorPin);
  // }  

  for (int i=0; i<ACOUNT; i++) {
    digitalWrite(ledPin, HIGH);  
    delay(10); 
    digitalWrite(ledPin, LOW);  
    delay(2990); // wait for a new updated reading
    do {} while (!readPMSdata(&pmsSerial));  // read until we get a valid sentence
    //adat[0] += data.pm10_standard;
    //adat[1] += data.pm25_standard;
    //adat[2] += data.pm100_standard;
    //adat[3] += data.particles_03um; // Particles > 0.3um / 0.1L air
    //adat[4] += data.particles_05um;
    //adat[5] += data.particles_10um;
    //adat[6] += data.particles_25um;
    //adat[7] += data.particles_50um;
    //adat[8] += data.particles_100um; // Particles > 10.0 um / 0.1L air
  }
  a = pdata.pm25_standard;
  // a = pdata.particles_03um;  // select this reading 
  data[offset] = a;  // insert new reading into data[]
  minmaxData();  // update min,max vals
  
  sprintf(dstring,"%ld, %d",now(),a);
  Serial.println(dstring);
  
  // Serial.println(dmin);

} // end dataUpdate()

// ==========================================================================

void setup(void) {
  pinMode(ledPin, OUTPUT);  // LED indicator light
  digitalWrite(ledPin, HIGH);  
  Serial.begin(9600);
  delay(2000);
  Serial.println("time, pm03");
  Serial.println("# OLED Data Display v1");
  Serial.println("# Ready for Time Sync message");
  // for example, on Linux: "date +T%s\n > /dev/ttyACM0" (UTC time zone)
  pmsSerial.begin(9600);    // communicate with PMS5003 air sensor

  u8g2.begin();  
  u8g2.setFont(FONTNAME);  // set the target font to calculate the pixel width
  u8g2.setFontMode(0);    // enable transparent mode, which is faster
  do {} while (!readPMSdata(&pmsSerial));  // read until we get a valid sentence
  digitalWrite(ledPin, LOW);  

  loadData();             // fill data[] array
  //if (timeStatus() == timeNotSet) {
  //  setTime(DEFAULT_TIME); // set time but obviously incorrect
  // }
}

// ============================================================================

void loop(void) {

  do {
    if (Serial.available()) {
      processSyncMessage();
    }
  } while (now() < 8);
  
  dataUpdate(i);  // insert new reading into data[]
  i++;
  if (i >= DSIZE) {i=0;}

  // generate string with date/time as "YYMMDD HH:MM:SS"
  sprintf(dstring,"%02d%02d%02d %02d:%02d:%02d",year()%100,month(),day(),hour(),minute(),second());
  // Serial.println(dstring); 
  
  u8g2.firstPage();  
  do {
    u8g2.setCursor(1,8);  // show min, max values of data
    u8g2.print(dmin);
    u8g2.setCursor(26,8);
    u8g2.print(dmax);
    u8g2.setCursor(54,8); // show current date and time
    u8g2.print(dstring);

    u8g2_plot(i);

  } while( u8g2.nextPage() ); // loop until full display refreshed
 
} // end loop()


// ===========================================================================
// set Real-Time Clock

void processSyncMessage() {
  unsigned long pctime;
  // const unsigned long DEFAULT_TIME = 1594591998; // 12-July-2020 when first compiled
  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
       sprintf(dstring,"%04d-%02d-%02d %02d:%02d:%02d",year(),month(),day(),hour(),minute(),second());
       Serial.print("# Time set to ");
       Serial.println(dstring);        
     }
  }
}

// ==================================================================
// === read data from PMS5003 air sensor into global pdata
 
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
  memcpy((void *)&pdata, (void *)buffer_u16, 30);
 
  if (sum != pdata.checksum) {
    // Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}
