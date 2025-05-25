# Excercise ADS1256 control program
# J.Beale 22-Feb-2025

import serial
import time
import os
import numpy as np
from datetime import datetime, timedelta

# Handle zoneinfo import for Python 3.9
try:
    from zoneinfo import ZoneInfo
except ImportError:
    from backports.zoneinfo import ZoneInfo

VERSION = "ADS1256 log v0.3 24-May-2025 JPB"


def sendSer(s):
    ser.write(s.encode())
    print(f"Sent: {s}")
    while True:
        response = ser.read()
        if not response:  # empty response means timeout
            break
        try:
            # Only try to decode if it looks like text
            if all(b <= 0x7F for b in response):
                print(response.decode(), end="")
            else:
                print(f"<binary: {response.hex()}>", end="")
        except UnicodeDecodeError:
            # Fallback for any decode errors
            print(f"<binary: {response.hex()}>", end="")

# -----------------------------------------------

outDir = r"/home/john/Documents/source"

now = datetime.now()
fstr = now.strftime("%Y%m%d-%H%M")
logFile = ("%s_adc1256-log2.csv" % fstr)
logPath = os.path.join(outDir, logFile)
port = "/dev/ttyACM2"  # Replace with your serial port
#port = "/dev/ttyACM0"  # Replace with your serial port
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
s1 = "F8" # set sample rate to 60 Hz
#s1 = "F10" # set sample rate to 30 Hz
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

def get_next_rotation_time():
    """Return the next noon or midnight timestamp"""
    now = datetime.now(ZoneInfo("America/Los_Angeles"))
    if now.hour < 12:
        next_rotation = now.replace(hour=12, minute=0, second=0, microsecond=0)
    else:
        next_rotation = (now.replace(hour=0, minute=0, second=0, microsecond=0) + 
                        timedelta(days=1))
    return next_rotation

try:
    # Open the serial port
    ser = serial.Serial(port, baudrate, timeout=timeout)
    time.sleep(2)  # Wait for the serial port to initialize
    if not ser.is_open:
        raise serial.SerialException(f"Failed to open serial port {port}")
    
    print(f"Serial port {port} opened successfully")
    ser.reset_input_buffer()  # Clears residual bytes
    ser.reset_output_buffer()  # Clears residual bytes
    sendSer('s')
    time.sleep(3)

    # Modified loop to handle binary data
    while True:
        response = ser.read()  # Read one byte at a time
        if not response:  # Empty response means timeout
            break
        # Skip trying to decode binary data
        
    sendSer(s1)    
    sendSer(s2)    
    sendSer(s3)    

    f = open(logPath, 'a')  # open log file    
    f.write ("epoch_time, Vavg, uVstd, vMin, vMax\n")
    f.write ("# %s\n" % VERSION)
    print("Writing to logfile: %s" % logPath)

    ser.write(s4.encode())
    print("Starting run")


    next_rotation = get_next_rotation_time()
    while True:
        now = datetime.now(ZoneInfo("America/Los_Angeles"))
        
        # Check if it's time to rotate
        if now >= next_rotation:
            f.close()
            print(f"Log file {logPath} closed at rotation time")
            
            # Create new log file
            fstr = now.strftime("%Y%m%d-%H%M")
            logFile = f"{fstr}_adc1256-log2.csv"
            logPath = os.path.join(outDir, logFile)
            f = open(logPath, 'a')
            f.write("epoch_time, Vavg, uVstd, vMin, vMax\n")
            f.write(f"# {VERSION}\n")
            print(f"New log file opened: {logPath}")
            
            # Calculate next rotation time
            next_rotation = get_next_rotation_time()

        inbuf = ser.read(pktlen)
        byteCount = len(inbuf)
        if (byteCount != pktlen):  # skip if not enough bytes read
            continue

        wordBufB = np.frombuffer(inbuf, dtype=np.int32)   # Convert raw bytes to 32-bit integers
        wordBuf = wordBufB.byteswap()                     # Swap byte order for endianness
        epoch = time.time()                               # Get current Unix timestamp
        wordString = ",".join(map(str, wordBuf))          # Convert integers to comma-separated string
        outbuf = ("%.2f" % epoch) + ', ' + wordString     # Add timestamp to start of string
        print (outbuf)
        f.write (outbuf)
        f.write (eol_str)

     

except serial.SerialException as e:
    print(f"Error: {e}")
except Exception as e:
    print(f"Error: {e}")

finally:
    if 'f' in locals() and not f.closed:
        f.close
        print("Log file %s closed" % logPath)
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port %s closed" % port)
