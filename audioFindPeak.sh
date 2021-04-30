#!/bin/bash

# /home/pi/Audio/findPeak.sh - J.Beale 30-Apr-2021

# crontab entry to run every 15 minutes, offset by 2 min:
# 2-59/15 * * * * /home/pi/Audio/findPeak.sh

# run 4x per hour, at least 1 minute after the 1/4 hour
# retreive audio file of most recent 15-minute period
# find Peak RMS value during that interval

# Example line of data output: epoch, date time, value
# 1619808300,2021-04-30 11:45:00,-40.56

tmpdir="/dev/shm"
outfile=$tmpdir"/pkAlog.csv"

# get the current time as seconds from Unix Epoch
eNow=$(date +%s)

# find the epoch seconds value 15 minutes ago
ePrev=$(expr $(($eNow - 900)) )
dtPrev=$(date -d@$ePrev +"%Y-%m-%d")

# get hour and minute of recording start time
hour=$(date -d@$ePrev +"%H")
min=$(date -d@$ePrev +"%M")

#echo "prev minute: "$min
qn=$(expr $(($min / 15)))
min=$(expr $(($qn * 15)) )
# handle special case < 10 minutes, need leading "0"
if [ "$min" = "0" ]; then 
  min="00"
fi

fname="ChA_"$dtPrev"_"$hour"-"$min"-00.flac"
#echo "fname: "$fname

# date/time in format like "2021-04-19 21:15:00"
dt=$dtPrev" "$hour":"$min":00"
epoch=`date +%s -d "$dt"`

# get audio recordings file from remote  host
rsync -aqe ssh john@john-Z83-4.local:/media/john/Seagate4GB/MINIX-John/Audio1/$fname $tmpdir/$fname

# extract 6th column, which is  RMS Pk dB value for Right Channel
pk=$(sox $tmpdir/$fname -n stats 2>&1 | grep "RMS Pk dB" | awk '{print $6}')

# save the line of data as: date time, RMS_Pk
echo $epoch,$dt,$pk >> $outfile

# clean up the audio file we fetched
rm $tmpdir/$fname
