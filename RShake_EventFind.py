#!/home/john/miniconda3/envs/obspy/bin/python

# uses obspy environment (miniConda)

import numpy as np
import scipy.ndimage as nd
from scipy.signal import hilbert  # for amplitude envelope
from scipy.signal import butter, lfilter, filtfilt, decimate
import matplotlib.pyplot as plt

from obspy import read
import obspy.signal

#hpfreq=10.0  # remove frequencies below this (Hz)

ndir="/home/john/RShake"

#file="AM.RF7DC.00.SHZ.D.2020.112"
#filename="AM.R79D5.00.EHZ.D.2020.112"  # newer, PiZero @ 100 Hz
#filename="AM.R79D5.00.EHZ.D.2020.113"  # newer, PiZero @ 100 Hz
filename = "AM.R79D5.00.EHZ.D.2020.115"

#filename="AM.RF7DC.00.SHZ.D.2020.111" # older, Pi3 @ 50 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.112" # older, Pi3 @ 50 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.110" # older, Pi3 @ 50 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.109" # older, Pi3 @ 50 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.108" # older, Pi3 @ 50 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.107" # older, Pi3 @ 50 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.106" # older, Pi3 @ 50 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.115" # older, Pi3 @ 50 Hz

cutoff = 0.08                # LP filter cutoff, in Hz units
plotminutes = 5*360      # minutes duration to show on plot
#plotminutes = 30      # minutes duration to show on plot
#vThresh = 6.3   # vehicle traffic threshold is ~ 6.3 after log() operation
vThresh = 6.55   # vehicle traffic threshold is ~ 6.3 after log() operation


fname=ndir+"/"+filename
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

def LP_zerophase_filter(data, cutoff, fs, order=3):
    b, a = butter_lowpass(cutoff, fs, order=order)
    y = filtfilt(b, a, data)
    return y

def boxcar_filter(data, M):
    y = nd.uniform_filter1d(data, size=M, mode='reflect')
    return y



# ---------------------------------------------------

st = read(fname)  # load example seismogram

data = st[0].data
npts = st[0].stats.npts
samprate = st[0].stats.sampling_rate
dt = st[0].stats.starttime  # date/time of initial sample

# Filter specs
order = 3
BoxSize = int(samprate/cutoff)     # boxcar filter size
Drate = 25                        # decimation ratio

b, a = butter_lowpass(cutoff, samprate, order)  # generate filter coefficients

st.filter('bandpass', freqmin=8, freqmax=20, corners=2, zerophase=True)

data_env = abs(obspy.signal.filter.envelope(st[0].data)) # envelope could be neg. without abs()
#data_env = abs(hilbert(st[0].data))  # find envelope of narrowband signal

# some kind of artifact requires abs() here?
envD1 = abs(decimate(data_env, int(Drate/5), n=1))            # reduce size by 10x
envD2 = abs(decimate(envD1, 5, n=1))
nptsD = envD2.size                                  # number of points in decimated data



bcf = (boxcar_filter(envD2, int(BoxSize/Drate)))  # LP filtered version
# note: is this value ever 0 or negative? that would be a problem for log()

#print("length of raw data: %d" % nptsD)
#print("Raw min val: %f" % np.amin(envD2) )
#print("index of raw min: %d" % np.argmin(envD2) )
#print("Boxcar min,max val: %f , %f" % (np.amin(bcf), np.amax(bcf) ))
#print("index of min,max: %d , %d" % (np.argmin(bcf), np.argmax(bcf) ))

envf = np.log(bcf)
fsD = samprate / Drate   # sample rate of final decimated data vector


sublen = npts   # number of points in sub-segment to view
# plotstart = 30        # minutes from beginning where plot starts

sph = fsD*60*60  # samples per hour, from samples per second
spm = fsD*60     # samples per minute, from samples per second

StartPlt = int(0)
plotSpan = int(plotminutes * spm)
t = np.arange(0, (sublen-0.5)/spm, 1/spm)

eventTotal = 0

while (StartPlt <= (nptsD-1)):
  EndPlt = StartPlt + plotSpan     # min * samples/min = samples
  if (EndPlt >= nptsD):
    EndPlt = nptsD
  print("Start:%d  End:%d  Total: %d" % (StartPlt, EndPlt, nptsD))

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
  print("New: %4d  Total Events: %5d" % (nb_labels, eventTotal)) # how many vehicle events detected

#  if False :
  if (nb_labels > 0):
    print("#   minutes  Max  Width")
    for i in range(nb_labels):
       s = poslist[i][0] # [n][0]  to get first element of tuple which is slize (2nd member is null)
       #print("%d : %d" % (s.start, s.stop)) # debug: list of slices?
       tStart = t[s.start]   # units of time (m)
       tEnd = t[s.stop]
       tDur = 60*(tEnd-tStart)  # units of seconds
       print("%03d: %5.2f  %2.1f %4.1f" % (i+1, pktimes[i], max_vals[i], tDur))


  plt.plot(tWin, envfWin, 'k-', linewidth=0.5, label=filename)
  plt.title(st[0].stats.starttime)
  plt.ylabel('Data Envelope')
  plt.xlabel('Time [m]')
  plt.grid()
  plt.legend()
  # plt.subplots_adjust(hspace=0.3)
  plt.show()

  StartPlt += plotSpan  # move on to next section of data

print("\n=============")
print("%s %s Events: %d" % (filename, st[0].stats.starttime, eventTotal)) # how many vehicle events detected
