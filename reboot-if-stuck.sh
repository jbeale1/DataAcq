#!/bin/bash

# Confirm audio logging is still active and reboot if not

todir="/dev/shm/audio"  # check if this directory exists
ftype="*.mp3"           # check if it contains this type of file
logfile="/home/pi/restart.log"

take_action() {
  date >> $logfile
  if [ ! -d $todir ] ; then
    echo "$todir missing, starting countdown..." >> $logfile
  else
    echo "$todir contains no $ftype, starting countdown..." >> $logfile
  fi
  sleep 120  # delay to allow for manual changes if testing
  date >> $logfile
  echo "=== Now rebooting..." >> $logfile
  sudo reboot
}

# --------------------------------------------

# does audio directory exist?
if [ ! -d $todir ] ; then  
  take_action
  exit 1
fi

# does it contain at least one file of this kind?
for f in $todir/$ftype; do  
    [ ! -e "$f" ] && take_action
    break
done

# date >> $logfile
