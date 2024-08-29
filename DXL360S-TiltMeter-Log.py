#!/usr/bin/python3
# Python code to read data via USB serial port on DXL360S 0.01 degree tiltmeter
# It has nonstandard USB port with no USB signal. 
# Red and Black are +5V input (battery charge) and GND.
# There is 3.3V logic UART signal output on USB D- (white wire) referenced to ground.
# USB D+ (green) may be unused; not clear.
# By default, module will power off after being still for a while.
# 2024-Aug-28 J.Beale

from serial import *
import time, datetime, os, sys

LOGDIR = "/home/john/Documents/Spectrometer"
LOGFILE = "AngleLog.csv"
PORT = "/dev/ttyUSB0"
VERSION  = "DXL360S TiltMeter 8/28/2024 JPB"
CSV_HEADER = "date_time, epoch, reading"
eol_str = "\n"  # end of line string in file output

#timeInterval = 1   # how many seconds between readings
readingsToAverage = 20  # about 10 readings per second

# at a rate of x samples per second
ser=Serial(PORT,9600,8,'N',1,timeout=0.04)  # serial input
#ser=Serial(PORT,9600,8,'N',1)  # serial input

dstring = datetime.datetime.today().strftime('%Y-%m-%d_%H%M%S_')

fnameout = os.path.join(LOGDIR, (dstring+LOGFILE))
print("Writing data to %s" % fnameout)
f = open(fnameout, 'w')  # open log file
print("%s" % VERSION)
print("%s" % CSV_HEADER)
f.write ("%s\n" % CSV_HEADER)
f.write ("# %s\n" % VERSION)

ser.read(255)            # clear out existing buffer & throw it away
lines = 0                # how many lines we've read
xSum = 0                # sum of X angle values read so far
readings = 0            # valid readings so far
while True:
    buf = ""
    t = datetime.datetime.now()

    # DXL360S format: X-1234Y-1234
    buf = ser.read(12)    # get data packet which should be 12 
    recCount = len(buf)
    if (recCount != 12):  # we insist that only 12 characters are acceptable
        # print(recCount)
        continue

    #print(".",end="")

    p1 = buf.find(0x58) # 'X' = 0x58  example string: "X-0005Y-0001"
    p2 = buf.find(0x59) # 'Y' = 0x59 example string: "X-0005Y-0001"
    if (p1<0) or (p2 < 0):  # if we couldn't find X and Y, something's wrong.
        print(buf.decode("utf-8"))
        print("Data format error: p1: %d  p2: %d" % (p1,p2))
        continue

    tEpoch = (int(time.time()*10))/10.0         # time() = seconds.usec since Unix epoch
    tNow = datetime.datetime.now()


    xString = (buf[p1+1:p2].decode("utf-8")) # angle in +dd.dd format (decimal degrees)
    yString = (buf[p2+1:].decode("utf-8"))   # angle in +dd.dd format (decimal degrees)
    if (xString == '') or (yString == ''):
        continue
    try:
        xval = int(xString)
        yval = int(yString)
        # print("%d, %d" % (xval,yval))
        xdeg = xval/100.0  # packet comes in units of 1/100 degree
        xSum += xdeg
        readings += 1
    except Exception as e:
        print("Unexpected error:", sys.exc_info()[0])
        print("Input string: %s" % buf.decode("utf-8"))

    if (readings >= readingsToAverage):
        xdeg = xSum / readings
        readings = 0
        xSum = 0
        buffer = ("%.3f" % (xdeg))
        
        # buffer = buf.decode("utf-8").rstrip()         # string terminated with '\n'
        outbuf = str(datetime.datetime.now())[:-3] + "," + \
        ("%12.1f" % tEpoch ) + "," + buffer
        print (outbuf)
        f.write (outbuf)
        f.write (eol_str)
        lines += 1
        if (lines % 50 == 0):  # save it out to actual file, every so often
            f.flush()


f.close                  # close log file
ser.close()            # close serial port when done. If we ever are...
