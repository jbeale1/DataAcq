#!/bin/bash

# Get Temp (deg. F) and Humidity (%) from pp-Code Watchman wifi sensor
# made from NodeMcu V3 wifi board + AM2302 (wired DHT22) sensor
# https://www.amazon.com/pp-Code-Temperature-sensor-monitor-party-detect/dp/B07D23G27R/

IP=192.168.1.116     # IP address of sensor device on LAN
IDNUM=abcd12341234   # unique ID of sensor (label on bottom)
LOG="tlog.txt"       # output file with data in CSV format

OUT="http://"$IP"/"$IDNUM"&Stats"  # http string to retrieve temp & humid. readings

# output format:   YYYY-MM-DD HH:MM:SS , TEMP(F) , RH(%)

while [ true ]; do
  sleep $((60 - $(date +%S) ))  # wait until top of next minute
  td=$(date "+%F %T ,")  # current date and time

  IN=$( wget $OUT -qO- )
  IFS=':' read -ra AR <<< "$IN"
  
  echo -n $td >> $LOG
  s=${AR[1]} ;  echo -n " ${s%%F*} , " >> $LOG
  s=${AR[2]} ;  echo "${s%%\%*}" >> $LOG
  
done
