#!/bin/bash

# Record mono (1-channel) audio at 8kHz sample rate, split into 15-minute files
# Files will start on even 15-minute boundaries per system time, after the first one
# suitable for cheap USB-Audio adapters (mic mono-in, headphone stereo-out)

# must use ffmpeg option -nostdin to enable running in background
# otherwise ffmpeg has to run in foreground to monitor keyboard for "Q" to quit

ffmpeg -nostdin -loglevel quiet -f alsa -ac 1 -ar 8000 -i plughw:1 -map 0:0 -acodec libmp3lame \
  -b:a 64k -f segment -strftime 1 -segment_time 900 -segment_atclocktime 1 \
   ChA_%Y-%m-%d_%H-%M-%S.mp3

# ============== alternate version ===========================
#!/bin/bash

# location to store audio files
#todir="/run/shm/audio/"
todir="./"

# number of seconds each .mp3 file should last
duration="900"

# which audio device card number to work with
cnum="1"

# set mic input level to ~ 0 dB (which is strangely 15 out of 100)
amixer -c "$cnum" cset numid=3 15

# Record fixed-length MP3 files from audio device continually
ffmpeg -nostdin -loglevel quiet -f alsa -ac 2 -ar 44100 -i plughw:"$cnum" -map 0:0 -acodec libmp3lame \
  -b:a 256k -f segment -strftime 1 -segment_time "$duration" -segment_atclocktime 1 \
   "$todir"ChA_%Y-%m-%d_%H-%M-%S.mp3 &

sudo renice -n -19 $!
