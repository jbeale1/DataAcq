/**
 * DHT22.c
 * Read data (Temperature & Relative Humidity) from DHT22 sensor. Average n readings.
 * Based on code found at:
 *    http://www.uugear.com/portfolio/read-dht1122-temperature-humidity-sensor-from-raspberry-pi/
 *    https://forums.raspberrypi.com/viewtopic.php?t=284053
 *    https://github.com/Qengineering/DHT22-Raspberry-Pi/blob/main/src/DHT22.cpp
 *
 * Compile with: cc -Wall DHT22.c -o DHT22 -lwiringPi
 * Run with: ./DHT22
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MAX_TIMINGS 85                  // Takes 84 state changes to transmit data 
// #define DHT_PIN     27                  // BCM16  GPIO-16  Phy 36 
#define DHT_PIN     7                  // GPIO-4  Phy 07 

#define BAD_VALUE   999.9f

int     data[5] = {0,0,0,0,0};
float   cachedH = BAD_VALUE;
float   cachedC = BAD_VALUE;

typedef struct {
	float c,h;   // temperature in degrees C, RH in %
} dStruct;

//------------------------------------------------------------------------------------
static int durn(struct timespec t1, struct timespec t2) {
	return(((t2.tv_sec-t1.tv_sec)*1000000) + ((t2.tv_nsec-t1.tv_nsec)/1000));	// elapsed microsecs
}

/**
 * read_DHT_Data
 * Signals DHT22 Sensor to send data.  Attemps to read data sensor sends.  If data passes checks, display it  
 * otherwise display cached values if valid else an error msg.
 * @param None
 * @return None
 */

dStruct read_DHT_Data()  {
    uint8_t lastState     = HIGH;
    uint8_t stateDuration = 0;
    uint8_t stateChanges  = 0;
    uint8_t bitsRead      = 0;
    float   h             = 0.0;
    float   c             = 0.0;
    dStruct retval;                     // output data of temperature & RH
    struct timespec	st, cur;

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    /* Signal Sensor we're ready to read by pulling pin UP for 10 milliseconds,
       pulling pin down for 18 milliseconds and then back up for 40 microseconds. */

    pinMode( DHT_PIN, OUTPUT );
    digitalWrite( DHT_PIN, HIGH );
    delay(10);
    digitalWrite( DHT_PIN, LOW );
    delay(18);
    digitalWrite( DHT_PIN, HIGH );
    delayMicroseconds(40);

    /* Read data from pin.  Look for a change in state. */

    pinMode( DHT_PIN, INPUT );

    // =================  Read data from sensor.
    for(stateChanges=0; (stateChanges < MAX_TIMINGS) && (stateDuration < 255); stateChanges++) {

        clock_gettime(CLOCK_REALTIME, &st);
        while((digitalRead(DHT_PIN)==lastState) && (stateDuration < 255) ) {
            clock_gettime(CLOCK_REALTIME, &cur);
            stateDuration=durn(st,cur);
        };

        lastState = digitalRead( DHT_PIN );

        // First 2 state changes are sensor signaling ready to send, ignore them.
        // Each bit is preceeded by a state change to mark its beginning, ignore it too.
        if( (stateChanges > 2) && (stateChanges % 2 == 0)){
            // Each array element has 8 bits.  Shift Left 1 bit.
            data[ bitsRead / 8 ] <<= 1;
            // A State Change > 35 µS is a '1'.
            if(stateDuration>35) data[ bitsRead/8 ] |= 0x00000001;

            bitsRead++;
        }
    }

    /**
     * Read 40 bits. (Five elements of 8 bits each)  Last element is a checksum.
     */

    if ( (bitsRead >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
        h = (float)((data[0] << 8) + data[1]) / 10.0;
        c = (float)((data[2] << 8) + data[3]) / 10.0;
        if ( data[2] & 0x80 )           // Negative Sign Bit on.
            c *= -1;
        cachedH  = h;
        cachedC  = c;
    }
    else {                              // Data Bad, use cached values.
        h = BAD_VALUE;
        c = BAD_VALUE;
    }

    //if ( (h == BAD_VALUE) || (c == BAD_VALUE) )
    //    printf("Data not good, Skipped\n");

    retval.c = c;
    retval.h = h;
    return (retval);
}


// ===============================================================================
int main( void ) {

    int hours, minutes, seconds, day, month, year;
    time_t now;
    dStruct sdat;                     // output data of temperature & RH
 
    if ( wiringPiSetup() == -1 )
        exit( 1 );

    read_DHT_Data();  // the first read doesn't work, for some reason

    int n = 50;  // how many samples to average

    for (;; ) // infinite loop
    {
        time(&now);  // get current time
        struct tm *local = localtime(&now);
        hours = local->tm_hour;         // get hours since midnight (0-23)
        minutes = local->tm_min;        // get minutes passed after the hour (0-59)
        seconds = local->tm_sec;        // get seconds passed after a minute (0-59)
        day = local->tm_mday;            // get day of month (1 to 31)
        month = local->tm_mon + 1;      // get month of year (0 to 11)
        year = local->tm_year + 1900;   // get year since 1900
        printf("%04d-%02d-%02d_%02d:%02d:%02d, ", year,month,day,hours,minutes,seconds);

        double c = 0;
        double h = 0;
        int count = 0;
        for (; count<n; ) {
            sdat = read_DHT_Data();
            if (sdat.c != BAD_VALUE) {
	            c += sdat.c;
        	    h += sdat.h;
                    count++;
                    delay(10);
            }
        }
        c /= count;
        h /= count;
        printf("%6.3f, %6.3f\n", c, h);
    }

    return(0);
}
