#!/home/john/miniconda3/envs/obspy/bin/python

# uses obspy environment (miniConda)

import numpy as np
from scipy.signal import butter, lfilter, filtfilt
from scipy.ndimage import uniform_filter1d
import matplotlib.pyplot as plt

from obspy import read
import obspy.signal

#hpfreq=10.0  # remove frequencies below this (Hz)

ndir="/home/john/RShake"

#file="AM.RF7DC.00.SHZ.D.2020.112"
filename="AM.R79D5.00.EHZ.D.2020.112"  # newer, PiZero @ 100 Hz
#filename="AM.R79D5.00.EHZ.D.2020.113"  # newer, PiZero @ 100 Hz
#filename="AM.RF7DC.00.SHZ.D.2020.113" # older, Pi3 @ 50 Hz

cutoff = 0.08                # LP filter cutoff, in Hz units


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
    y = uniform_filter1d(data, size=M, mode='reflect')
    return y



# ---------------------------------------------------

st = read(fname)  # load example seismogram

data = st[0].data
npts = st[0].stats.npts
samprate = st[0].stats.sampling_rate
dt = st[0].stats.starttime  # date/time of initial sample

# Filter specs
order = 3
fs = st[0].stats.sampling_rate
BoxSize = int(fs/cutoff)     # boxcar filter size

b, a = butter_lowpass(cutoff, fs, order)  # generate filter coefficients

st.filter('bandpass', freqmin=8, freqmax=20, corners=2, zerophase=True)

data_env = obspy.signal.filter.envelope(st[0].data)
#envf = LP_zerophase_filter(data_env, cutoff, fs, order)  # LP filtered version
envf = boxcar_filter(data_env, BoxSize)  # LP filtered version

sublen = npts   # number of points in sub-segment to view
plotstart = 30        # minutes from beginning where plot starts
plotminutes = 360      # minutes duration to show on plot

sph = samprate*60*60  # samples per hour, from samples per second
spm = samprate*60     # samples per minute, from samples per second
t = np.arange(0, (sublen-0.5)/spm, 1/spm)

StartPlt = int(0)
plotSpan = int(plotminutes * spm)

while (StartPlt <= npts):
  EndPlt = StartPlt + plotSpan     # min * samples/min = samples
  if (EndPlt >= npts):
    EndPlt = npts
  plt.figure(num=1, figsize=(16,4))          # window size in inches
  plt.plot(t[StartPlt:EndPlt], np.log(np.square(envf[StartPlt:EndPlt])), 'k-', linewidth=0.5, label=filename)
  plt.title(st[0].stats.starttime)
  plt.ylabel('Data Envelope')
  plt.xlabel('Time [m]')
  plt.grid()
  plt.legend()
  # plt.subplots_adjust(hspace=0.3)
  plt.show()
  StartPlt += plotSpan  # move on to next section of data
