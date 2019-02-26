/*
 * ADS1256_read.c:
 *
 * Code to test 24bit ADC chip on ADS1256 on Waveshare "High Precision AD/DA Board"
 * https://www.waveshare.com/wiki/High-Precision_AD/DA_Board
 * uses Broadcom BCM2835 library from http://www.airspayce.com/mikem/bcm2835/
 * Note due to SPI bus access, this code requires root permissions
 * Make:  gcc ADS1256_read.c -o ADS1256_read -lbcm2835 -lm
 *
 * Example: read first channel (AD0) for 20 samples at rate 5 (1 kHz) and print all samples
 *      sudo ./ADS1256_read 0 20 5 1
 *
 * ADS1256 has 16 different sampling rates. This code counts them 0 (30 kHz) to 15 (2.5 Hz) 
 * The fastest settings barely work, with extremely high noise (using RPi2 B).
 * A rate of 7.5 kHz or below is better and the slowest rates have the least noise.
 * Using print flag = 1 reduces effective sample rate, from printf() I/O overhead
 *
 * RPi2+Waveshare results sampling a low-noise source (NiCd AA battery) for 10 seconds:
 *  "30  kHz"  actual 25393 Hz, pk-pk noise = 10223504 counts, std.dev = 24090
 *  "7.5 kHz"  actual  7403 Hz, pk-pk noise =     3575 counts, std.dev = 66.1
 *  " 2  kHz"  actual  1998 Hz, pk-pk noise =      508 counts, std.dev = 26.4
 *  "60   Hz"  actual  60.0 Hz, pk-pk noise =       40 counts, std.dev = 5.85
 *  "2.5  Hz"  actual 2.500 Hz, pk-pk noise =        5 counts, std.dev = 1.19
 *
 * Code cleanup and mod for single-channel-only ADC reading by J.Beale 24-Feb-2019
 *
 */
 
#include <bcm2835.h>
#include <stdlib.h>    // atoi()
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>  // elapsed time to microseconds; measure sample rate
#include <time.h>      // time of day: time(), localtime()

#define  DRDY  17  // ADC Data Ready:  RPi GPIO P1.11
#define  RST   18  // ADC Reset:       RPi GPIO P1.12
#define  SPICS 22  // ADC Chip Select: RPi GPIO P1.15

#define CS_1() bcm2835_gpio_write(SPICS,HIGH)  // disable ADC SPI I/O
#define CS_0()  bcm2835_gpio_write(SPICS,LOW)  // enable ADC SPI I/O

#define DRDY_IS_LOW()	((bcm2835_gpio_lev(DRDY)==0))  // DRDY low = data ready

#define RST_1() 	bcm2835_gpio_write(RST,HIGH)   // normal operating state
#define RST_0() 	bcm2835_gpio_write(RST,LOW)    // reset is active low

/* Unsigned integer types  */
#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned long

typedef enum {FALSE = 0, TRUE = !FALSE} bool;

/* gain channel  */
typedef enum
{
	ADS1256_GAIN_1			= (0),	/* GAIN   1 */
	ADS1256_GAIN_2			= (1),	/*GAIN   2 */
	ADS1256_GAIN_4			= (2),	/*GAIN   4 */
	ADS1256_GAIN_8			= (3),	/*GAIN   8 */
	ADS1256_GAIN_16			= (4),	/* GAIN  16 */
	ADS1256_GAIN_32			= (5),	/*GAIN    32 */
	ADS1256_GAIN_64			= (6),	/*GAIN    64 */
} ADS1256_GAIN_E;

/* Sampling speed choice*/
/* 
	0xF0  11110000 = 30,000SPS (default)
	0xE0  11100000 = 15,000SPS
	0xD0  11010000 = 7,500SPS
	0xC0  11000000 = 3,750SPS
	0xB0  10110000 = 2,000SPS
	0xA1  10100001 = 1,000SPS
	0xA2  10010010 = 500SPS
	0x82  10000010 = 100SPS
	0x72  01110010 = 60SPS
	0x63  01100011 = 50SPS
	0x53  01010011 = 30SPS
	0x43  01000011 = 25SPS
	0x33  00110011 = 15SPS
	0x23  00100011 = 10SPS
	0x13  00010011 = 5SPS
	0x03  00000011 = 2.5SPS
*/

