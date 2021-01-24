#!/usr/bin/python3

# ObsPy code to get data from Earthworm waveserver on BBShark+RPi
# 2020-Dec-23 J.Beale

from obspy.clients.earthworm import Client
import obspy
from scipy.io import savemat   # for export to Matlab format file
import sys            # for writing file
import numpy as np    # for writing
# ------------------------------------------------------------------

hours = 2  # how many hours of data to export into CSV file
calibration = 1.0   # multiply data by this scale factor

client = Client("192.168.1.227", 16022) # IP and port of EW server
response = client.get_availability('*', '*', channel='EHZ')
print(response)
  
tStart = response[0][4]      # date/time of beginning of data record
tEnd = response[0][5]        # date/time of end of data record
StationName = response[0][1] # eg. 'SHARK'

dur = 60*60*hours  # 60 seconds * 60 minutes * hours

winEnd = tEnd  # fixed end time
winStart = tEnd - dur

print("Requesting " + str(winStart) + " - " + str(winEnd))

startStr = str(winStart)
fname = startStr[0:13]+startStr[14:16]+"_"+StationName+".csv"

# Specify: network, station, location, channel, startTime, endTime
st = client.get_waveforms('PA', 'SHARK', '00', 'EHZ', winStart, winEnd)
print(st)

print("Writing %s\n" % fname)    # DEBUG verbose show filename
for i, tr in enumerate(st):
    f = open("%s" % fname, "w")
    f.write("%s\n" % StationName)
    f.write("# STATION %s\n" % (tr.stats.station))
    f.write("# CHANNEL %s\n" % (tr.stats.channel))
    f.write("# START_TIME %s\n" % (str(winStart)))
    f.write("# SAMP_FREQ %f\n" % (tr.stats.sampling_rate))
    f.write("# NDAT %d\n" % (tr.stats.npts))
    np.savetxt(f, tr.data * calibration, fmt="%6.0f")
    f.close()  
