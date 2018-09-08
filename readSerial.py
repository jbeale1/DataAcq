#!/usr/bin/python3

# Read CSV text from serial port (USB adaptor) eg. Arduino 
# tested with Python 3.6.5 (default, Apr  1 2018, 05:46:30) 
import serial

logfile='temp-log.csv'

f = open(logfile, 'w')
fctr = 8

with serial.Serial('/dev/ttyUSB0', 9600, timeout=10) as ser:
  while True:
    line = ser.readline()
    # print(line)
    s = line.decode("utf-8").strip()  # bytes to string, without start/end whitespace
    s = s + "\n"
    nc = len(s)  # how many characters in the string?
    if ( nc > 0 ):
      print(s,end='')
      f.write(s)
      fctr += 1
      if (fctr > 10):
        f.flush()
        fctr = 0
    # print(s,end='')
