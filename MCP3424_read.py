#!/usr/bin/python3

# Simple code to read MCP3424 18-bit ADC  5/5/2019 JPB

from smbus import SMBus
from time import sleep
import datetime    # for time of day datestamp

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
#  1-shot, Ch4, 18 bits/3.75 sps, PGA Gain=x1   1110 1100 = 0xec

cnum = 4  # how many channels there are
channels = [0x8c, 0xac, 0xcc, 0xec]  # list of channel config values
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
print("# MCP3424 4-Ch ADC log v0.1")
print("# Start: " + str(t) + " UTC")

# restart ADC conversion + read prev result which is junk because previous settings unknown
junk = getadreading(adc_address, channels[0]) # I2C address, config register with channel #

while ( True ):
  # sleep(1/3.75) # MCP3424: 3.75 samples per second at 18 bit resolution
  sleep((2.00 - 0.024) - 3*tacq) # MCP3424: 3.75 samples per second at 18 bit resolution

  t = datetime.datetime.utcnow()
  sectime = t.time().second + t.time().microsecond/1000000.0

  raw[0] = getadreading(adc_address, channels[1]) # I2C address, config register with channel #
  for c in range(1, cnum):  # 1,2,3
    sleep(tacq)
    next = (c + 1) % cnum  # channel number to read next: 2,3,0
    raw[c] = getadreading(adc_address, channels[next]) # I2C address, config register with channel #

  for i in range(0,cnum):
    print( "{:+.{}f}".format( raw[i], 6 ),end=' , ' )
  print( "{0:.3f}".format(sectime) )  # second of the UTC minute [0...60)
