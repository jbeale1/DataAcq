# Excercise ADS1256 control program
# J.Beale 22-Feb-2025

import serial
import time
import os
import numpy as np

VERSION = "ADS1256 log v0.2 22-Mar-2025 JPB"


def sendSer(s):
    ser.write(s.encode())
    print(f"Sent: {s}")
    while True:
        response = ser.readline().decode().strip()
        if response == "":
            break
        print(response)

# -----------------------------------------------

outDir = r"/home/john/Documents/source"
logFile = "adc1256-log.csv"
logPath = os.path.join(outDir, logFile)
#port = "/dev/ttyACM2"  # Replace with your serial port
port = "/dev/ttyACM0"  # Replace with your serial port
baudrate = 115200
timeout = 0.6  # seconds

"""
  DRATE_30000SPS,
  DRATE_15000SPS,
  DRATE_7500SPS,
  DRATE_3750SPS,
  DRATE_2000SPS,
  DRATE_1000SPS,
  DRATE_500SPS,
  DRATE_100SPS,
  DRATE_60SPS,
  DRATE_50SPS,
  DRATE_30SPS,
  DRATE_25SPS,
  DRATE_15SPS,
  DRATE_10SPS,
  DRATE_5SPS,
  DRATE_2SPS
}
"""

# s1 = "F7" # set sample rate to 100 Hz
#s1 = "F8" # set sample rate to 60 Hz
s1 = "F10" # set sample rate to 30 Hz
#s1 = "F12" # set sample rate to 15 Hz
#s1 = "F3" # set sample rate to 3750 Hz
s2 = "P6" # set PGA gain to max (PGA64)
s3 = "Md0"  # set Mux to diff set 0 (A0,A1 pair)
s4 = "J15"  # get binary packet with N 4-byte words
eol_str = "\n"  # end of line string in file output
pktlen = 60  # how many bytes in binary packet

# VREF = 2.500 volts, PGA = 64
# voltage = ((2 * VREF) / 8388608) * raw_counts / (pow(2, PGA));

print (VERSION)
try:
    # Open the serial port
    ser = serial.Serial(port, baudrate, timeout=timeout)
    print(f"Serial port {port} opened successfully")
    sendSer('s')
    time.sleep(4)

    while True:
        response = ser.readline().decode().strip()
        if (response == ""):
            break
        print(response)

    sendSer(s1)    
    sendSer(s2)    
    sendSer(s3)    

    f = open(logPath, 'a')  # open log file    
    f.write ("epoch_time, Vavg, uVstd, vMin, vMax\n")
    f.write ("# %s" % VERSION)

    ser.write(s4.encode())
    print("Starting run")


    while True:
        inbuf = ser.read(pktlen)
        byteCount = len(inbuf)
        if (byteCount != pktlen):
            continue

        #hex_list = ['{:02x} '.format(byte) for byte in inbuf]
        #hex_string = ''.join(hex_list)
        #print(hex_string)
        #continue

        wordBufB = np.frombuffer(inbuf, dtype=np.int32)
        wordBuf = wordBufB.byteswap()
        epoch = time.time()         
        wordString = ",".join(map(str, wordBuf))
        outbuf = ("%.2f" % epoch) + ', ' + wordString # assemble output string
        print (outbuf)
        f.write (outbuf)
        f.write (eol_str)

     

except serial.SerialException as e:
    print(f"Error: {e}")
except Exception as e:
    print(f"Error: {e}")

finally:
    f.close
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")