typedef enum
{
	ADS1256_30000SPS = 0,
	ADS1256_15000SPS,
	ADS1256_7500SPS,
	ADS1256_3750SPS,
	ADS1256_2000SPS,
	ADS1256_1000SPS,
	ADS1256_500SPS,
	ADS1256_100SPS,
	ADS1256_60SPS,
	ADS1256_50SPS,
	ADS1256_30SPS,
	ADS1256_25SPS,
	ADS1256_15SPS,
	ADS1256_10SPS,
	ADS1256_5SPS,
	ADS1256_2d5SPS,
	ADS1256_DRATE_MAX
} ADS1256_DRATE_E;

#define ADS1256_DRAE_COUNT 15    // maximum value of rate enum

typedef struct
{
	ADS1256_GAIN_E Gain;		/* GAIN  */
	ADS1256_DRATE_E DataRate;	/* DATA output  speed*/
	int32_t AdcNow[8];			/* ADC  Conversion value */
	uint8_t Channel;			/* The current channel*/
	uint8_t ScanMode;	/*Scanning mode,   0  Single-ended input  8 channel, 1 Differential input  4 channel*/
}ADS1256_VAR_T;

/*Register definition  Table 23. Register Map --- ADS1256 datasheet Page 30*/
enum
{
	/*Register address, followed by reset the default values */
	REG_STATUS = 0,	// x1H
	REG_MUX    = 1, // 01H
	REG_ADCON  = 2, // 20H
	REG_DRATE  = 3, // F0H
	REG_IO     = 4, // E0H
	REG_OFC0   = 5, // xxH
	REG_OFC1   = 6, // xxH
	REG_OFC2   = 7, // xxH
	REG_FSC0   = 8, // xxH
	REG_FSC1   = 9, // xxH
	REG_FSC2   = 10, // xxH
};

/* Command definition£º TTable 24. Command Definitions --- ADS1256 datasheet Page 34 */
enum
{
	CMD_WAKEUP  = 0x00,	// Completes SYNC and Exits Standby Mode 0000  0000 (00h)
	CMD_RDATA   = 0x01, // Read Data 0000  0001 (01h)
	CMD_RDATAC  = 0x03, // Read Data Continuously 0000   0011 (03h)
	CMD_SDATAC  = 0x0F, // Stop Read Data Continuously 0000   1111 (0Fh)
	CMD_RREG    = 0x10, // Read from REG rrr 0001 rrrr (1xh)
	CMD_WREG    = 0x50, // Write to REG rrr 0101 rrrr (5xh)
	CMD_SELFCAL = 0xF0, // Offset and Gain Self-Calibration 1111    0000 (F0h)
	CMD_SELFOCAL= 0xF1, // Offset Self-Calibration 1111    0001 (F1h)
	CMD_SELFGCAL= 0xF2, // Gain Self-Calibration 1111    0010 (F2h)
	CMD_SYSOCAL = 0xF3, // System Offset Calibration 1111   0011 (F3h)
	CMD_SYSGCAL = 0xF4, // System Gain Calibration 1111    0100 (F4h)
	CMD_SYNC    = 0xFC, // Synchronize the A/D Conversion 1111   1100 (FCh)
	CMD_STANDBY = 0xFD, // Begin Standby Mode 1111   1101 (FDh)
	CMD_RESET   = 0xFE, // Reset to Power-Up Values 1111   1110 (FEh)
};


ADS1256_VAR_T g_tADS1256;
static const uint8_t s_tabDataRate[ADS1256_DRATE_MAX] =
{
	0xF0,		/*reset the default values  */
	0xE0,
	0xD0,
	0xC0,
	0xB0,
	0xA1,
	0x92,
	0x82,
	0x72,
	0x63,
	0x53,
	0x43,
	0x33,
	0x23,
	0x13,
	0x03
};

void  bsp_DelayUS(uint64_t micros);
static void ADS1256_Send8Bit(uint8_t _data);
void ADS1256_CfgADC(ADS1256_GAIN_E _gain, ADS1256_DRATE_E _drate);
static void ADS1256_DelayDATA(void);
static uint8_t ADS1256_Recive8Bit(void);
static void ADS1256_WriteReg(uint8_t _RegID, uint8_t _RegValue);
static uint8_t ADS1256_ReadReg(uint8_t _RegID);
static void ADS1256_WriteCmd(uint8_t _cmd);
uint8_t ADS1256_ReadChipID(void);
static void ADS1256_WaitDRDY(void);
static void ADS1256_SetChannel(uint8_t _ch);
static int32_t ADS1256_ReadDataCont(void);
// static void ADS1256_SetDiffChannel(uint8_t _ch);
// static int32_t ADS1256_ReadData(void);

