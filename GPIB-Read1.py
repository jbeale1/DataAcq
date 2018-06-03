#!/usr/bin/python

# Python 2.7 code to get readings from Keithley 196 multimeter via GPIB
# Assumes meter is already set up to correct mode, eg. from front panel
# Dec. 17 2012 - June 2 2018  J.Beale

#  sudo apt-get install cu          <-- install basic serial terminal
#  cu -l /dev/ttyUSB0 -s 460800     <-- connect to first USB serial port
#  +a:22                            <-- set GPIB adaptor target address to 22
#  ?                                <-- request data from GPIB DVM on addr 22
#  ~.                               <-- exit from 'cu' command

from __future__ import print_function   # to use print without newline
from serial import *
import time,datetime

cmd1 = '+a:22'    # talk to GPIB Address 22
cmd2 = '*IDN?'    # get instrument name / ID
cmd = "?\r"            # GPIB command to Keithley 196 must end with carriage return

eol_str = "\n"  # end of line string in file output
buffer =''           # serial input buffer
outbuf = ''          # file output buffer

# ======================================================

def doCmd(cmd):
  buffer = ''              # serial input buffer
  retbuf = ''              # file output buffer
  ser.read(255)            # clear out existing buffer & throw it away
  ser.write(cmd)                 # send query to instrument
  buf = ser.readline()         # string terminated with '\n'
  print(buf)
  # buffer = buf.split()[0]   # get rid of the \r\n characters at the end
  retbuf = str(datetime.datetime.now()) + ',' + buf
  return retbuf
# ======================================================

# open GPIB-USB board on this USB port with 460800 bps, 8-N-1
ser=Serial('/dev/ttyUSB0',460800,8,'N',1,timeout=1)  
f = open('K196-log.csv', 'w')
print ("Keithley 196 log v0.12 May 26 2018 JPB")
f.write ("date_time, volts\n")
f.write ("# Keithley 196 log v0.12 May 26 2018 JPB\n")

# print (doCmd(cmd1))
# print (doCmd(cmd2))

fctr = 0                     # how many lines to write before flush
while True:
    ser.read(255)            # clear out existing buffer & throw it away
    ser.write(cmd)                 # send query to instrument
    buf = ser.readline()         # string terminated with '\n'
    buffer = buf.split()[0]   # get rid of the \r\n characters at the end
    outbuf = str(datetime.datetime.now()) + ', ' + buffer[4:]  # remove first 4 char 'NDCV'
    # print (outbuf)
    f.write (outbuf)
    f.write (eol_str)
    fctr += 1
    if (fctr > 10):
      f.flush()
      fctr = 0
    time.sleep(1)                        # from 'time' to wait this many seconds
    
f.close                  # close log file
ser.close()            # close serial port when done. If we ever are...
