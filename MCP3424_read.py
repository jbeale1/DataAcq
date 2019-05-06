#!/usr/bin/python3

# Simple code to read MCP3424 18-bit ADC  5/5/2019 JPB

from smbus import SMBus
from time import sleep

bus = SMBus(1)

def twos_complement(value, bitWidth):
    if value >= 2**bitWidth:
        # out of range should never happen, but...
        raise ValueError("Value: {} out of range of {}-bit value.".format(value, bitWidth))
    else:
        return value - int((value << 1) & 2**bitWidth)

def getadreading(address,adcConfig):
 w = bus.read_i2c_block_data(address,adcConfig) # default is reading 4 bytes
 w24 = ( (w[0]<<16)  | (w[1]<<8) | w[2] ) # 24-bit result word, w[3] is config register
 # print(format(w24, '08x'), end=' , ')  # DEBUG display hex value
 tc = twos_complement( w24, 24 )
 t = tc * (2.048 / 2**17)  # gain=1 fullscale = +/- 2.048 V
 return t

# ====================

# the first reading always seems to be inaccurate, so just throw this away
junk = getadreading(0x69,0x8c) # I2C address, config register value

while ( True ):
  sleep(1/3.75) # MCP3424: 3.75 samples per second at 18 bit resolution

  # MCP3424 Config register:  1-shot, Ch1, 18 bits/3.75 sps, PGA Gain=x1
  # 1000 1100 = 0x8c
  k = getadreading(0x69,0x8c) # I2C address, config register with channel #
  print( "{:+.{}f}".format( k, 6 ) )
