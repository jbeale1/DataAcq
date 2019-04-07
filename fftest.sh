#!/bin/bash

# crontab: check if ffmpeg process still running and restart process if not
# * * * * * sleep 30; /home/pi/audio/fftest.sh

log="/home/pi/audio/ff_log.txt"

# check if ffmpeg process is running
if [[ $(ps aux | grep ffmpeg | grep segment) ]]; then
    : # echo "ffmpeg process is running"
else
    date >> $log
    echo "no ffmpeg process found - restarting" >> $log
    sudo -u pi /home/pi/audio/startaudio.sh
fi


# === startaudio.sh file below:
#!/bin/bash
#mkdir /run/shm/audio
#mkdir /run/shm/audio/OLD
# start record process running, even if login shell exits
#nohup /home/pi/audio/rec2.sh >> /dev/null 2>&1 &
