#!/bin/bash

# record Raspberry Pi CPU temperature and system clock offset to a file

# temperature log file
logf="/home/pi/templog/tlog.csv"

#   check if time sync is active with:  systemctl status systemd-timesyncd.service 
# get system clock frequency offset

# === use below code if you're running systemd-timesyncd.service
#x=`timedatectl show-timesync | grep Frequency`
#ar=($x)              # array ('Frequency=123' 'whatever=something' 'etc')
#freq=${ar[0]#*=}     # "123"
# =================


# === use below code if you're running chronyd ====
x=`chronyc tracking | grep Frequency`
ar=($x)              # array ('Frequency' ':' '12.3' 'ppm' 'slow')
fnum=${ar[2]}        # "12.3"
fs=${ar[4]}
if [ "$fs" = "fast" ]; then  # no sign, just string "fast", "slow"
  freq="+"
else
  freq="-"
fi
freq+=$fnum
# =================


# === use below code if you're running ntpd ====
#freq=`cat /var/lib/ntp/ntp.drift`  # get current freq offset in PPM if ntpd is in use
# =================

# record time/date, CPU temp, external temp, external RH (from DHT22 sensor), frequency offset
# readtemp.c based on wiringPi DHT11 / DHT22 code example
# Example log file line. CPU temp units are milli-degrees:
# 20200721_113902,52616,29.1, 41.4,-121708

(date +"%Y%m%d_%H%M%S,`cat /sys/class/thermal/thermal_zone0/temp`" | tr '\n' ',') >> $logf
# (/usr/local/bin/readtemp 7 25 0 | tr '\n' ',') >> $logf
(echo $freq) >> $logf