void ADS1256_Reset(void);                          // reset the chip
void time2string(char* start_date, struct timeval start); // convert time struct to readable string

// =========================================================================

// time delay functions from bcm2835 library
void  bsp_DelayUS(uint64_t micros)
{
  bcm2835_delayMicroseconds (micros);
}

void  bsp_DelayMS(uint32_t millis)
{
  bcm2835_delay	(millis);
}

/*
*********************************************************************************************************
*	name: ADS1256_Send8Bit
*	function: SPI bus to send 8 bit data
*	parameter: _data:  data
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_Send8Bit(uint8_t _data)
{

	bsp_DelayUS(2);
	bcm2835_spi_transfer(_data);
}


// ADS1256_Reset() : Reset the ADS1256 chip using its hardware reset pin

void ADS1256_Reset(void)                        // reset the chip
{
	RST_0();  // assert chip reset
	bsp_DelayUS(2);
	RST_1();  // release chip reset
	bsp_DelayUS(2);
}


/*
*********************************************************************************************************
*	name: ADS1256_CfgADC
*	function: The configuration parameters of ADC, gain and data rate
*	parameter: _gain:gain 1-64
*                      _drate:  data  rate
*	The return value: NULL
*********************************************************************************************************
*/
void ADS1256_CfgADC(ADS1256_GAIN_E _gain, ADS1256_DRATE_E _drate)
{
	g_tADS1256.Gain = _gain;
	g_tADS1256.DataRate = _drate;


	uint8_t buf[4];		/* Storage ads1256 register configuration parameters */

		/*Status register define
			Bits 7-4 ID3, ID2, ID1, ID0  Factory Programmed Identification Bits (Read Only)

			Bit 3 ORDER: Data Output Bit Order
				0 = Most Significant Bit First (default)
				1 = Least Significant Bit First
			Input data  is always shifted in most significant byte and bit first. Output data is always shifted out most significant
			byte first. The ORDER bit only controls the bit order of the output data within the byte.

			Bit 2 ACAL : Auto-Calibration
				0 = Auto-Calibration Disabled (default)
				1 = Auto-Calibration Enabled
			When Auto-Calibration is enabled, self-calibration begins at the completion of the WREG command that changes
			the PGA (bits 0-2 of ADCON register), DR (bits 7-0 in the DRATE register) or BUFEN (bit 1 in the STATUS register)
			values.

			Bit 1 BUFEN: Analog Input Buffer Enable
				0 = Buffer Disabled (default)
				1 = Buffer Enabled

			Bit 0 DRDY :  Data Ready (Read Only)
				This bit duplicates the state of the DRDY pin.

			ACAL=1  enable  calibration
		*/
	buf[0] = (0 << 3) | (1 << 2) | (1 << 1);// buffer ON, auto-cal ON
        // buf[0] = (0 << 3) | (1 << 2) | (0 << 1);  // buffer OFF, auto-cal ON

        //ADS1256_WriteReg(REG_STATUS, (0 << 3) | (1 << 2) | (1 << 1));

	buf[1] = 0x08;	

		/*	ADCON: A/D Control Register (Address 02h)
			Bit 7 Reserved, always 0 (Read Only)
			Bits 6-5 CLK1, CLK0 : D0/CLKOUT Clock Out Rate Setting
				00 = Clock Out OFF
				01 = Clock Out Frequency = fCLKIN (default)
				10 = Clock Out Frequency = fCLKIN/2
				11 = Clock Out Frequency = fCLKIN/4
				When not using CLKOUT, it is recommended that it be turned off. These bits can only be reset using the RESET pin.

			Bits 4-3 SDCS1, SCDS0: Sensor Detect Current Sources
				00 = Sensor Detect OFF (default)
				01 = Sensor Detect Current = 0.5 uA
				10 = Sensor Detect Current = 2  uA
				11 = Sensor Detect Current = 10 uA
				The Sensor Detect Current Sources can be activated to verify  the integrity of an external sensor supplying a signal to the
				ADS1255/6. A shorted sensor produces a very small signal while an open-circuit sensor produces a very large signal.

			Bits 2-0 PGA2, PGA1, PGA0: Programmable Gain Amplifier Setting
				000 = 1 (default)
				001 = 2
				010 = 4
				011 = 8
				100 = 16
				101 = 32
				110 = 64
				111 = 64
		*/
	buf[2] = (0 << 5) | (0 << 3) | (_gain << 0);
		// ADS1256_WriteReg(REG_ADCON, (0 << 5) | (0 << 2) | (GAIN_1 << 1));	/* choose 1: gain 1 ;input 5V */

	buf[3] = s_tabDataRate[_drate];	// DRATE_10SPS;	

		CS_0();	// SPI CS = 0   (CS active low. Doc claims no setup time needed after this)

        ADS1256_WaitDRDY();

	ADS1256_Send8Bit(CMD_WREG | 0);	/* Write command register, send the register address */
	ADS1256_Send8Bit(0x03);			/* Register number 4,Initialize the number   */

	ADS1256_Send8Bit(buf[0]);	/* Set the status register */
	ADS1256_Send8Bit(buf[1]);	/* Set the input channel parameters */
	ADS1256_Send8Bit(buf[2]);	/* Set the ADCON control register,gain */
	ADS1256_Send8Bit(buf[3]);	/* Set the output rate */

	CS_1();	/* SPI  CS = 1 */


        // printf("drate: %d  Buf3 = %d, %02x\n",_drate,buf[3], buf[3]);

	bsp_DelayUS(50);
}


