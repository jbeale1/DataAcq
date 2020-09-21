#!/usr/bin/python3

# Play audio file and record mic input at the same time, for sonar application
# v0.6 21-SEP-2020 J.Beale

# sudo apt install python3-pip
# python3 -m pip install sounddevice
# pip3 install soundfile
# pip3 install numpy
# sudo apt install libatlas-base-dev

#   set full volume out:
# amixer -c 1 cset name='Speaker Playback Volume' 100%

import argparse
import sounddevice as sd
import soundfile as sf
import numpy as np                 # amax
import time                        # time.time()

outdir="/dev/shm"  # put recorded files here

#data, samplerate = sf.read('existing_file.wav')
#sf.write('new_file.flac', data, samplerate)

def int_or_str(text):
    """Helper function for argument parsing."""
    try:
        return int(text)
    except ValueError:
        return text


parser = argparse.ArgumentParser(add_help=False)
parser.add_argument(
    '-l', '--list-devices', action='store_true',
    help='show list of audio devices and exit')
args, remaining = parser.parse_known_args()
if args.list_devices:
    print(sd.query_devices())
    parser.exit(0)
parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter,
    parents=[parser])
parser.add_argument(
    'filename', metavar='FILENAME',
    help='audio file to be played back')
parser.add_argument(
    '-d', '--device', type=int_or_str,
    help='output device (numeric ID or substring)')
args = parser.parse_args(remaining)

recChan = 2  # how many channels we are recording

sec = int(time.time())  # current Unix timestamp

ofname = 'R_' + str(sec) + '.wav'

try:
    data, fs = sf.read(args.filename, dtype='float32')
    #print("Play fs: %d\n",fs)

    # sd.play(data, fs, device=args.device)  # play 'data' array at fs sample rate
    recdata = sd.playrec(data, fs, channels=recChan, device=args.device) # play data array AND record recChan channels
    # recdata is a [n][recChan] array of float32 type (?)
    status = sd.wait()  # wait here until playback is done

    #pk1 = np.amax(recdata[:,0])  # peak recorded value (positive)
    #pk2 = np.amax(recdata[:,1]) 

    opath= outdir + "/" + ofname
    sf.write(opath, recdata, fs)  # save recorded data to file
    tstamp = ofname[2:-4]
    # print("%s %d %4.2f %4.2f " % (tstamp,int(fs/1000),pk1,pk2),end="")
    print("%s" % opath);

except KeyboardInterrupt:
    parser.exit('\nInterrupted by user')
except Exception as e:
    parser.exit(type(e).__name__ + ': ' + str(e))
if status:
    parser.exit('Error during playback: ' + str(status))
