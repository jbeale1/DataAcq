#!/bin/bash
# Reset any connected Teensy 3.2 device, knowing its USB VENDOR:PRODUCT id numbers
# Must be run as root (sudo)

VENDOR=16c0
PRODUCT=0483

for DIR in $(find /sys/bus/usb/devices/ -maxdepth 1 -type l); do
  if [[ -f $DIR/idVendor && -f $DIR/idProduct &&
                $(cat $DIR/idVendor) == $VENDOR && $(cat $DIR/idProduct) == $PRODUCT ]]; then
            # echo "Resetting USB Device $DIR"
            echo 0 > $DIR/authorized
            sleep 0.5
            echo 1 > $DIR/authorized
  fi
done