/*
*********************************************************************************************************
*	name: ADS1256_DelayDATA
*	function: delay
*	parameter: NULL
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_DelayDATA(void)
{
	/*
		Delay from last SCLK edge for DIN to first SCLK rising edge for DOUT: RDATA, RDATAC,RREG Commands
		min  50   CLK = 50 * 0.13uS = 6.5uS
	*/
	bsp_DelayUS(30); // DANGER - BUG - EMPIRICAL DELAY.  Supposedly even 6.5us is ok... But no.
}

/*
*********************************************************************************************************
*	name: ADS1256_Recive8Bit
*	function: SPI bus receive function
*	parameter: NULL
*	The return value: NULL
*********************************************************************************************************
*/
static uint8_t ADS1256_Recive8Bit(void)
{
	uint8_t read = 0;
	read = bcm2835_spi_transfer(0xff);
	return read;
}

/*
*********************************************************************************************************
*	name: ADS1256_WriteReg
*	function: Write the corresponding register
*	parameter: _RegID: register  ID
*			 _RegValue: register Value
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_WriteReg(uint8_t _RegID, uint8_t _RegValue)
{
	CS_0();	/* SPI  cs  = 0 */
	ADS1256_Send8Bit(CMD_WREG | _RegID);	/*Write command register */
	ADS1256_Send8Bit(0x00);		/*Write the register number */

	ADS1256_Send8Bit(_RegValue);	/*send register value */
	CS_1();	/* SPI   cs = 1 */
}

/*
*********************************************************************************************************
*	name: ADS1256_ReadReg
*	function: Read  the corresponding register
*	parameter: _RegID: register  ID
*	The return value: read register value
*********************************************************************************************************
*/
static uint8_t ADS1256_ReadReg(uint8_t _RegID)
{
	uint8_t read;

	CS_0();	/* SPI  cs  = 0 */
	ADS1256_Send8Bit(CMD_RREG | _RegID);	/* Write command register */
	ADS1256_Send8Bit(0x00);	/* Write the register number */
	ADS1256_DelayDATA();	/*delay time */
	read = ADS1256_Recive8Bit();	/* Read the register values */
	CS_1();	/* SPI   cs  = 1 */

	return read;
}

/*
*********************************************************************************************************
*	name: ADS1256_WriteCmd
*	function: Sending a single byte order
*	parameter: _cmd : command
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_WriteCmd(uint8_t _cmd)
{
	CS_0();	/* SPI   cs = 0 */
	ADS1256_Send8Bit(_cmd);
	CS_1();	/* SPI  cs  = 1 */
}

/*
*********************************************************************************************************
*	name: ADS1256_ReadChipID
*	function: Read the chip ID
*	parameter: _cmd : NULL
*	The return value: four high status register
*********************************************************************************************************
*/
uint8_t ADS1256_ReadChipID(void)
{
	uint8_t id;

	ADS1256_WaitDRDY();
	id = ADS1256_ReadReg(REG_STATUS);
	return (id >> 4);
}

