# Read pair of AHT10 Temp,RH sensors on Pi Pico / micropython
# J.Beale 18-Nov-2022

import utime
from machine import Pin, I2C
import ahtx0 # https://github.com/targetblank/micropython_ahtx0

i2c0 = I2C(0, scl=Pin(17), sda=Pin(16), freq=400_000)
i2c1 = I2C(1, scl=Pin(15), sda=Pin(14), freq=400_000)

sensor0 = ahtx0.AHT10(i2c0)
sensor1 = ahtx0.AHT10(i2c1)
rtc=machine.RTC()
timestamp = rtc.datetime()  # does this set time from HW RTC?
timestring="%04d-%02d-%02d %02d:%02d:%02d"%(timestamp[0:3] +
                                                timestamp[4:7])
    
avgCount = 10  # how many readings to average together
utime.sleep(4)
print("epoch, T0,T1, RH0,RH1")
print("# Start: %s" % timestring)

while True:
    Tsum0 = 0
    Hsum0 = 0
    Tsum1 = 0
    Hsum1 = 0
    for i in range(avgCount):
        Tsum0 += sensor0.temperature
        Tsum1 += sensor1.temperature
        Hsum0 += sensor0.relative_humidity
        Hsum1 += sensor1.relative_humidity
        utime.sleep(1)
        
    T0 = Tsum0 / avgCount
    H0 = Hsum0 / avgCount    
    T1 = Tsum1 / avgCount
    H1 = Hsum1 / avgCount    
    epoch=utime.time() # UNIX epoch, in local time zone
    print("%d, %0.3f,%0.3f, %0.3f,%0.3f" % (epoch,T0,T1,H0,H1))
    
