#!/usr/bin/python3

# tested with Python 3.6.5 (default, Apr  1 2018, 05:46:30) 

#   Example line in output logfile (time in UTC):
# Date Time, OutRH, OutC, InRH, InC
# 2018-09-24 22:46:48.723997, 53.30 , 20.40 , 45.60 , 21.70

import serial, datetime
from urllib.request import urlopen
from urllib.error import HTTPError

logfile='/home/myshake/serlog/temp-log.csv'
FLIM = 10  # flush buffer after this many lines
APIKEY='MySecretKeyString'
TURL='https://api.thingspeak.com/update?api_key='
F1='&field1='
F2='&field2='
F3='&field3='
F4='&field4='
DRATIO = 24 # decimation ratio, how many lines input per line output

# example ThingSpeak data update URL
# https://api.thingspeak.com/update?api_key=32X3XFHQAY803X7X&field1=22&field2=65&field3=25.1&field4=48.5

f = open(logfile, 'w')

fctr = 8  # saved-line counter (flush buffer after FLIM lines)
pctr = DRATIO-2  # sample decimation counter 
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
      sval = s.split(",")  # sval[0] is date, sval[1] is outside RH, ...
      rcnt = len(sval)     # number of elements in list s
      # print(rcnt)
      if (rcnt > 3):
        pctr += 1
        # if (pctr > 4):  # only print every 5th line to save space
        if (pctr >= DRATIO):  # decimate output by this factor
          pctr = 0 

          if (firstline == 1):
            oline = "date_time, " + s
            firstline = 0
          else:
            oline = str(datetime.datetime.now()) + ", " + s

          tups = TURL + APIKEY + F1 + sval[1].strip() + F2 + sval[0].strip() 
          tups = tups + F3 + sval[3].strip() + F4 + sval[2].strip() + "\n"
          # print(tups)  # URL to update ThingSpeak site
          try:
            resp = urlopen(tups)  # send data to ThingSpeak site via URL GET
            ts_reply = resp.read()  # reply from TS server
          except ( HTTPError):
            print("HTTP Error , " + oline)

          f.write(oline)
          fctr += 1
          if (fctr > FLIM):
            f.flush()
            fctr = 0

          # print(s,end='')
