// based on PIGPIO example code
// compile with: gcc -Wall -pthread -o ptest1 ptest1.c -lpigpio -lrt
// Purpose: send out pulse train on GPIO pin starting at top of each second
// 
// J.Beale 01-DEC-2019

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>  // time of day tv_sec, tv_usec
#include <time.h>      // localtime, strftime

#include <pigpio.h> // pigpio.zip from http://abyz.me.uk/rpi/pigpio/download.html


int pnum = 2;  // GPIO 02
uint32_t dSP = 250;  // duration of high pulse, microseconds (short tick)
uint32_t dLP = 500;  // duration of high pulse, us (longer tone)
int tickPulses = 5;  // count of pulses in pulse train output
int minutePulses = 300;  // count of pulses for minute mark
int delay30s = 100000;    // usec delay between double-tick at 30s mark

// compensated delay: shorten this wait by amount the previous wait ran over
// p : 1 for higher precision delay
int compDelay(int T, int previous, int p) {
int startTick, realDelay;

    int newDelay = 2*T - previous;
    if (newDelay > 0) {
      startTick = gpioTick();
      if (p==1) {
        gpioDelay(newDelay/3);  // longer delays are less accurate
        gpioDelay(newDelay/3);
        gpioDelay(newDelay/3);
      } else {
        gpioDelay(newDelay);
      }
      realDelay = gpioTick() - startTick;
    } else realDelay = 0;  // actually slightly more than 0 but...
    return(realDelay);
}

// generate a pulse train containing P pulses, prec=1 for higher-precision delay
void sigGen(int P, int Dur, int prec) {

uint32_t realDelay;

  realDelay = Dur;
  for (int i=0;i<P;i++) {
    gpioWrite(pnum, 1); // Set GPIO[pnum] high 
    realDelay = compDelay(Dur, realDelay, prec);
    gpioWrite(pnum, 0); // Set GPIO[pnum] low
    realDelay = compDelay(Dur, realDelay, prec);
  }
}

// =============================================================================

int main(int argc, char *argv[]) {

struct timeval currentTime;  // for current time of day
long int usec[4];  // current seconds and microseconds
struct tm* ptm;    // local time zone date/time
int seconds=0;       // seconds value of current time
char time_string[40]; // time/date string

  if (gpioInitialise() < 0)
  {
   printf("PIGPIO init failed.\n");
   return -1;
  }

  gpioSetMode(pnum, PI_OUTPUT); // Set GPIO pin as output
  gpioSetPullUpDown(pnum, PI_PUD_OFF);  // Clear any pull-ups/downs

  do {

  gettimeofday(&currentTime, NULL);
  usec[0] = currentTime.tv_usec;
  
  // Note: 80us is a fudge factor which improves accuracy on my Pi3B unit
  long int dGoal = (1e6 - usec[0]) - 80;  // this long until top of next second
  if (dGoal > 0) gpioDelay(dGoal);
  
  // send out pulse train to mark start of a new second
  switch (seconds) {
    case 59:  // now reached top of the minute?
      sigGen(minutePulses, dLP, 0); // longer tone for minute mark
      break;
    case 58:  // omit tick at 59 seconds
      break;
    case 29:  // now reached the half-minute (30 sec)?
      sigGen(tickPulses, dSP, 1); // double tick
      gpioDelay(delay30s);  // pause between ticks
      sigGen(tickPulses, dSP, 1); 
      break;
    default:
      sigGen(tickPulses, dSP, 1); // normal second tick
  }

  gettimeofday(&currentTime, NULL);
  usec[2] = currentTime.tv_usec;

  ptm = localtime(&currentTime.tv_sec);
  seconds = ptm->tm_sec;  // current time, seconds of the minute
  strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
  printf(" %s  usec: %ld\n", time_string, usec[2]);

  } while (1);

  gpioTerminate();  // close pigpio library, release mem, reset DMA chans

  return 0;
} // end main()
