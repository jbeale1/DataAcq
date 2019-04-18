#!/usr/bin/python3

# Read raw serial data from UNI-T UT61E multimeter (and similar)
# Based on www.starlino.com/uni-t-ut61e-multimiter-serial-protocol-reverse-engineering.html
# /dev/ttyUSB0  19200 7-O-1   (19200 baud, 7 bits, odd parity, 1 stop bit)

import serial

port = '/dev/ttyUSB0'
ser = serial.Serial(port, 19200, timeout=2, bytesize=7, parity='O', stopbits=1)
ser.setDTR(True) # DTR-RTS pin voltage diff powers UNI-T serial cable opto-isolator
ser.setRTS(False) # note: using PL-2303 USB to DB9/M serial cable, 067b:2303

while True:
  s = ser.readline()
  print(s)

# Sample output with meter in ohms, reading 13.359k ohms:
# b'213359300020\r\n'
# b'213359300020\r\n'
# b'213358300020\r\n'
# b'213358300020\r\n'
