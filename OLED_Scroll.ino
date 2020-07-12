// Scrolling data plot on 128x64 OLED display
// for example amazon.com/gp/product/B076PDVFQD/
// J.Beale 12-July-2020

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const int sensorPin = A0;  // analog input signal

#define FONTNAME u8g2_font_5x8_mr
u8g2_uint_t width;      // pixel width of the scrolling text (must be lesser than 128 unless U8G2_16BIT is defined

// DSIZE is how many elements our data buffer contains (eg. # columns we can display on graph)
#define DSIZE 128
unsigned int data[DSIZE];
unsigned int dmax, dmin;  // max and min elements of data[]

const unsigned int sxmax = 128;  // # of pixels across display horizontally
const unsigned int symax = 64;   // # of pixels down display vertically
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
    data[i] = 1000 + 2000.0 * (sin(i/6.0) + 1.0) * (sin(i/70.0)+1.0);
  }
  minmaxData();
} // end loadData()

// ==========================================================================
// read new analog data, and insert into array

void dataUpdate(uint8_t offset) {  
  uint16_t a = 0;
  for (uint8_t i=0;i<10;i++) {
    a += analogRead(sensorPin);
  }
  data[offset] = a;
  
  minmaxData();  // update min,max vals
  Serial.println(dmin);
} // end dataUpdate()

// ==========================================================================

void setup(void) {
  Serial.begin(9600);
  delay(2000);
  Serial.println("OLED start 1");
  
  u8g2.begin();  
  u8g2.setFont(FONTNAME);  // set the target font to calculate the pixel width
  u8g2.setFontMode(0);    // enable transparent mode, which is faster
  loadData();             // fill data[] array
}

// ============================================================================
uint8_t i=0;

void loop(void) {

  dataUpdate(i);  // insert new reading into data[]
  i++;
  if (i >= DSIZE) {i=0;}

  u8g2.firstPage();  
  do {
    u8g2.setCursor(1,9);  // show min, max values of data
    u8g2.print(dmin);
    u8g2.setCursor(30,9);
    u8g2.print(dmax);
    u8g2_plot(i);

  } while( u8g2.nextPage() ); // loop until full display refreshed

  // delay(10);  // delay in animation loop
} // end loop()
