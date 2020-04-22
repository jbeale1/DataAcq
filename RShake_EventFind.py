#!/home/john/miniconda3/envs/obspy/bin/python

# uses obspy environment (miniConda)

import numpy as np
from scipy.signal import butter, lfilter, filtfilt
import matplotlib.pyplot as plt

from obspy import read
import obspy.signal

hpfreq=10.0  # remove frequencies below this (Hz)

ndir="/home/john/RShake"

file="AM.R79D5.00.EHZ.D.2020.113"  # newer, PiZero @ 100 Hz
#file="AM.RF7DC.00.SHZ.D.2020.113" # older, Pi3 @ 50 Hz

fname=ndir+"/"+file
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

# ---------------------------------------------------

st = read(fname)  # load example seismogram

data = st[0].data
npts = st[0].stats.npts
samprate = st[0].stats.sampling_rate
dt = st[0].stats.starttime  # date/time of initial sample

# Filter specs
order = 3
fs = st[0].stats.sampling_rate
cutoff = 0.06  # in Hz units

b, a = butter_lowpass(cutoff, fs, order)  # generate filter coefficients

st.filter('bandpass', freqmin=8, freqmax=12, corners=2, zerophase=True)

data_env = obspy.signal.filter.envelope(st[0].data)
envf = LP_zerophase_filter(data_env, cutoff, fs, order)  # LP filtered version

sublen = npts   # number of points in sub-segment to view

t = np.arange(0, (sublen-0.5)/samprate, 1/samprate)
# plt.plot(t, np.square(data_env)/30, 'y-', linewidth=0.5, label='envelope')
plt.plot(t, np.square(envf), 'k-', linewidth=2, label='LP filtered')
plt.title(st[0].stats.starttime)
plt.ylabel('Data Envelope')
plt.xlabel('Time [s]')
plt.grid()
plt.legend()
# plt.subplots_adjust(hspace=0.3)
plt.show()