/*
*********************************************************************************************************
*	name: ADS1256_SetChannel
*	function: Configuration channel number
*	parameter:  _ch:  channel number  0--7
*	The return value: NULL
*********************************************************************************************************
*/
static void ADS1256_SetChannel(uint8_t _ch)
{
	/*
	Bits 7-4 PSEL3, PSEL2, PSEL1, PSEL0: Positive Input Channel (AINP) Select
		0000 = AIN0 (default)
		0001 = AIN1
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when PSEL3 = 1, PSEL2, PSEL1, PSEL0 are dont care)

		NOTE: When using an ADS1255 make sure to only select the available inputs.

	Bits 3-0 NSEL3, NSEL2, NSEL1, NSEL0: Negative Input Channel (AINN)Select
		0000 = AIN0
		0001 = AIN1 (default)
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when NSEL3 = 1, NSEL2, NSEL1, NSEL0 are dont care)
	*/
	if (_ch > 7)
	{
		return;
	}
	ADS1256_WriteReg(REG_MUX, (_ch << 4) | (1 << 3));	/* Bit3 = 1, AINN connection AINCOM */
}

/*
*********************************************************************************************************
*	name: ADS1256_SetDiffChannel
*	function: The configuration difference channel
*	parameter:  _ch:  channel number  0--3
*	The return value:  four high status register
*********************************************************************************************************
*/

/*
	Bits 7-4 PSEL3, PSEL2, PSEL1, PSEL0: Positive Input Channel (AINP) Select
		0000 = AIN0 (default)
		0001 = AIN1
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when PSEL3 = 1, PSEL2, PSEL1, PSEL0 are don't care)

		NOTE: When using an ADS1255 make sure to only select the available inputs.

	Bits 3-0 NSEL3, NSEL2, NSEL1, NSEL0: Negative Input Channel (AINN)Select
		0000 = AIN0
		0001 = AIN1 (default)
		0010 = AIN2 (ADS1256 only)
		0011 = AIN3 (ADS1256 only)
		0100 = AIN4 (ADS1256 only)
		0101 = AIN5 (ADS1256 only)
		0110 = AIN6 (ADS1256 only)
		0111 = AIN7 (ADS1256 only)
		1xxx = AINCOM (when NSEL3 = 1, NSEL2, NSEL1, NSEL0 are don't care)
*/

/*
static void ADS1256_SetDiffChannel(uint8_t _ch)
{
	if (_ch == 0)
	{
		ADS1256_WriteReg(REG_MUX, (0 << 4) | 1);	// DiffChannel  AIN0 AIN1
	}
	else if (_ch == 1)
	{
		ADS1256_WriteReg(REG_MUX, (2 << 4) | 3);	// DiffChannel   AIN2 AIN3
	}
	else if (_ch == 2)
	{
		ADS1256_WriteReg(REG_MUX, (4 << 4) | 5);	// DiffChannel    AIN4 AIN5
	}
	else if (_ch == 3)
	{
		ADS1256_WriteReg(REG_MUX, (6 << 4) | 7);	// DiffChannel   AIN6 AIN7
	}
}
*/

/*
*********************************************************************************************************
*	name: ADS1256_WaitDRDY
*	function: delay time  wait for automatic calibration
*	parameter:  NULL
*	The return value:  NULL
*********************************************************************************************************
*/

#define TIMEOUT 8000000   // deeply inadvisable magic timeout number

static void ADS1256_WaitDRDY(void)
{
	uint32_t i;

        bsp_DelayUS(5); // DANGER: EMPIRICAL DELAY. less may catch DRDY before it returns high(?)

	for (i = 0; i < TIMEOUT; i++)
	{
		if (DRDY_IS_LOW())
		{
			break;
		}
	}
	if (i >= TIMEOUT)
	{
		printf("Error: ADS1256_WaitDRDY() timeout.\n");
	}
}

/*
*********************************************************************************************************
*	Name: ADS1256_ReadDataCont
*	Function: make one ADC reading assuming continuous mode. Does not change channel, gain, etc
*	Return value: signed 32-bit integer (Min: -2^23, Max: +2^23-1)
*********************************************************************************************************
*/
static int32_t ADS1256_ReadDataCont(void)
{
 uint32_t read = 0;
 static uint8_t buf[3];

    CS_0();	/* SPI   CS = 0 */

    // read the 3 byte ADC value
    buf[0] = ADS1256_Recive8Bit();
    buf[1] = ADS1256_Recive8Bit();
    buf[2] = ADS1256_Recive8Bit();

    // assemble 3 bytes into one 32 bit word
    read = ((uint32_t)buf[0] << 16) & 0x00FF0000;
    read |= ((uint32_t)buf[1] << 8);
    read |= buf[2];

    CS_1();	/* SPI CS = 1 */

	/* Extend a signed number*/
    if (read & 0x800000)
    {
	    read |= 0xFF000000;
    }

    return (int32_t)read;
} // end ADS1256_ReadDataCont()

