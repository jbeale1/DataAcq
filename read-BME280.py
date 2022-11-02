#!/usr/bin/env python3

# read two BME280 sensors on Raspberry Pi
# Code based on https://github.com/pimoroni/bme280-python/all-values.py
# J.Beale 1-Nov-2022

import time
import datetime
try:
    from smbus2 import SMBus
except ImportError:
    from smbus import SMBus
from bme280 import BME280
# ================================

fnameOut = "Bme-log1.csv"

# Initialise the BME280 parts
bus3 = SMBus(3)
bme1 = BME280(i2c_dev=bus3)
bus4 = SMBus(4)
bme2 = BME280(i2c_dev=bus4)

n = 183  # how many samples to average
lineCount = 0

with open(fnameOut, 'a') as f:
  headers = "date_time, epoch, T1,T2, P1,P2, H1,H2"
  f.write("%s\n" % headers)
  while True:
    t1=0.0; p1=0.0; h1=0.0
    t2=0.0; p2=0.0; h2=0.0
    for i in range(n):
      t1 += bme1.get_temperature()
      p1 += bme1.get_pressure()
      h1 += bme1.get_humidity()
      t2 += bme2.get_temperature()
      p2 += bme2.get_pressure()
      h2 += bme2.get_humidity()
      time.sleep(0.05)
    t1 /= n; p1 /= n; h1 /= n
    t2 /= n; p2 /= n; h2 /= n

    now = datetime.datetime.now()
    sec = time.time()

    outString = ("%s, %d, %5.3f,%5.3f,%5.3f,%5.3f,%5.3f,%5.3f" % 
      ( now, sec, t1,t2,p1,p2,h1,h2 ) )
    print(outString)
    f.write('%s\n' % outString)

    lineCount += 1
    if (lineCount % 10) == 0:
      f.flush()
