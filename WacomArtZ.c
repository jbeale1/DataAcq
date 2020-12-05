// Userland Wacom serial tablet driver for X11 (WACOM IV protocol)
// pressure-sensing stylus works with GIMP, MyPaint on Raspberry Pi

// 01-DEC-2020: v.1 basic x/y + 2 buttons
// 02-DEC-2020: v.2 added pressure
// 04-DEC-2020: v.3 added tilt
// J.Beale 04-DEC-2020   github.com/jbeale1

// Based on this reference data and code:
// -------------------------------------------------------------------
// Protocol: github.com/jigpu/linuxwacom-wiki-archive/blob/master/wiki/Serial_Protocol_IV.md
// Serial I/O: stackoverflow.com/questions/6947413/
// X11 events: www.kernel.org/doc/html/v4.14/input/event-codes.html
// Userland mouse driver example: gitlab.com/DarkElvenAngel/serial-mouse
// devices: www.kernel.org/doc/Documentation/input/input-programming.txt
// tablet: github.com/rfc2822/GfxTablet/blob/master/driver-uinput/networktablet.c
// trackpad: blog.marekkraus.sk/c/linuxs-uinput-usage-tutorial-virtual-gamepad/
// X11 config: help.ubuntu.com/community/RemoveWacomTabletError
// (not relevant?) evdev: digimend.github.io/support/howto/drivers/evdev/
// See also: https://www.raspberrypi.org/forums/viewtopic.php?f=63&t=292834

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define VERSION "Wacom ArtZ II serial driver v0.3 04-DEC-2020"

#define SERPORT  "/dev/ttyUSB0"  // tablet serial device port
#define WBYTES1 7  // 7 bytes in basic Wacom IV data packet
#define WBYTES2 9  // 9 bytes if X,Y tilt is enabled
#define DEVICENAME "stylus"  // xinput name  

// AFAIK Gimp won't recoginize it, unless it's named "stylus"
// MyPaint can work with other names

#define die(str, args...) do { \
        perror(str); \
        handle_sigint(9); \
    } while(0)

// ===================================================================
// Global Variables
// ================================================================== 
int                     sd;  // serial tablet input port
int                    	fd;  // X11 input device descriptor
struct uinput_user_dev 	uidev;  // for inputdevice setup
struct input_event     	ev;

int debug = 0;           // print out raw packet bytes
int tilt = 1;            // 1 enables tilt mode, 0 for basic no-tilt
int packetsize = WBYTES1; // # of bytes in packet depends on mode
int verbose = 0;         // 1 to display X,Y,... data
int pThresh = 8;         // pen pressure threshold for B1 button-press
int pOffsetMax = 15;     // max reasonable zero-pressure stylus offset
int pOffsetMin = 0;      // minimum stylus pressure reading this session
int pOffsetFuzz = 2;     // noise level on pressure offset
int pOffset = 0;         // current stylus pressure offset
int xResolution = 1200;  // tablet resolution, pixels per inch
int yResolution = 1200;  // tablet resolution, pixels per inch
int Ymin = 1920;  // limit tablet Ymin for 4:3 tablet and 16:9 screen
int Ymax = 7680;  // ArtZ II tablet Y axis max value
    

// ==================================================================
// Signal and Exit Handler
// ================================================================== 
void handle_sigint(int sig) 
{ 
	fprintf(stderr, "QUITTING\n");
	ioctl(fd, UI_DEV_DESTROY);
    close(fd);  // logical X11 device
    close(sd);  // hardware serial input port
	exit(0);
} 

