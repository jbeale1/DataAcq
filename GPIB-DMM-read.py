#!/usr/bin/python2

# Python 2.7 code to get readings from Keithley 196 multimeter via GPIB
# Assumes meter is already set up to correct mode, eg. from front panel
# 2012-2017 J.Beale

from __future__ import print_function   # to use print without newline
from serial import *
import time, datetime

cmdSetup = "+a:22\r"      # set which GPIB address for Casagrande GPIB adaptor board to talk with

cmd = "?\r"            # GPIB command to Keithley 196 must end with carriage return
eol_str = "\n"  # end of line string in file output
buffer =''           # serial input buffer
outbuf = ''          # file output buffer
lastAngle = '0.0'    # starting rotation stage angle

ser=Serial('/dev/ttyUSB0',460800,8,'N',1,timeout=0.5)  # GPIB-USB board on USB1, with 460800 bps, 8-N-1
# ser=Serial(17,460800,8,'N',1,timeout=1)  # GPIB-USB board on COM18, with 460800 bps, 8-N-1

f = open('K196-log.csv', 'w')  # open log file
print ("Keithley 196 log v0.2 16-SEP-2017 JPB")
f.write ("date_time, volts\n")
f.write ("# Keithley 196 log v0.2 16-SEP-2017 JPB\n")

ser.write(cmdSetup)                 # send setup command to GPIB board 
ser.read(255)            # clear out existing buffer & throw it away


while True:

    # wait until the next even 10 second mark
    t = datetime.datetime.utcnow()
    sleeptime = 10.0 - ((t.second % 10) + t.microsecond/1000000.0)
    time.sleep(sleeptime)
    
    ser.write(cmd)                 # send query to instrument
    buf = ser.readline()         # string terminated with '\n'
    if (buf != '') :
      buffer = buf.split()[0]   # get rid of the \r\n characters at the end
      trim1 = buffer[4:]        # get rid of the 'NDCV' mode string at the start
      outbuf = str(datetime.datetime.now()) + ', ' + trim1 
      print (outbuf)
      f.write (outbuf)
      f.write (eol_str)
      time.sleep(1.00)                        # from 'time' to wait this many seconds
   
f.close                  # close log file
ser.close()            # close serial port when done. If we ever are...
