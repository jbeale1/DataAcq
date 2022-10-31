#!/usr/bin/env python

# Read several sensors, by creating more I2C buses on GPIO
# https://www.instructables.com/Raspberry-PI-Multiple-I2c-Devices/
# Sensor code based on Adafruit BMP280 example
# J.Beale 2022-Oct-30

import datetime
import time
from bmp280 import BMP280
try:
    from smbus2 import SMBus
except ImportError:
    from smbus import SMBus
# ==================================

fnameOut = "tp-log1.csv"

bmp280 = []       # will be list of I2C devices
i2cDev = [1,3,4]  # I2C buses with devices

# Initialise the BMP280 on each bus
for bus in i2cDev:
  bmp280.append( BMP280(i2c_dev=SMBus(bus)) )

nAvg = 187  # how many readings to average

lineCount = 0

with open(fnameOut, 'a') as f:
  headers = "date_time, epoch, T1,T2,T3, P1,P2,P3"
  f.write("%s\n" % headers)
  while True:
    cnt = len(i2cDev) # how many devices there are
    degC = [0] * cnt
    pres = [0] * cnt
    for n in range(nAvg):
      for i in range(cnt):
        degC[i] += bmp280[i].get_temperature()
        pres[i] += bmp280[i].get_pressure()
      time.sleep(0.05)

    for i in range(cnt): 
      degC[i] /= nAvg
      pres[i] /= nAvg

    now = datetime.datetime.now()
    sec = time.time()


    outString = ('%s, %d, %05.3f, %05.3f, %05.3f, %05.3f, %05.3f, %05.3f' % (now, sec, 
      degC[0], degC[1], degC[2],
      pres[0], pres[1], pres[2]))
    print(outString)
    f.write('%s\n' % outString)
    lineCount += 1
    if (lineCount % 10) == 0:
      f.flush()
