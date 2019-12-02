// PIGPIO example code
// gcc -Wall -pthread -o ftest1 ftest1.c -lpigpio -lrt
// send out pulse train on GPIO pin at top of each second
// J.Beale 01-DEC-2019

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>  // time of day tv_sec, tv_usec

#include <pigpio.h>


int pnum = 2;  // GPIO 02
uint32_t dTarget = 200;  // microseconds to delay

int main(int argc, char *argv[]) {

uint32_t startTick, endTick; // sys tick, the number of microseconds since system boot.
int diffTick;
struct timeval currentTime;  // for current time of day
uint32_t realDelay;  // actual microseconds that delay routine took
long int usec[4], sec[4];  // current seconds and microseconds

  if (gpioInitialise() < 0)
  {
   // pigpio initialisation failed.
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

  long int rdiff = realDelay - dGoal;
  gettimeofday(&currentTime, NULL);
  usec[1] = currentTime.tv_usec;
  sec[1] = currentTime.tv_sec;
  
  startTick = gpioTick();

  for (int i=0;i<5;i++) {
    gpioWrite(pnum, 1); // Set GPIO[pnum] high.
    realDelay = gpioDelay(dTarget);
    gpioWrite(pnum, 0); // Set GPIO[pnum] high.
    int32_t delta = realDelay - dTarget;
    int32_t newDelay = dTarget - delta;
    if (newDelay > 0) {
      realDelay = gpioDelay(newDelay);
    }  
  }

  endTick = gpioTick();
  diffTick = endTick - startTick;

  gettimeofday(&currentTime, NULL);
  usec[2] = currentTime.tv_usec;
  sec[2] = currentTime.tv_sec;

  printf("Initial time: sec: %ld  usec: %ld\n",sec[0],usec[0]);
  printf("actual excess delay: %ld\n",rdiff);
  printf("Start time: sec: %ld  usec: %ld\n",sec[1],usec[1]);
  printf("End time: sec: %ld  usec: %ld\n",sec[2],usec[2]);
  printf("Elapsed systick time %d microseconds\n", diffTick);

  } while (1);

  gpioTerminate();  // close pigpio library, release mem, reset DMA chans

  return 0;
} // end main()
