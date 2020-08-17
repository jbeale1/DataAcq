#!/bin/bash

# Extract speed of sound, and range from Sonar log file to CSV
# Also, add Unix Epoch timestamp column, based on date & time

wdir="/home/john/Music"
iname="sonarDat.txt"
oname="ProcSonar.csv"

echo "date time , m/s , m , epoch" > $wdir/$oname  # header line: speed of sound, range in m

# Example line in log file:
# /dev/shm/20200817_095903.wav   96 kHz  30.000 sec  146 pulses D1: 3.051 (0.0008) m  D2: 5.580 (0.0007) m  340.213 m/s D2a: 5.5996 m

# split pathname into date,time as YYYYMMDD HHMMSS
# then convert date to Unix Epoch, from "MM/DD/YYYY HH:MM:SS TZOFF"
# date -d '08/16/2020 10:22:00 -0700' +"%s"


cat $wdir/$iname | 
   awk '
        function basename(file, a, n) {
        n = split(file, a, "/")
        return a[n]
        }
        { print basename($1) , "," , $16 , "," , $19; }' | 
   cut -c -15,20- | 
   sed 's/_/ /' |
   awk '{print substr($1,5,2)"/"substr($1,7,2)"/"substr($1,1,4) , 
         substr($2,1,2)":"substr($2,3,2)":"substr($2,5,2),"-700",$3,$4,$5,$6; }' |
   awk '{ printf("%s, ", $0);
         ep=$(system("date +%s -d \""$1" "$2" "$3"\"") );
         printf("%s", $ep ); }' >> $wdir/$oname

# transmit results to remote host
scp ProcSonar.csv pi@192.168.1.107:Documents/.
