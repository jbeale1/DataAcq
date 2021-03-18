#!/usr/bin/python3
# Python code to read frequent lines from serial port
# and log the first line received after the top of each 10 second interval
# 2021-MAR-18 J.Beale

from serial import *
import time, datetime

LOGFILE = "TempHum-log.csv"
PORT = "/dev/ttyUSB0"
VERSION  = "Temp-Hum log v0.2 18-MAR-2021 JPB"
CSV_HEADER = "date_time, epoch, degC, RH, CRC16"
eol_str = "\n"  # end of line string in file output

ser=Serial(PORT,9600,8,'N',1,timeout=2)  # serial input
f = open(LOGFILE, 'w')  # open log file
print("%s" % VERSION)
print("%s" % CSV_HEADER)
f.write ("%s\n" % CSV_HEADER)
f.write ("# %s\n" % VERSION)

ser.read(255)            # clear out existing buffer & throw it away
lines = 0                # how many lines we've read
while True:
    buf = ""
    t = datetime.datetime.now()
    sleeptime = 10.0 - ((t.second % 10) + t.microsecond/1000000.0)
    tStop = t + datetime.timedelta(seconds=sleeptime)
    while True:      # wait until the next even 10 second mark
      buf = ser.readline()              # byte string
      tEpoch = int(time.time())         # time() = seconds.usec since Unix epoch
      tNow = datetime.datetime.now()
      if (tNow > tStop):
          break

    buffer = buf.decode("utf-8").rstrip()         # string terminated with '\n'
    if (buffer != '') :
      outbuf = str(datetime.datetime.now())[:-3] + "," + \
        str(tEpoch) + "," + buffer
      print (outbuf)
      f.write (outbuf)
      f.write (eol_str)
      lines += 1
      if (lines % 10 == 0):  # save it out to actual file, every so often
          f.flush()

f.close                  # close log file
ser.close()            # close serial port when done. If we ever are...
