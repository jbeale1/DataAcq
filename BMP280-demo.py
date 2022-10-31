#!/usr/bin/env python

# based on Adafruit example code for BMP280 I2C pressure/temp sensor
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

# Initialise the BMP280
bus = SMBus(1)
bmp280 = BMP280(i2c_dev=bus)

nAvg = 187  # how many readings to average (about 1 group / 10 sec)

lineCount = 0

with open(fnameOut, 'a') as f:
  while True:
    degC = 0
    pres = 0
    for i in range(nAvg):
      degC += bmp280.get_temperature()
      pres += bmp280.get_pressure()
      time.sleep(0.05)

    degC /= nAvg
    pres /= nAvg
    now = datetime.datetime.now()
    sec = time.time()

    outString = ('%s, %d, %05.3f, %05.3f' % (now, sec, degC, pres))
    print(outString)
    f.write('%s\n' % outString)

    lineCount += 1
    if (lineCount % 10) == 0:
      f.flush()

