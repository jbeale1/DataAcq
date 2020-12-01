// Test of Wacom tablet (WACOM IV serial protocol)
// J.Beale 01-DEC-2020   github.com/jbeale1

// Protocol: github.com/jigpu/linuxwacom-wiki-archive/blob/master/wiki/Serial_Protocol_IV.md
// Serial I/O: stackoverflow.com/questions/6947413/
// X11 events: www.kernel.org/doc/html/v4.14/input/event-codes.html
// Userland mouse driver example: gitlab.com/DarkElvenAngel/serial-mouse

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define SERPORT  "/dev/ttyUSB0"  // tablet serial device name
#define PACKETSIZE 7             // basic Wacom IV packet without tilt

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

// ---------------------------------------------
// extract tablet data from pkt[] and take action

void parsePacket(unsigned char pkt[])
{
     // show raw 7-byte Wacom-IV packet in hex
     // for (unsigned int j=0;j<7;j++) printf("%02x ",pkt[j]);
     
     int px = ((pkt[0] & 0x40) == 0x40);  // pen in range?
     int xp = pkt[1]*128 + pkt[2];        // X coordinate
     int yp = pkt[4]*128 + pkt[5];        // Y coordinate
     int zp = (pkt[6]&0x3f) + px*0x40*(pkt[6]<0x40);  // pen pressure          
     int b1 = (pkt[3] & 0x10) == 0x10; // Button 1 
     int b2 = (pkt[3] & 0x20) == 0x20; // Button 2 (& eraser)
     
      // (x,y), presure, button1, button2/eraser, proximity
     printf("%05d , %05d , %03d , %d, %d, %d\n",xp,yp,zp,b1,b2,px);      
}

// ====================================================================
int main()
{
    char *portname = SERPORT;
    int fd;
    int wlen;
    
        // xstr is command sent to device at start
    char *xstr = "\r#\r"; // reset protocol to WACOM IV (7 byte packet)
    // char *xstr = "~C\r";    // report max (X,Y) coords
    // char *xstr = "~#\r";    // report Model/ROM
    // char *xstr = "~R\r";    // report config
    // char *xstr = "FM1\r";   // enable tilt protocol (9-byte packet)
    
    int xlen = strlen(xstr);

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    set_interface_attribs(fd, B9600); // 9600 baud 8-N-1
    //set_mincount(fd, 0);            // set to pure timed read

    wlen = write(fd, xstr, xlen);  // send a command to serial device
    if (wlen != xlen) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd);    // delay for output

    unsigned int i=0;  // index into packet buffer pkt[]


    // simple noncanonical serial input, in endless loop
    do {
        unsigned char buf[80];  // single-read serial input data buffer
        unsigned char pkt[10];  // one WACOM IV packet (7 or 9 chars)
        int rdlen;

        rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
            unsigned char *p;

            for (p = buf; rdlen-- > 0; p++) { // work over rec'd bytes
                
                if (*p & 0x80) i=0; // b7 is start-of-packet flag
                pkt[i++] = *p;      // add this byte to packet
                
                if (i > PACKETSIZE) {
                    printf("Packet Error, length=%d\n",i);
                    i=0;
                }
            } 
            
            if (i == PACKETSIZE) {       // complete packet received
              parsePacket(pkt);  // extract the data and take action
              i = 0;
            }

        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        } else {  /* rdlen == 0 */
            printf("Timeout from read\n");
        }               

    } while (1); // repeat read to get rest of data
    
} // end main


/*
 *   7-byte Wacom IV protocol
 * 
e0 23 0c 00 26 21 40 
e0 23 0e 00 26 26 40 
e0 23 11 00 26 2a 40 
* 
*    9-byte Wacom IV protocol
*
e0 0a 62 00 20 6b 40 2f 06 
e0 0a 64 00 20 74 40 2f 06 
e0 0a 68 00 21 00 40 30 05 
e0 0a 6b 00 21 06 40 31 05 
*
* 
*  X pos, Y pos, Pressure, B1, B2, Proximity
* 
03909 , 04887 , 000 , 0, 0, 1
03907 , 04887 , 003 , 0, 0, 1
03903 , 04887 , 010 , 0, 0, 1
03902 , 04889 , 021 , 0, 0, 1
03902 , 04889 , 057 , 0, 1, 1
03911 , 04880 , 105 , 0, 1, 1
03918 , 04872 , 103 , 0, 1, 1
03931 , 04869 , 071 , 0, 1, 1
03940 , 04850 , 008 , 0, 1, 1
03946 , 04835 , 000 , 0, 0, 0  <== pen leaves active area

*/
