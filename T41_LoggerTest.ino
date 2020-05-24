/*********************************************************
   Example file for datalogger object
   medium (+slow) speed Analog  logger with ASCII file output

   M. Borgerson   5/13/2020
   mods J. Beale 5/24/2020
 **********************************************************/

#include <DataLogger.h>
// instantiate a datalogger object
DataLogger mydl;


//  The ISR handler for an interval timer must be at global scope,
//  and not in the DataLogger object scope.  THUS,  This function to
//  transfer back to the object so that the buffering can use all
//  the datalogger private variables.
void LoggerISR(void) {  // call the data logger collection ISR handler
  mydl.TimerChore();
}

//  samples per second, was 4000
#define SAMPLERATE 5
#define BUFFERMSEC 3200
char logfilename[64];

// use our own buffer on the T3.6
#define MAXBUFFER 220000
uint8_t mybuffer[MAXBUFFER];

bool logging = false;
bool firstLogLine = true;   // is this the first line of the log file? 
bool firstPrintLine = true; // first line of serial output printed summary file
uint32_t avgIn[4];          // running average of ADC values
uint8_t  drdg[4];           // digital values read in
uint32_t Asf = 10;          // scale factor for analog input value, so 12bit max = 4096 * Asf
const uint32_t ff = 20;    // filter factor: 10 means 1/10 current value added in each iteration
const uint32_t bavg = 100;  // boxcar-average together this many readings
bool mSeen = 0;             // any motion seen since last printout

// User-define data storage structure.
// A simple structure to hold time stamp #records written, and storage location
// The structure takes up 16 bytes to align millitime on 4-byte boundary
struct datrec {
  uint32_t unixtime;    // 1-second time/date stamp
  uint32_t millitime;   // millis() since collection start when collection occurred
  uint16_t avals[4];    // i6 byte record length
  uint8_t  dvals[4];    // digital values
};
const char compileTime [] = "  Compiled on " __DATE__ " " __TIME__;
const char hString [] = "Ctime,msec,A0,A1,A2,A3,D0";


TLoggerStat *lsptr;
uint32_t numrecs = 0;
elapsedMillis filemillis;

void setup() {
  uint32_t bufflen;

  pinMode(ledpin, OUTPUT);
  pinMode(A0, INPUT_DISABLE);  // 4 ADC inputs without digital "keeper" drive on pin
  pinMode(A1, INPUT_DISABLE);
  pinMode(A2, INPUT_DISABLE);
  pinMode(A3, INPUT_DISABLE);
  pinMode(2, INPUT);           // digital input from PIR sensor
  
  Serial.begin(9600);
  delay(2000);

  Serial.print("\n\nData Logger Example ");
  Serial.println(compileTime);
  analogReadResolution(12);
  analogReadAveraging(16);
  mydl.SetDBPrint(true);  // turn on debug output
  if (!mydl.InitStorage()) { // try starting SD Card and file system
    // initialize SD Card failed
    fastBlink();
  }
  // now try to initialize buffer.  Check that request falls within
  // size of local buffer or if it will fit on heap
  bufflen = mydl.InitializeBuffer(sizeof(datrec), SAMPLERATE, BUFFERMSEC, mybuffer);
  if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
    Serial.println("Not enough buffer space!  Reduce buffer time or sample rate.");
    fastBlink();
  }
  // Now attach our customized callback functions
  mydl.AttachCollector(&myCollector); // specify our collector callback function

  // If you attach no writer or a NULL pointer, by default all binary data is written

  mydl.AttachWriter(&myASCIIWriter); // logger saves Ascii data that you generate
  mydl.AttachDisplay(&myASCIIDisplay, 15'000); // display collected data once per X milliseconds

  avgIn[0] = Asf*analogRead(A0); // initialize filtered outputs with one (scaled) ADC reading
  avgIn[1] = Asf*analogRead(A1);
  avgIn[2] = Asf*analogRead(A2);
  avgIn[3] = Asf*analogRead(A3);

  memset(drdg,0,sizeof(drdg));  // zero out array to start

} // end setup;


void loop() {
  char ch;
  uint32_t bsum[4];

  memset(bsum,0,sizeof(bsum));  // zero out array to start
  
  // add together bavg readings
  for (int i=0;i<bavg;i++) {
    bsum[0] += Asf*analogRead(A0);
    bsum[1] += Asf*analogRead(A1);
    bsum[2] += Asf*analogRead(A2);
    bsum[3] += Asf*analogRead(A3);
  }
  drdg[0] = digitalReadFast(2);    // read PIR sensor
  if (drdg[0])  mSeen = 1;         // set the 'motion seen' flag
  
  avgIn[0] = (avgIn[0] * (ff-1) + (bsum[0]/bavg) )/ff;
  avgIn[1] = (avgIn[1] * (ff-1) + (bsum[1]/bavg) )/ff;
  avgIn[2] = (avgIn[2] * (ff-1) + (bsum[2]/bavg) )/ff;
  avgIn[3] = (avgIn[3] * (ff-1) + (bsum[3]/bavg) )/ff;

  mydl.CheckLogger();  // check for data to write to SD at regular intervals

  if (Serial.available()) {
    ch = Serial.read();
    if (ch == 'r')  StartLogging();
    if (ch == 'q') QuitLogging();
    if (ch == 's') GetStatus();
    if (ch == 'f') mydl.FormatSD(true); // format exFat for all sizes
    if (ch == 'd') mydl.ShowDirectory();
  }
  delay(1); // check very frequently
}

