/*
 * Read serial data from Radio Shack multimeter 22-168A
 * also Micronta DMM 22-182 and 22-168, Metex M-3650D
 * Derived from http://www.linuxtoys.org/dvm/dvm.html
 * Example:  dmm /dev/tty02 > readings
 * Compile: gcc -o dmm dmm.c -lm
 */

#include <stdio.h>
#include <termio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <math.h>      // lrint()
#include <time.h>      // localtime()
#include <sys/time.h>  // gettimeofday()

void waitNextInterval(u_int32_t interval);  // wait until the start of the next time interval
void time2string(char* datestring, struct timeval tv);  // convert time val to string

char start_date[80];               // string holding start date/time

// ----------------------------------------------------------------------
// time2string()  convert time value to readable string

void time2string(char* datestring, struct timeval tv) 
{
  int millisec;        // milliseconds past date/time whole second
  char buffer[26];     // temporary string buffer
  struct tm* tm_info;  // local time data

    millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
      millisec -=1000;
      tv.tv_sec++;
    }

    tm_info = localtime(&tv.tv_sec);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    sprintf(datestring,"%s.%03d", buffer, millisec);  // save date/time to msec in string

}  // end time2string()

// ------------------------------------------------------------------------------------
// sleep until the top of the next time interval of specified length 
// eg. interval=10  waits until the next divisible-by-10 second mark

void waitNextInterval(u_int32_t interval)  
{
  struct timeval start;
  u_int32_t millis;

  gettimeofday(&start, NULL);  // what time is it right now?
  millis = (1000 * interval) - (1000*(start.tv_sec % interval) + (start.tv_usec / 1000));
  // printf("Waiting for %d millis\n",millis);

  struct timespec ts;
  ts.tv_sec = millis / 1000;
  ts.tv_nsec = (millis % 1000) * 1000000;
  nanosleep(&ts, NULL);

} // end waitNextInterval()

// ========================================================================================
int main (int argc, char *argv[])
{
    int        m;          // modem control lines
    int        fd_dev;     // file descriptor for the open serial port
    char       c;          //  char received from the DMM 
    struct  termios    tbuf;  // serial line settings
    struct timeval start;  // start time of conversion

    if (argc != 2) {
        printf("Usage: %s [tty port]\n", argv[0]);
        printf("Example: %s /dev/ttyUSB0\n", argv[0]);
        exit(1);
    }

    if (( fd_dev = open(argv[1], O_RDWR, 0)) < 0 ) {
        printf("Unable to open tty port specified\n");
        exit(1);
    }

    // parameters for RS DMM serial comm: 7,2,N 
    tbuf.c_cflag = CS7|CREAD|CSTOPB|B1200|CLOCAL;
    tbuf.c_iflag = IGNBRK;
    tbuf.c_oflag = 0;
    tbuf.c_lflag = 0;
    tbuf.c_cc[VMIN] = 1; /* character-by-character input */
    tbuf.c_cc[VTIME]= 0; /* no delay waiting for characters */
    if (tcsetattr(fd_dev, TCSANOW, &tbuf) < 0) {
        printf("%s: Unable to set device '%s' parameters\n",argv[0], argv[1]);
        exit(1);
    }

    /* Set DTR (to +12 or +5V depending on hardware) */
    m = TIOCM_DTR;
    if (ioctl(fd_dev, TIOCMSET, &m) < 0) {
        printf("%s: Unable to set '%s' modem status\n",argv[0], argv[1]);
        exit(1);
    }


    while (1) {
      gettimeofday(&start, NULL);  // start time in microseconds
      time2string(start_date, start);  // convert time data to readable string
      printf("%s, ",start_date);

      write(fd_dev,"D", 1);  // request reading from DMM
      do {                   // DMM response ends with '\r'
            read(fd_dev, &c, 1);
            printf("%c",c);
        } while (c != '\r');
      printf("\n");

      waitNextInterval(2);  // delay until top of next 'n' second interval
    }
    // should restore serial line parameters on exit, but...

    exit(0);
} // end main()
