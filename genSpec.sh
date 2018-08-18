#!/bin/bash

# use SOX to generate spectrograms of audio files

INDIR="/home/john/doppler"       # read input sound files here
OUTDIR="/home/john/doppler/out"  # put spectrogram files here
SR="4k"                          # resample file to this rate (Hz) before generating spectrogram

XSIZE=1000   # width of spectrogram (pixels)
YSIZE=300    # height of spectrogram (pixels)

for f in $INDIR/*.mp3 ; do
  f1=`basename "$f"`         # get just filename, without the path
  FN="${f1%.*}"              # get base filename, without .mp3 extension
  # echo $INDIR/$FN.mp3 $OUTDIR/$FN.png
  sox $INDIR/$FN.mp3 -r $SR -t wav - | sox -t wav - -n spectrogram -y $YSIZE -x $XSIZE -z 60 -Z -30 -m -o $OUTDIR/$FN.png
done
