#!/usr/bin/python3

# tested with Python 3.6.5 (default, Apr  1 2018, 05:46:30) 
import serial, datetime

logfile='/home/myshake/serlog/temp-log.csv'
FLIM = 10  # flush buffer after this many lines

f = open(logfile, 'w')

fctr = 8  # saved-line counter (flush buffer after FLIM lines)
pctr = 5  # sample decimation counter 
firstline = 1  # have not yet finished the first line

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
      pctr += 1
      if (pctr > 4):  # only print every 5th line to save space
        pctr = 0

        if (firstline == 1):
          oline = "date_time, " + s
          firstline = 0
        else:
          oline = str(datetime.datetime.now()) + ", " + s

        f.write(oline)
        fctr += 1
        if (fctr > FLIM):
          f.flush()
          fctr = 0
      # print(s,end='')
