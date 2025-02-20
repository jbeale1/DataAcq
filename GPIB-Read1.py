#!/usr/bin/env python

# Python 3 code to get readings from Keithley 196 multimeter via GPIB
# Assumes meter is already set up to correct mode, eg. from front panel
# 2012-2024 J.Beale

from serial import *
import time, datetime

cmdSetup = "+a:22\r".encode('utf-8')     # set which GPIB address for Casagrande GPIB adaptor board to talk with

cmd = "?\r".encode('utf-8')           # GPIB command to Keithley 196 must end with carriage return
eol_str = "\n"  # end of line string in file output
buffer =''           # serial input buffer
outbuf = ''          # file output buffer
lastAngle = '0.0'    # starting rotation stage angle
nSec = 1             # must be integer: how many seconds between readings

ser=Serial('/dev/ttyUSB0',460800,8,'N',1,timeout=0.5)  # GPIB-USB board on USB1, with 460800 bps, 8-N-1
#ser=Serial('/dev/ttyUSB1',460800,8,'N',1,timeout=0.5)  # GPIB-USB board on USB1, with 460800 bps, 8-N-1

f = open('K196-log.csv', 'a')  # open log file
print ("Keithley 196 log v0.3 3-Nov-2024 JPB")
f.write ("epoch_time, volts\n")
f.write ("# Keithley 196 log v0.31 19-Feb-2025 JPB")

ser.write(cmdSetup)                 # send setup command to GPIB board 
ser.read(255)            # clear out existing buffer & throw it away

ser.write(cmd)                 # send query to instrument
ser.readline()         # string terminated with '\n'

while True:

    # wait until the next even nSec second mark
    t = datetime.datetime.now(datetime.timezone.utc)
    sleeptime = nSec - ((t.second % nSec) + t.microsecond/1000000.0)
    time.sleep(sleeptime)
    
    ser.write(cmd)                 # send query to instrument
    rawbuf = ser.readline()
    buf = rawbuf.decode()         # string terminated with '\n'
    if (buf != '') :
      buffer = buf.split()[0]   # get rid of the \r\n characters at the end
      trim1 = buffer[4:]        # get rid of the 'NDCV' mode string at the start
      epoch = time.time()         
      outbuf = ("%.1f" % epoch) + ', ' + trim1 # assemble output string
      print (outbuf)
      f.write (outbuf)
      f.write (eol_str)
      time.sleep(0.5)                        # from 'time' to wait this many seconds
   
f.close                  # close log file
ser.close()            # close serial port when done. If we ever are...
