# Read out WitMotion JY-ME02-485 absolute angle encoder
# (tested to actually have 0.01 degree accuracy, at least for some small angles)
# J.Beale 9/15/2024

# Witmotion Encoder command (9600 bps using USB-RS485 converter)
# Send this hex byte string:  50 03 00 04 00 20 08 52
# Receive 69 bytes back. B29,B30 are the 15-bit angle value, B28 is rotation #
# Receive string always starts with (B0...B5):
# 50 03 40 00 02 00 
# encoder resolves 360 degree angular position into 15-bit word: 0000 to 32767
# data packet also includes angular rate and sensor temperature

import serial, os
import time, datetime
import struct # unpack bytes to int

averages = 94  # how many readings to average together

maxCounts = 32768 # number of counts from 15-bit sensor (0..maxCounts-1)
degPerCount = 360.0 / (maxCounts) # angular scale factor of sensor

LOGDIR = r"C:\Users\beale\Documents\Telescope"
LOGFILE = "AngleEncLog.csv"
VERSION  = "JY-ME02 Encoder 9/15/2024 JPB"
CSV_HEADER = "epoch, angle, degC"
EOL = "\n" # end of line for log file

dstring = datetime.datetime.today().strftime('%Y-%m-%d_%H%M%S_')
fnameout = os.path.join(LOGDIR, (dstring+LOGFILE))
print("Writing data to %s" % fnameout)
f = open(fnameout, 'w')  # open log file
print("%s" % VERSION)
print("%s" % CSV_HEADER)
f.write ("%s\n" % CSV_HEADER)
f.write ("# %s\n" % VERSION)

if __name__ == "__main__":
    port = 'COM6'  # or '/dev/ttyUSB0' etc. on Linux
    baudrate = 9600
    recLen = 69   # number of bytes in response from JYME02
    # command to encoder to readout angle, angular rate, temperature:
    cmd_bytes = bytes.fromhex('50 03 00 04 00 20 08 52')  
    # response packet should start with these bytes:
    header = bytes.fromhex('50 03 40 00 02 00')

    with serial.Serial(port, baudrate, timeout=0.1) as ser:        
        ser.read(200) # clear out any data in receive buffer

        while True:
            angleSum = 0 # accumulated sum of angle readings
            tempSum = 0  # accumulated sum of temperature readings (deg.C)
            validReads = 0 # how many good readings so far
            while (validReads < averages):            
                ser.write(cmd_bytes)
                recData = ser.read(recLen)
                recCount = len(recData)  
                firstSix = recData[0:6]              
                # does response packet look good?
                if (recCount != recLen) or (firstSix != header):                 
                    bstr = firstSix.hex()
                    outs = ("# Receive error: %d: %s\n" % (recCount,bstr))
                    print(outs)
                    f.write(outs)
                    ser.read(200) # clear out any bad data in receive buffer
                    continue
                angleR = recData[29:31] # 2 bytes of angle data
                tempR = recData[35:37]  # 2 bytes of temperature data
                angle = struct.unpack('>H', angleR)[0] # convert to 16-bit integer                
                if (angle > (3*maxCounts)/4):  # put split at -90deg
                    angle = angle-maxCounts

                # temp in integer hundredths of deg.C: 09D9 means 25.21 C
                temp = struct.unpack('>H', tempR)[0] # convert to 16-bit integer                
                angleSum += angle
                tempSum += temp
                validReads += 1

            angleDeg = degPerCount * (angleSum / validReads) # angle in degrees
            tempC = (tempSum / validReads) / 100.0 # temp in degrees C
            tEpoch = time.time()
            outs=("%0.1f, %5.3f, %5.3f" % (tEpoch,angleDeg,tempC))
            print(outs)
            f.write(outs+EOL)
