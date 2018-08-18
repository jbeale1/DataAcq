#!/bin/bash

# Record mono (1-channel) audio at 8kHz sample rate, split into 15-minute files
# Files will start on even 15-minute boundaries per system time, after the first one
# suitable for cheap USB-Audio adapters (mic mono-in, headphone stereo-out)

# must use ffmpeg option -nostdin to enable running in background
# otherwise ffmpeg has to run in foreground to monitor keyboard for "Q" to quit

ffmpeg -nostdin -loglevel quiet -f alsa -ac 1 -ar 8000 -i plughw:1 -map 0:0 -acodec libmp3lame \
  -b:a 64k -f segment -strftime 1 -segment_time 900 -segment_atclocktime 1 \
   ChA_%Y-%m-%d_%H-%M-%S.mp3
