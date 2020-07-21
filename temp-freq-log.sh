#!/bin/bash

# temperature log file
logf="/home/pi/tlog.csv"

#   check if time sync is active with:  systemctl status systemd-timesyncd.service 
# get system clock frequency offset
x=`timedatectl show-timesync | grep Frequency`
ar=($x)              # array ('Frequency=123' 'whatever=something' 'etc')
freq=${ar[0]#*=}     # "123"

# record time/date, CPU temp, external temp, external RH (from DHT22 sensor), frequency offset
# readtemp.c based on wiringPi DHT11 / DHT22 code example
# Example log file line. CPU temp units are milli-degrees:
# 20200721_113902,52616,29.1, 41.4,-121708

(date +"%Y%m%d_%H%M%S,`cat /sys/class/thermal/thermal_zone0/temp`" | tr '\n' ',') >> $logf
(/usr/local/bin/readtemp 7 25 0 | tr '\n' ',') >> $logf
(echo $freq) >> $logf