// ==================================================================
// --- Configure hardware serial port for Tablet
// ==================================================================
int set_interface_attribs(int fd1, int speed)
{
    struct termios tty;

    if (tcgetattr(fd1, &tty) < 0) {
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

    if (tcsetattr(fd1, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

// ==================================================================
// set up timed input port reads 
// ==================================================================
void set_mincount(int fd1, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd1, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd1, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

// ==================================================================
// extract tablet data from pkt[] and take action
// ==================================================================
void parsePacket(unsigned char pkt[])
{
     // show raw 7-byte Wacom-IV packet in hex
     if (debug) {
       for (int j=0;j<packetsize;j++) 
         printf("%02x ",pkt[j]);
       printf("\n");
     }
     
     // as stylus moves up out of sense range, you get one last packet
     // with Byte 0, Bit 6 set to 0
     
     int tx,tx1 = 0;  // X,Y tilt value
     int ty,ty1 = 0;
     
     int px = ((pkt[0] & 0x40) == 0x40);  // 1 = pen in range 
     int xp = pkt[1]*128 + pkt[2];        // X coordinate
     int yp = pkt[4]*128 + pkt[5];        // Y coordinate
     yp += Ymin/2;                        // center active Y window
     if (yp > Ymax) yp = Ymax;  // force Y values inside active window
     if (yp < Ymin) yp = Ymin;
     int b1 = (pkt[3] & 0x10) == 0x10; // Button 1 
     int b2 = (pkt[3] & 0x20) == 0x20; // Button 2 (& eraser)
     int zp = (pkt[6]&0x3f) + px*0x40*(pkt[6]<0x40);  // pen pressure
     if (tilt) {
       tx = pkt[7]; // 7-bit signed int, -63..0..+63 (0x3f)
       ty = pkt[8];
       if (tx > 63) tx-=64;
       else tx += 64; // convert to 0..127 with center being 63
       if (ty > 63) ty-=64;  
       else ty += 64;
       // printf("%03d,%03d\n",tx,ty); // DEBUG show values
     }

     // hack to supress pressure sensor drift and noise
     if (px == 0) {    // stylus just went out of range?
         pOffsetMin = pOffsetMax;  // reset each time stylus goes away
     } else {
         if (zp < pOffsetMin) // track minimum value of this session
           pOffsetMin = zp;
         pOffset = pOffsetMin + pOffsetFuzz;
     }
     zp -= pOffset;  // fix a zero-pressure +offset on stylus

     int contact = 0;
     if (zp < 0) zp = 0;
     if (zp > 0) contact = 1;           // any force at all
     // if (zp > pThresh) b1 = 1;  // force button on by tip pressure
     
      /// (x,y), presure, button1, button2/eraser, proximity
     if (verbose) {
       printf("%05d , %05d , %03d , %d, %d, %d\n",xp,yp,zp,b1,b2,px);      
     }
            /// Send (X,Y) tablet data to virtual device
     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_ABS;
     ev.code = ABS_X;
     ev.value = xp;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");

     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_ABS;
     ev.code = ABS_Y;
     ev.value = yp;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");

     if (tilt) {
       memset(&ev, 0, sizeof(struct input_event));
       ev.type = EV_ABS;
       ev.code = ABS_TILT_X;
       ev.value = tx;
       if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: TILT_X write");

       memset(&ev, 0, sizeof(struct input_event));
       ev.type = EV_ABS;
       ev.code = ABS_TILT_Y;
       ev.value = ty;
       if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: TILT_Y write");

     }
                  
            /// tablet stylus pressure
     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_ABS;
     ev.code = ABS_PRESSURE;
     ev.value = zp;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");

               /// Stylus in range of tablet (eg. hovering)
     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_KEY;
     ev.code = BTN_TOOL_PEN;
     ev.value = px;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");

               /// Stylus non-zero force (touching)
     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_KEY;
     ev.code = BTN_TOUCH;
     ev.value = contact;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");

               /// Stylus Button 1
     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_KEY;
     ev.code = BTN_STYLUS;
     ev.value = b1;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");

               /// Stylus Button 2
     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_KEY;
     ev.code = BTN_STYLUS2;
     ev.value = b2;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");                 


              /// All done with this data readout
     memset(&ev, 0, sizeof(struct input_event));
     ev.type = EV_SYN;
     ev.code = 0;
     ev.value = 0;
     if(write(fd, &ev, sizeof(struct input_event)) < 0)
                  die("error: write");
                  
} // end parsePacket()

 
// ==================================================================
//  create X11 logical user input device
// ==================================================================
int init_device(int fd)
{    	
    	// enable synchronization
	if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
		die("error: ioctl UI_SET_EVBIT EV_SYN");
        
    // Note: for absolute axes, must set MIN, MAX values
    // Mouse uses BTN_LEFT etc, Tablet uses BTN_STYLUS

    if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        die("error: ioctl");
   	if (ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) < 0)
		die("error: ioctl UI_SET_KEYBIT");
	if (ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_PEN) < 0)
		die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS) < 0)
		die("error: ioctl UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS2) < 0)
		die("error: ioctl UI_SET_KEYBIT");
        
    if(ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0)
        die("error: ioctl");
    if(ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0)
        die("error: ioctl");
    if(ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0)
        die("error: ioctl");
    if(ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE) < 0)
        die("error: ioctl");

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, DEVICENAME);
    // uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.bustype = BUS_USB;
    //uidev.id.vendor  = 0x056a;  // Wacom
    //uidev.id.product = 0x00f4;
    uidev.id.vendor = 0x00;
    uidev.id.product = 0x00;
    uidev.id.version = 1;
    //uidev.ff_effects_max = 0;  // what is this?

// from KDE wacomtablet project 
// phabricator.kde.org/file/data/hj5om5upujwlln7bcz4u/PHID-FILE-osc4kcwgj2hkdlfslneq/file

    struct uinput_abs_setup xabs;
    xabs.code = ABS_X;
    xabs.absinfo.minimum = 0;
    xabs.absinfo.maximum = 10240; // X range of ArtZ II 6x8" tablet
    xabs.absinfo.fuzz = 0;
    xabs.absinfo.flat = 0;
    xabs.absinfo.resolution = xResolution;  // pixels per inch
    xabs.absinfo.value = 0;

    // use subset of tablet Y range, to match 16:9 screen aspect ratio
    struct uinput_abs_setup yabs;
    yabs.code = ABS_Y;
    yabs.absinfo.minimum = Ymin; // limit min range to 1920
    yabs.absinfo.maximum = Ymax;  // Y range of tablet
    yabs.absinfo.fuzz = 0;
    yabs.absinfo.flat = 0;
    yabs.absinfo.resolution = yResolution;  // pixels per inch
    yabs.absinfo.value = 0;
 
    struct uinput_abs_setup pressure;
    pressure.code = ABS_PRESSURE;
    pressure.absinfo.minimum = 0;
    pressure.absinfo.maximum = 127;  // Z (pressure) range of tablet
    pressure.absinfo.fuzz = 0;
    pressure.absinfo.flat = 0;
    pressure.absinfo.resolution = 0;
    pressure.absinfo.value = 0;
    
     
    if(write(fd, &uidev, sizeof(uidev)) < 0)
        die("error: UI Device Setup");
    
    //if (ioctl(fd, UI_DEV_SETUP, &uidev) < 0)
    //  die("UI_DEV_SETUP");
    if (ioctl(fd, UI_ABS_SETUP, &xabs) < 0)
      die("X setup");
    if (ioctl(fd, UI_ABS_SETUP, &yabs) < 0)
      die("Y setup");
    if (ioctl(fd, UI_ABS_SETUP, &pressure) < 0)
      die("pressure setup");

    if (tilt) {
     struct uinput_abs_setup xtilt;
     xtilt.code = ABS_TILT_X;
     xtilt.absinfo.minimum = 0;
     xtilt.absinfo.maximum = 127;  // x tilt range of tablet
     xtilt.absinfo.fuzz = 0;
     xtilt.absinfo.flat = 0;
     xtilt.absinfo.resolution = 0;
     xtilt.absinfo.value = 0;

     struct uinput_abs_setup ytilt;
     ytilt.code = ABS_TILT_Y;
     ytilt.absinfo.minimum = 0;
     ytilt.absinfo.maximum = 127;  // y tilt range of tablet
     ytilt.absinfo.fuzz = 0;
     ytilt.absinfo.flat = 0;
     ytilt.absinfo.resolution = 0;
     ytilt.absinfo.value = 0;
     
     if (ioctl(fd, UI_ABS_SETUP, &xtilt) < 0)
      die("X Tilt setup");
     if (ioctl(fd, UI_ABS_SETUP, &ytilt) < 0)
      die("Y Tilt setup");
    }
    
    if(ioctl(fd, UI_DEV_CREATE) < 0)
        die("error: device creation");
        
    return 0;
        
} // end startDev()

