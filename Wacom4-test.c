
// ultra-crude test of Wacom ArtZ II serial tablet (WACOM IV serial protocol)
// connected through USB-Serial adaptor on ttyUSB0
// from  stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c/38318768#38318768

#define TERMINAL    "/dev/ttyUSB0"

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

// ====================================================================
int main()
{
    char *portname = TERMINAL;
    int fd;
    int wlen;
    
    char *xstr = "\r#\r"; // reset protocol to WACOM IV
    // char *xstr = "~C\r"; // report max (X,Y)
    //char *xstr = "~#\r";  // report Model/ROM
    // char *xstr = "~R\r";    // report config
    // char *xstr = "FM1\r";    // enable tilt protocol
    // FM1	Enable extra protocol for tilt management
    int xlen = strlen(xstr);

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    // set_interface_attribs(fd, B115200);
    set_interface_attribs(fd, B9600);
    //set_mincount(fd, 0);                /* set to pure timed read */

    /* simple output */
    wlen = write(fd, xstr, xlen);
    if (wlen != xlen) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd);    /* delay for output */

    unsigned int i=0;  // index into packet buffer pkt[]

    /* simple noncanonical input */
    do {
        unsigned char buf[80];  // raw set of serial input data
        unsigned char pkt[10];  // WACOM IV data packet (7 or 9 chars)
        int rdlen;

        rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {

// #define DISPLAY_STRING

#ifdef DISPLAY_STRING
            buf[rdlen] = 0;
            printf("Read %d: \"%s\"\n", rdlen, buf);
#else /* display hex */

            unsigned char *p;
            // printf("Read %d:", rdlen);
            for (p = buf; rdlen-- > 0; p++) {
                if (*p & 0x80) i=0;
                pkt[i++] = *p;
                if (i > 7) {
                    printf("Error i=%d\n",i);
                    i=0;
                }
            }  // watch out for buffer overrun
            if (i == 7) {
                
              //for (unsigned int j=0;j<7;j++) {
              //  printf("%02x ",pkt[j]);
              // }
              int xh = ((int32_t) pkt[1]); // high part of X coord
              int xl = ((int32_t) pkt[2]); // low bits of X coord
              int xp = xh*128 + xl;
              int yp = ((int32_t)pkt[4])*128 + pkt[5];
              int zp = (int32_t) (pkt[6] & 0x3f) + 0x40*((pkt[6] < 0x40));
              printf("%d , %d , %d\n",xp,yp,zp);  // x,y coords
              // printf("\n");
              i = 0;
            }
#endif
        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        } else {  /* rdlen == 0 */
            printf("Timeout from read\n");
        }               
        /* repeat read to get full message */
    } while (1);
}

/*
 *   7-byte protocol
 * 
e0 23 0c 00 26 21 40 
e0 23 0e 00 26 26 40 
e0 23 11 00 26 2a 40 
e0 23 13 00 26 37 40 
e0 23 10 00 26 53 40 
e0 23 08 00 26 61 40 
e0 22 7f 00 26 71 40 
*
* 
* 
*    9-byte protocol
*
e0 0a 62 00 20 6b 40 2f 06 
e0 0a 64 00 20 74 40 2f 06 
e0 0a 68 00 21 00 40 30 05 
e0 0a 6b 00 21 06 40 31 05 
e0 0a 71 00 21 12 40 31 04 
e0 0a 77 00 21 1d 40 32 04 
e0 0a 7a 00 21 22 40 32 03 
e0 0b 02 00 21 2b 40 33 03 
* 
* 
* 
 X   ,  Y   , Z (pressure)
2640 , 4987 , 61
2622 , 4987 , 60
2575 , 4983 , 58
2560 , 4982 , 58
2529 , 4976 , 57
2501 , 4970 , 57
2475 , 4962 , 56
2451 , 4954 , 54
2420 , 4940 , 51
2401 , 4930 , 50
2392 , 4924 , 50
2384 , 4918 , 50


*/
