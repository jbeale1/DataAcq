#!/bin/bash

# transfer all mp3 files older than 6 sec to storage directory
# then rsync (transfer) from storage dir to a different machine, and delete local copy

RECDIR="/home/pi/audio"
STOREDIR="/home/pi/audio/OLD"
RMACHINE="john@john-Z83-4"
RTODIR="/home/john/doppler"  # remote directory on other machine

echo $RECDIR $STOREDIR $RMACHINE:$RTODIR
find $RECDIR -maxdepth 1 -name "*.mp3" -type f -mmin +0.1 -exec mv -t $STOREDIR {} +
# scp $TODIR/*.mp3 john@john-Z83-4:/$RTODIR
rsync --remove-source-files -a $STOREDIR/ $RMACHINE:$RTODIR/