// ==================================================================
//   Main Wacom tablet driver program
// ==================================================================
int main()
{
    char *portname = SERPORT;
    int wlen;

    printf("%s\n",VERSION);
	uid_t uid=getuid();   /// Check Permisions
	if (uid != 0) {
		fprintf (stderr,"Must be run as root or with sudo!\n");
		return 1;
	}
    
        // xstr is command sent to device at start
    char *xstr = "\r#\r"; // reset protocol to WACOM IV (7 byte packet)
    // char *xstr = "~C\r";    // report max (X,Y) coords
    // char *xstr = "~#\r";    // report Model/ROM
    // char *xstr = "~R\r";    // report config
    // char *xstr = "FM1\r";   // enable tilt protocol (9-byte packet)
    

    sd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (sd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    set_interface_attribs(sd, B9600); // 9600 baud 8-N-1
    //set_mincount(sd, 0);            // set to pure timed read

    if (tilt) packetsize = WBYTES2;  // extended Wacom IV with X,Y tilt
    else packetsize = WBYTES1;       // basic Wacom IV protocol, no tilt

    int xlen = strlen(xstr);   
    wlen = write(sd, xstr, xlen);  // send cmd to serial device
    if (wlen != xlen) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(sd);    // delay for output
            
	if (tilt) {  // set up extended mode for including X,Y tilt
      char *tstr = "FM1\r";   // enable tilt protocol (9-byte packet)
      int tlen = strlen(tstr);   
      wlen = write(sd, tstr, tlen);  // send command 
      if (wlen != tlen) {
        printf("Error from write: %d, %d\n", wlen, errno);
      }
      tcdrain(sd);    // delay for output        
    }
    
    /// Fire Up UINPUT for X11 logical input device
	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd < 0)
        die("error: open");

    init_device(fd);  // create logical X11 user input device

    unsigned int i=0;  // index into packet buffer pkt[]


    // simple noncanonical serial input, in endless loop
    do {
        unsigned char buf[80];  // single-read serial input data buffer
        unsigned char pkt[10];  // one WACOM IV packet (7 or 9 chars)
        int rdlen;

        rdlen = read(sd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
            unsigned char *p;

            for (p = buf; rdlen-- > 0; p++) { // work over rec'd bytes
                
                if (*p & 0x80) i=0; // b7 is start-of-packet flag
                pkt[i++] = *p;      // add this byte to packet
                
                if (i > packetsize) {
                    printf("Packet Error, length=%d\n",i);
                    i=0;
                }
            } 
            
            if (i == packetsize) {       // complete packet received
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
*

$ cat /var/log/Xorg.0.log
[ 37754.227] (II) libinput: stylus: needs a virtual subdevice
[ 37754.227] (**) stylus Pen (0): Applying InputClass "wacom"
[ 37754.227] (**) stylus Pen (0): Applying InputClass "libinput tablet catchall"
[ 37754.227] (II) Using input driver 'libinput' for 'stylus Pen (0)'
[ 37754.227] (**) stylus Pen (0): always reports core events
[ 37754.227] (**) Option "Device" "/dev/input/event2"
[ 37754.228] (**) Option "_source" "_driver/libinput"
[ 37754.228] (II) libinput: stylus Pen (0): is a virtual subdevice
[ 37754.228] (**) Option "config_info" "udev:/sys/devices/virtual/input/input11/event2"
[ 37754.228] (II) XINPUT: Adding extended input device "stylus Pen (0)" (type: STYLUS, id 9)

$ xinput
⎡ Virtual core pointer                    	id=2	[master pointer  (3)]
⎜   ↳ Virtual core XTEST pointer              	id=4	[slave  pointer  (2)]
⎜   ↳ Microsoft  Microsoft Basic Optical Mouse v2.0 	id=6	[slave  pointer  (2)]
⎜   ↳ stylus Pen (0)                          	id=9	[slave  pointer  (2)]
⎣ Virtual core keyboard                   	id=3	[master keyboard (2)]
    ↳ Virtual core XTEST keyboard             	id=5	[slave  keyboard (3)]
    ↳ DELL DELL USB Keyboard                  	id=7	[slave  keyboard (3)]
    ↳ stylus                                  	id=8	[slave  keyboard (3)]

*/