// ********************************************
// time2string()  convert time value to readable string
// ********************************************

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

#define new_max(x,y) ((x) >= (y)) ? (x) : (y)

void waitNext10s(void)  // wait until the start of the next even 10 second mark
{
  struct timeval start;
  // struct timeval  end;
  uint32_t millis;
  // char start_date[80], end_date[80];               // string holding start date/time

  gettimeofday(&start, NULL);  // what time is it right now?
  millis = 10000 - (1000*(start.tv_sec % 10) + (start.tv_usec / 1000));
  bsp_DelayMS(millis);  // now on even 10 second boundary (usually within a few ms)

  // time2string(start_date, start);  // convert time data to readable string
  // gettimeofday(&end, NULL);  // start time in microseconds
  // time2string(end_date, end);  // convert time data to readable string
  // printf("start %s, waited %lu millis, now %s\n",start_date,millis,end_date);

}

/*
*********************************************************************************************************
*	name: main
*	function: Read a single ADC channel a certain number of times
*       Usage Example: Read 100 samples from ADC Channel 3 (0 = CH1) at rate 5 and don't print each value:
*         sudo ./ADS1256_Read 2 100 5 0
*********************************************************************************************************
*/

#define COUNTSPERVOLT 1695929   // approximate, in Gain=1 mode

