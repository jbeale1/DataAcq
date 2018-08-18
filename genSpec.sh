#!/bin/bash

# use SOX to generate spectrograms of audio files

INDIR="/home/john/doppler"       # read input sound files here
AUDTODIR="/home/john/doppler/180818"  # move sound files to here
OUTDIR="/home/john/doppler/out"  # put spectrogram files here

SR="4k"                          # resample file to this rate (Hz) before generating spectrogram
ZSTART="-30" # top value of spec (dB)
ZRANGE="60"  # displayed dynamic range (dB)
XSIZE=1000   # width of spectrogram (pixels)
YSIZE=300    # height of spectrogram (pixels)

for f in $INDIR/*.mp3 ; do
  f1=`basename "$f"`         # get just filename, without the path
  FN="${f1%.*}"              # get base filename, without .mp3 extension
  # mfile=$INDIR/$FN.mp3       # full filename of MP3 file to use
  # echo $INDIR/$FN.mp3 $OUTDIR/$FN.png
  sox $f -r $SR -t wav - | sox -t wav - -n spectrogram -y $YSIZE -x $XSIZE \
    -z $ZRANGE -Z $ZSTART -m -o $OUTDIR/$FN.png
  mv $f  $AUDTODIR/$f1  # move file after processing
done
