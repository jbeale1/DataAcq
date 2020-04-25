#!/home/john/miniconda3/envs/obspy/bin/python
# uses obspy environment (miniConda) to read Raspberry Shake data files

import sys                        # command line arguments
import os                         # file basename
import datetime                   # current processing time
import numpy as np
import scipy.ndimage as nd
from scipy.signal import hilbert  # for amplitude envelope
from scipy.signal import butter, lfilter, decimate
import matplotlib.pyplot as plt

from obspy import read
import obspy.signal

ndir="/home/john/RShake"  # input directory
odir="/home/john/RShake"  # output directory

filename="AM.R79D5.00.EHZ.D.2020.110"  # newer, PiZero @ 100 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.111" # older, Pi3 @ 50 Hz

cutoff = 0.08                # LP filter cutoff, in Hz units
plotminutes = 5*360      # minutes duration to show on plot
#plotminutes = 30      # minutes duration to show on plot
vThresh = 6.45     # for RF7DC station (traffic event threshold log() operation)
#vThresh = 6.55   # for R79D5 station
tDurThresh = 5   # valid event must be longer than this (seconds)

# =========================================================

def butter_lowpass(cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    return b, a

def butter_lowpass_filter(data, cutoff, fs, order=5):
    b, a = butter_lowpass(cutoff, fs, order=order)
    y = lfilter(b, a, data)
    return y

#def LP_zerophase_filter(data, cutoff, fs, order=3):
#    b, a = butter_lowpass(cutoff, fs, order=order)
#    y = filtfilt(b, a, data)
#    return y

def boxcar_filter(data, M):
    y = nd.uniform_filter1d(data, size=M, mode='reflect')
    return y

# ---------------------------------------------------


if __name__ == "__main__":
  if (len(sys.argv) > 1):
    fname=sys.argv[1]

bname = os.path.basename(fname)  # base filename without path
fparts = bname.split('.')  # get components of filename
oname = odir+'/'+(fparts[1]+'.'+fparts[5]+'.'+fparts[6])+'.csv'

st = read(fname)  # load example seismogram
data = st[0].data
npts = st[0].stats.npts
samprate = st[0].stats.sampling_rate
dt = st[0].stats.starttime  # date/time of initial sample

print("%s" % bname,end='',flush=True)
# -----------------------------------

of = open(oname, 'w')  # open output results file

# -----------------------------------
# Filter specs
order = 3
BoxSize = int(samprate/cutoff)     # boxcar filter size
Drate = 25                        # decimation ratio

b, a = butter_lowpass(cutoff, samprate, order)  # generate filter coefficients

st.filter('bandpass', freqmin=8, freqmax=20, corners=2, zerophase=True)

data_env = abs(obspy.signal.filter.envelope(st[0].data)) # envelope could be neg. without abs()
#data_env = abs(hilbert(st[0].data))  # find envelope of narrowband signal

# note: decimate with n>0 interpolation may yeild negative values; bad for subsequent log()
envD1 = (decimate(data_env, int(Drate/5), n=0))            # reduce size by 10x
envD2 = (decimate(envD1, 5, n=0))
nptsD = envD2.size                                  # number of points in decimated data

bcf = (boxcar_filter(envD2, int(BoxSize/Drate)))  # LP filtered version

envf = np.log(bcf)
fsD = samprate / Drate   # sample rate of final decimated data vector

sublen = npts   # number of points in sub-segment to view

sph = fsD*60*60  # samples per hour, from samples per second
spm = fsD*60     # samples per minute, from samples per second

StartPlt = int(0)
plotSpan = int(plotminutes * spm)
t = np.arange(0, (sublen-0.5)/spm, 1/spm)

eventTotal = 0
durList= np.empty(5000)         # allocate array of event durations
durCount = int(0)

of.write("num,  start(m),  Max,  dur(s)\n")
of.write("# %s %s\n" %  (bname, st[0].stats.starttime))
of.write("# Processed at %s vThresh: %5.3f\n" % 
   (datetime.datetime.now().astimezone().isoformat(), vThresh) )

while (StartPlt <= (nptsD-1)):
  EndPlt = StartPlt + plotSpan     # min * samples/min = samples
  if (EndPlt >= nptsD):
    EndPlt = nptsD
  # print("Start:%d  End:%d  Total: %d" % (StartPlt, EndPlt, nptsD))

  # t = np.arange(0, ((EndPlt - StartPlt)-0.5)/spm, 1/spm)
  plt.figure(num=1, figsize=(16,4))          # display image size in inches
  envfWin = envf[StartPlt:EndPlt]            # current window region into full dataset
  tWin = t[StartPlt:EndPlt]

  mask = envfWin > vThresh                   # binary mask of above-threshold areas
  label_im, nb_labels = nd.label(mask)       # find and label connected regions
  poslist = nd.find_objects(label_im)        # find positions of labelled objects
  sizes = nd.sum(mask, label_im, range(1, nb_labels + 1))  # size of each region
  max_vals = nd.maximum(envfWin, label_im, range(1, nb_labels + 1))
  max_pos = nd.maximum_position(envfWin, label_im, range(1, nb_labels + 1))
  np.set_printoptions(precision=1)  # let's not get excessive on the decimals
  idx = [i[0] for i in max_pos]   # index of peak values
  pktimes = [tWin[x] for x in idx] # actual time of given index
  np.set_printoptions(precision=2)  # let's not get excessive on the decimals

  eventTotal += nb_labels

#  if False :
  if (nb_labels > 0):  # we have detected a traffic event (eg. car passing by)
    # print("New: %4d  Total Events: %5d" % (nb_labels, eventTotal)) # how many vehicle events detected
    for i in range(nb_labels):
       s = poslist[i][0] # [n][0]  to get first element of tuple which is slize (2nd member is null)
       #print("%d : %d" % (s.start, s.stop)) # debug: list of slices?
       tStart = t[s.start]   # units of time (m)
       tEnd = t[s.stop]
       tDur = 60*(tEnd-tStart)  # units of seconds
       if (tDur > tDurThresh):
         durList[durCount] = tDur
         durCount += 1
         #print("%03d, %5.2f,  %2.1f, %4.1f" % (i+1, pktimes[i], max_vals[i], tDur))
         of.write("%03d, %5.2f,  %2.1f, %4.1f\n" % (i+1, pktimes[i], max_vals[i], tDur)) # for this event

  if False :  # display graph of processed data
#  if True :  # display graph of processed data
    plt.plot(tWin, envfWin, 'k-', linewidth=0.5, label=filename)
    plt.title(st[0].stats.starttime)
    plt.ylabel('Data Envelope')
    plt.xlabel('Time [m]')
    plt.grid()
    plt.legend()
    plt.show()

  StartPlt += plotSpan  # move on to next section of data

# === end of file, finish up & close

durMean = np.mean(durList[0:durCount])
durStd = np.std(durList[0:durCount])

of.write("# %s %s \n" %  (bname, st[0].stats.starttime) )
of.write("# Events: %d avg:%5.3f std:%5.3f\n" % 
   (durCount, durMean, durStd)) # how many vehicle events detected
print(" %s Events: %d avg:%5.3f std:%5.3f" %  
     (st[0].stats.starttime, durCount, durMean, durStd)) # how many vehicle events detected
#print(" complete %s" % datetime.datetime.now() )
of.close()