void StartLogging(void) {
  Serial.println("Starting Logger.");
  logging = true;
  MakeFileName(logfilename);
  mydl.StartLogger(logfilename, 30000);  // sync once per this many msec
  filemillis = 40; // Opening file takes ~ 40mSec

  Serial.print("\n");
}

void QuitLogging(void) {
  Serial.println("Stopping Logger.");
  mydl.StopLogger();
  logging = false;
}

void MakeFileName(char *filename) {
  sprintf(filename, "LOG_%02lu%02lu.csv", (now() % 86400) / 3600, (now() % 3600) / 60);
  Serial.printf("File name is:  <%s>\n", filename);
}

// blink at 5 Hz forever to signal unrecoverable error
void fastBlink(void) {
  while (1) {
    LEDON
    delay(100);
    LEDOFF
    delay(100);
  }
}

// can be called before, during, and after logging.
void GetStatus(void) {
  TLoggerStat *tsp;
  float mbytes;
  char stime[25];  // string for HH:MM:SS system time
  time_t t;
  tsp =  mydl.GetStatus();

  mbytes = tsp->byteswritten / (1024.0 * 1024.0);
  Serial.println("\nLogger Status:");
  t = now();
  sprintf(stime, "%04d-%02d-%02d %02d:%02d:%02d", year(t),month(t),day(t),hour(t),minute(t),second(t) );
  Serial.printf("Current time is: %s\n", stime);
  Serial.printf("Collection time: %lu seconds  ", tsp->collectionend - tsp->collectionstart);
  Serial.printf("MBytes Written: %8.2f\n", mbytes);
  Serial.printf("Max chunks ready: %u ",tsp->maxchunksready);
  Serial.printf("Overflows: %lu ", tsp->bufferoverflows);
  Serial.printf("Max Collect delay: %lu uSecs\n", tsp->maxcdelay);
  Serial.printf("Avg Write Time: %6.3f mSec   ", tsp->avgwritetime / 1000.0);
  Serial.printf("Max Write Time: %6.3f mSec \n\n", tsp->maxwritetime / 1000.0);

}

/***************************************************
     Callback function to handle user-defined collection
     and logging.
 ******************************************************/

// called from the datalogger timer handler
//  save time of day, milliseconds since start, and analog channels
void myCollector(  void* vdp) {
  volatile struct datrec *dp;
  // Note: logger status keeps track of max collection time
  dp = (volatile struct datrec *)vdp;
  dp->millitime = filemillis; // save mSec wev'e been collecting
  dp->unixtime = now();
  for (int i=0;i<4;i++) {
    dp->avals[i] = avgIn[i];  // copy filtered ADC values to log output data
    dp->dvals[i] = drdg[i];   // copy digital readings
  }
}

void myASCIIDisplay( void* vdp) {
  if (!logging) return;

  static char mstr[80];

  if (firstPrintLine) {
    sprintf(mstr, "%s\n", hString); // column header string
    Serial.printf("%s", mstr);     // don't add a newline, string has one already
    firstPrintLine = false;
  }
  
  sprintf(mstr, "%8lu, %6lu, %4u, %4u, %4u, %4u, %1u\n", (uint32_t)now(), (uint32_t)filemillis, avgIn[0], avgIn[1], avgIn[2],
                        avgIn[3],mSeen);

  Serial.printf("%s", mstr);     // don't add a newline, string has one already
  mSeen = false;                // reset PIR motion event flag
}


// Used to write ASCII data.  If you attach this
// function it will write data to the file in the  ASCII format
// you specify.  It is called once for each record saved.
uint16_t myASCIIWriter(void *bdp, void* rdp) {
  static char dstr[80];
  char** sptr;    // pointer to the address where we need to store our string pointer
  sptr = (char**) rdp;
  struct datrec *dp;

  if (firstLogLine) {    // print a column header in the first line of the log file
    sprintf(dstr,"%s\n",hString);  // column header string
    // sprintf(dstr,"Ctime,msec,A0,A1,A2,A3,D0\n");
    firstLogLine = false;
  } else {
  dp = (struct datrec *)bdp;  //type cast once to save typing many times
  // rdp is the ADDRESS of a pointer object
  // simple ascii format--could use a complex date/time output format
  sprintf(dstr, "%8lu, %6lu, %4u, %4u, %4u, %4u, %1u\n", dp->unixtime, dp->millitime, dp->avals[0],
                        dp->avals[1], dp->avals[2],dp->avals[3],dp->dvals[0]);
  }
  *sptr = &dstr[0]; // pass back address of our string
  return strlen(dstr);  // Datalogger will write the string to the output file.
}
