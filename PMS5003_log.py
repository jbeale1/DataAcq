#!/usr/bin/python3
# log serial output from Arduino code which needs to get Unix epoch
# time sync, then reports current time.now() value and PMS5003 data
# J.Beale 12-July-2020 this was tested using Python 3.6.8

import serial, time
from datetime import datetime
import subprocess                # to execute external shell script

 
#SERPORT = "/dev/ttyUSB0" # input serial port
#SERPORT = "/dev/ttyACM0" # input serial port
SERPORT = "/dev/ttyACM1" # input serial port

#ONMOTION = "/home/john/Moteino/grab-CAM10.sh"
logFilePath = "/home/john/pmQuality/LogPM3.csv"

# Possible timeout values:
#    1. None: wait forever, block call
#    2. 0: non-blocking mode, return immediately
#    3. x, x is bigger than 0, float allowed, timeout block call

ser = serial.Serial()
ser.port = SERPORT
ser.baudrate = 9600
ser.bytesize = serial.EIGHTBITS #number of bits per bytes
ser.parity = serial.PARITY_NONE #set parity check: no parity
ser.stopbits = serial.STOPBITS_ONE #number of stop bits
ser.timeout = None          #block read
#ser.timeout = 1            #non-block read
ser.xonxoff = False      #disable software flow control
ser.rtscts = False       #disable hardware (RTS/CTS) flow control
ser.dsrdtr = False       #disable hardware (DSR/DTR) flow control
ser.writeTimeout = 2     #timeout for write

csvHeader = "time,0p3"

try: 
    ser.open()
except Exception as e:
    print ("error open serial port: " + str(e))
    exit()

with open(logFilePath, 'a') as fLog:  # append to logfile, if exists

  if ser.isOpen():
    try:
       now = datetime.now()
       dtString = now.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
       print("%s" % csvHeader)
       fLog.write("%s\n" % csvHeader)
       fLog.write("# Motion logfile start: %s\n" % dtString)

       ser.flushInput()  #flush input buffer, discarding all its contents
       ser.flushOutput() #flush output buffer, aborting current output 
                 #and discard all that is in buffer

       # write data
       #dataOut = "s\n"
       #ser.write(str.encode(dataOut))
       #print("Data sent: %s" % dataOut)
       #time.sleep(0.5)  #give the serial port sometime to receive the data

       numOfLines = 0

       # token1 = ""  # search for this string in data received
       token1 = "Reading"  # search for this string in data received

       recLine = ser.readline().decode(errors='ignore')   # convert to default string type
       unixTime = datetime.now().timestamp()
       # print("First line: %s" % recLine) # header "time, pm03"
       # print("Unix Time = %d" % unixTime)   # DEBUG show Python's idea of epoch time
       fudgeFactor = 1   # empirically observed to align clocks
       timestr = ("T" + ("%d\n" % (unixTime + fudgeFactor))).encode('utf-8')
       # print(timestr)
       ser.write(timestr) # send Unix epoch time sync string to Arduino

       while True:
          recLine = ser.readline().decode(errors='ignore')   # convert to default string type
          #now = datetime.now()
          #dtString = now.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
          unixTime = datetime.now().timestamp()

          fPos = recLine.find(token1) # found header token?
          if (fPos >= 0):
            print("# ******** %s Found %s at %d" % (dtString,token1,fPos))
            continue
          #  print("Now calling %s" % ONMOTION)
          #  retval = subprocess.call([ONMOTION]) # blocks until return

          #print("%.3f, %s" % (unixTime,recLine), end="")
          print("%s" % (recLine), end="")
          #fLog.write("%.3f, %s" % (unixTime,recLine))
          fLog.write("%s" % (recLine))
          #fLog.flush()                              # data is occasional but time-sensitive

          

          numOfLines = numOfLines + 1
          #if (numOfLines >= 15):
          #  break
 
       ser.close()
    except Exception as e1:
        print ("error communicating...: " + str(e1))

  else:
    print("cannot open serial port ")