int main (int argc, char *argv[])
{
  uint8_t id;
  uint8_t readchannel;
  uint32_t readcount;
  uint8_t samplerate;
  uint8_t printout;  // 1 if we should display all readings
  uint8_t repeat;    // 1 for continuous groups of "readcount" readings
  uint8_t tsync;     // 1 to synchronize sample start to UTC second

  struct timeval start, end, total;  // find elapsed time to usec
  char start_date[80];               // string holding start date/time
  char end_date[80];                 // string holding end date/time

  readchannel = 2;  // default: ADC input channel (0..7)
  readcount = 100;  // default: number of samples to read
  samplerate = 7;   // default: 100 Hz sample rate
  printout = 0;     // default: don't print individual readings
  repeat = 0;       // default: only do one set of 'readcount' readings
  tsync = 0;        // default: don't synchronize to even seconds in UTC time

  if (argc < 2) {
    printf("Usage : %s [CHn:0-7] [count:1..] [rate:0-15] [print flag:0,1] [repeat flag:0,1]\n",argv[0]);
    printf("Example: sudo %s 2 100 7 0 0   Read AD2, 100 samples at rate 7 (100 Hz), no print, no repeat\n\n",argv[0]);
  }
  if (argc > 1)  // which channel to read
    readchannel = MIN(atoi(argv[1]),7);
  if (argc > 2)  // how many samples
    readcount = atoi(argv[2]);
  if (argc > 3)  // how many samples
    samplerate = MIN(atoi(argv[3]), ADS1256_DRAE_COUNT);
  if (argc > 4)  // printout flag 1=show all values
    if (atoi(argv[4]) > 0)
      printout = 1;
  if (argc > 5)  // repeat flag 1=run groups forever
    if (atoi(argv[5]) > 0)
      repeat = 1;
  if (argc > 6)  // sync flag 1= wait until next even 10 seconds
    if (atoi(argv[6]) > 0)
      tsync = 1;


  if (!bcm2835_init())  // initialize BCM2835 library
        return 1;

   bcm2835_spi_begin();  // configure SPI mode
   bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
   bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);
   bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);

   bcm2835_gpio_fsel(SPICS, BCM2835_GPIO_FSEL_OUTP); // configure GPIO pin as output
   bcm2835_gpio_write(SPICS, HIGH);                  // chip select is active low

   bcm2835_gpio_fsel(DRDY, BCM2835_GPIO_FSEL_INPT);  // DRDY signal comes from ADC
   bcm2835_gpio_set_pud(DRDY, BCM2835_GPIO_PUD_UP);  // DRDY is active low

   // note you could use the software reset command or the special clock pattern to reset
   // in which case you would not need this line, but the Waveshare board has it, so...
   bcm2835_gpio_fsel(RST, BCM2835_GPIO_FSEL_OUTP);   // set RST line as output
   bcm2835_gpio_write(RST, HIGH);                    // reset is active low

   ADS1256_Reset();                        // reset the chip
   id = ADS1256_ReadChipID();

   if (id != 3)   {
     printf("Error, ASD1256 Chip ID = 0x%d\n", (int)id);
     return 1;
   }

   // note possibly long auto-cal runs if sample rate changed from last run
   ADS1256_CfgADC(ADS1256_GAIN_1, (ADS1256_DRATE_E) samplerate);
   ADS1256_CfgADC(ADS1256_GAIN_1, (ADS1256_DRATE_E) samplerate);

   ADS1256_SetChannel(readchannel);	// select ADC channel (0..7)
   bsp_DelayUS(5);
   ADS1256_WriteCmd(CMD_SYNC);  // SYNC needed after changing input MUX
   bsp_DelayUS(5);
   ADS1256_WriteCmd(CMD_WAKEUP); // restart ADC after SYNC operation
   bsp_DelayUS(25);

   ADS1256_WriteCmd(CMD_RDATAC); // start continuous reading mode
   bsp_DelayUS(5);

   // print column headers for CSV type output
   if (printout != 0) printf("ADC_counts\n");
   if (repeat != 0) printf("end_time, samples, raw, stdev, pk-pk, volts\n");

   do {  // run more than once if repeat != 0

    // --- variables for computing ADC reading statistics ---
    int32_t sMax = -(1<<24); // set initial extrema
    int32_t sMin = 1<<24;    // maximum possible value from 24-bit ADC
    uint32_t n;              // count of how many readings so far
    double x,datSum,mean,delta,m2,variance,stdev;  // to calculate standard deviation
    // ----------------------------------------------------------------

    n = 0;     // total number of ADC readings so far
    mean = 0;  // start off with running mean at zero
    m2 = 0;    // intermediate var for std.dev
    datSum = 0; // running sum of readings

    if (tsync != 0)
      waitNext10s();  // wait until the start of the next even 10 second mark

    gettimeofday(&start, NULL);  // start time in microseconds
    time2string(start_date, start);  // convert time data to readable string

    while(n < readcount)  // ===== MAIN READING LOOP ===========
    {
        ADS1256_WaitDRDY();  // wait for ADC result to be ready
        int32_t raw = ADS1256_ReadDataCont(); // get new ADC result
        if (printout != 0) printf("%8ld\n", (long)raw);  // raw ADC value as integer

        if (raw > sMax) sMax = raw;  // remember max & min values seen so far
        if (raw < sMin) sMin = raw;

        x = (double) raw;          // convert to floating point
        datSum += x;
              // from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
        n++;
        delta = x - mean;
        mean += delta/n;
        m2 += (delta * (x - mean));
    }  // ============= END MAIN READING LOOP ==============

    gettimeofday(&end, NULL);  // at this point we're all done
    time2string(end_date, end);  // convert time data to readable string

    variance = m2/((double)n-1);  // (n-1):Sample Variance  (n): Population Variance
    stdev = sqrt(variance);  // Calculate standard deviation
    double datAvg = (1.0*datSum)/n;         // average reading in raw ADC counts
    double volts = datAvg / COUNTSPERVOLT;  // average reading in Volts (uncalibrated)

    timersub(&end, &start, &total);  // calculate (end-start) time interval
    double duration = total.tv_sec + total.tv_usec/1E6;  // elapsed time in seconds
    double sps = (double)n / duration;  // achieved rate in samples per second
    if (repeat == 0) {
      printf("# Avg: %5.3f  Std.Dev: %5.3f  Pk-Pk: %d  Volts: %8.7f\n",
        datAvg, stdev, (sMax-sMin), volts);
      printf("# Start: %s   End: %s\n", start_date, end_date );
      printf("# Samples: %lu  Time: %5.3f sec  Rate: %5.3f Hz\n\n",n, duration, sps);
    } else {
      // end_time, samples, raw, stdev, pk, volts
      printf("%s, %lu, %5.3f, %5.3f, %d, %8.7f\n",
        end_date, n, datAvg, stdev, (sMax-sMin), volts);
    }

   } while (repeat != 0);

   bcm2835_spi_end();
   bcm2835_close();

   return 0;
} // end main()
