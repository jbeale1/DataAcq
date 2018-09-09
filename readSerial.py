#!/usr/bin/python3

# tested with Python 3.6.5 (default, Apr  1 2018, 05:46:30) 
import serial

logfile='/home/myshake/serlog/temp-log.csv'

f = open(logfile, 'w')
fctr = 8

with serial.Serial('/dev/ttyUSB0', 9600, timeout=10) as ser:
  while True:
    line = ser.readline()
    # print(line)
    s0 = line.decode("utf-8").strip()  # bytes to string, without start/end whitespace
    s = s0.strip('\0')  # serial port open sometimes gives a null byte
    nc = len(s)  # how many useful characters in the input string?
    s = s + "\n"
    if ( nc > 0 ):
      # print(s,end='')
      f.write(s)
      fctr += 1
      if (fctr > 10):
        f.flush()
        fctr = 0
    # print(s,end='')
