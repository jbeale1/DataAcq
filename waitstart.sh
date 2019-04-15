#!/bin/bash

# wait for date/time to be correct before starting serial logger

# timedatectl status > /home/pi/FMES/cron-start.txt

tail -50 /var/log/syslog | grep -q "Started Time & Date Service"
status=$?
while [ $status -ne 0 ]
do
    sleep 1
    tail -50 /var/log/syslog | grep -q "Started Time & Date Service"
    status=$?
done

sudo /home/pi/FMES/reset-teensy.sh
/home/pi/FMES/serlog.py &
