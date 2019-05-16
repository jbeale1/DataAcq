#!/usr/bin/python3

# Simple code to read MCP3424 18-bit ADC  15-May-2019 JPB

from smbus import SMBus
from time import sleep
import datetime    # for time of day datestamp
# ---------------------

path = "/home/pi/FMES/"    # path to log file
tInt = 2.0                 # sampling interval in seconds (1 set of 4 channels)

fInterval = 100            # how many lines to write before flushing to filesystem
eol = "\n"           # end of line string in file output
sep = ","            # separator between data elements on one CSV line
# ----------------------------

bus = SMBus(1)      # I2C device is 18-bit ADC MCP3424
adc_address = 0x69  # MCP3424 I2C address selection

#  == MCP3424 CONFIG register bits: ==
# b7: RDY 1-shot mode:, 1=start conversion
# b6,b5: C1-C0: Channel 1-4: 00, 01, 10, 11
# b4: Convert Mode 1 = Continuous, 0 = One-Shot single convert
# b3,b2: Rate+Res 00=240Hz/12bit 01=60/14bit 10=15/16bit 11=3.75/18 bit
# b1,b0:  G1-G0: PGA Gain 00 = x1  01 = x2  10 = x4   11 = x8

#  1-shot, Ch1, 18 bits/3.75 sps, PGA Gain=x1   1000 1100 = 0x8c
#  1-shot, Ch2, 18 bits/3.75 sps, PGA Gain=x1   1010 1100 = 0xac
#  1-shot, Ch3, 18 bits/3.75 sps, PGA Gain=x1   1100 1100 = 0xcc
#  1-shot, Ch4, 18 bits/3.75 sps, PGA Gain=x4   1110 1110 = 0xee
#  1-shot, Ch4, 18 bits/3.75 sps, PGA Gain=x8   1110 1111 = 0xef

tFudge = 0.024             # seconds of loop overhead
tInterval = tInt-tFudge    # account for loop time overhead

cnum = 4  # how many channels there are
channels = [0x8c, 0xac, 0xcc, 0xef]  # list of channel config values

# calibration done 15-May-2019 to SCF2 10.000V standard from voltagereference.com
scalefac = [10.020, 10.0361, 10.0251, 1.0403 * 100.0 / 8.0]  # scaling for each ADC channel
raw = [0.0, 0.0, 0.0, 0.0]  # list of raw ADC values

cfactor = 1.00136   # correction scaling factor (onboard Vref calibration)
tacq = 1.0/3.75      # MCP3424: 18 bits => 3.75 samples per second acquisition time

def twos_complement(value, bitWidth):
    if value >= 2**bitWidth:
        # out of range should never happen, but...
        raise ValueError("Value: {} out of range of {}-bit value.".format(value, bitWidth))
    else:
        return value - int((value << 1) & 2**bitWidth)

def getadreading(address,adcConfig):
 w = bus.read_i2c_block_data(address,adcConfig)
 w24 = ( (w[0]<<16)  | (w[1]<<8) | w[2] ) # 24-bit word
 # print(format(w24, '08x'), end=' , ')  # DEBUG display hex value
 tc = twos_complement( w24, 24 )
 t = tc * cfactor * (2.048 / 2**17)  # gain=1 fullscale = +/- 2.048 V
 return t

# ====================


t = datetime.datetime.utcnow()
tsUTC = t.strftime("%y%m%d_%H%M%S")  # for example: 190516_183009
fname = path + tsUTC + "_3MCP.csv"          # data log filename with start date/time
f = open(fname, 'w')                 # open log file

f.write("Vtilt, Vc1, Vc2, degC, sec" + eol)
f.write("# FMES Seismometer AUX channels: MCP3424 4-Ch ADC log v0.2" + eol)
f.write("# Start: " + str(t) + " UTC" + eol)

# restart ADC conversion + read prev result which is junk because previous settings unknown
junk = getadreading(adc_address, channels[0]) # I2C address, config register with channel #

linecount = fInterval-5   # number of lines written to output file

while ( True ):
  # sleep(1/3.75) # MCP3424: 3.75 samples per second at 18 bit resolution
  sleep(tInterval - 3*tacq) # MCP3424: 3.75 samples per second at 18 bit resolution

  t = datetime.datetime.utcnow()
  sectime = t.time().second + t.time().microsecond/1000000.0

  raw[0] = getadreading(adc_address, channels[1]) # I2C address, config register with channel #
  for c in range(1, cnum):  # 1,2,3
    sleep(tacq)
    next = (c + 1) % cnum  # channel number to read next: 2,3,0
    raw[c] = getadreading(adc_address, channels[next]) # I2C address, config register with channel #

  for i in range(0,cnum):
    f.write( "{:+.{}f}".format( scalefac[i] * raw[i], 4 ) + sep)  # each ADC channel reading
  f.write( "{0:.3f}".format(sectime) + eol )  # ADC read started, second of the UTC minute [0...60)

  linecount += 1
  if (linecount > fInterval):
    f.flush()
    linecount = 0
