// based on PIGPIO example code
// compile with: gcc -Wall -pthread -o ptest1 ptest1.c -lpigpio -lrt
// Purpose: send out pulse train on GPIO pin starting at top of each second
// J.Beale 01-DEC-2019

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>  // time of day tv_sec, tv_usec

#include <pigpio.h> // pigpio.zip from http://abyz.me.uk/rpi/pigpio/download.html


int pnum = 2;  // GPIO 02
uint32_t dHi = 250;  // duration of output high pulse, microseconds
uint32_t dLo = 250;  // duration of output low pulse, microseconds
int pulses = 5;  // count of pulses in pulse train output

// compensated delay: shorten this wait by amount the previous wait ran over
int compDelay(int T, int previous) {
int startTick, realDelay;

    int newDelay = 2*T - previous;
    if (newDelay > 0) {
      startTick = gpioTick();
      gpioDelay(newDelay/3);  // longer delays turn out to be less accurate
      gpioDelay(newDelay/3);
      gpioDelay(newDelay/3);
      realDelay = gpioTick() - startTick;
    } else realDelay = 0;  // actually slightly more than 0 but...
    return(realDelay);
}


int main(int argc, char *argv[]) {

struct timeval currentTime;  // for current time of day
uint32_t realDelay;  // actual microseconds that delay routine took
long int usec[4], sec[4];  // current seconds and microseconds

  if (gpioInitialise() < 0)
  {
   printf("PIGPIO init failed.\n");
   return -1;
  }

  gpioSetMode(pnum, PI_OUTPUT); // Set GPIO pin as output.
  gpioSetPullUpDown(pnum, PI_PUD_OFF);  // Clear any pull-ups/downs.

  do {

  gettimeofday(&currentTime, NULL);
  usec[0] = currentTime.tv_usec;
  sec[0] = currentTime.tv_sec;
  
  long int dGoal = (1e6 - usec[0]) - 80;  // how long until top of next second
  if (dGoal > 0) realDelay = gpioDelay(dGoal);
  else realDelay = 0;
  
  realDelay = dLo;    // initialize to the nominal value

  for (int i=0;i<pulses;i++) {
    gpioWrite(pnum, 1); // Set GPIO[pnum] high 
    realDelay = compDelay(dHi, realDelay);
    gpioWrite(pnum, 0); // Set GPIO[pnum] low
    realDelay = compDelay(dLo, realDelay);
  }

  gettimeofday(&currentTime, NULL);
  usec[2] = currentTime.tv_usec;
  sec[2] = currentTime.tv_sec;

  printf(" sec: %ld  usec: %ld\n",sec[2],usec[2]);

  } while (1);

  gpioTerminate();  // close pigpio library, release mem, reset DMA chans

  return 0;
} // end main()
