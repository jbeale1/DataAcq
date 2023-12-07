# read csv file of inter-pulse timings, find pulses per minute
# 7-Dec-2023 JPB

import numpy as np

inFilename = r"C:\data\sample.csv"

with open(inFilename) as file:
    lines = [line.rstrip().lstrip() for line in file]

print(lines[1])  # line showing data acquisition start time

print("%d lines in %s" % (len(lines), inFilename))

cpmList = []
totalTime = 0 
totalPulses = 0
secA = 0    # seconds accumulator for time interval eg. 60 sec
pulseA = 0  # pulse accumulator for time interval
intervalDur = 60  # length of time interval, in seconds
minutes = 0    # how many minutes so far
sumCPM = 0     # sum of CPM over all minutes

for i in range(3,len(lines)):
    elems = lines[i].split(",")
    totalPulses += 1
    deltaT = float(elems[1])
    totalTime += deltaT    
    secA += deltaT
    if (secA > intervalDur):
        cpm = pulseA
        minutes += 1
        cpmList.append(cpm)
        sumCPM += cpm
        pulseA = 0  # reset for next interval
        secA -= intervalDur
    pulseA += 1        

pctUncertain = 100.0/np.sqrt(totalPulses)
totalCPS = (totalPulses / totalTime)
totalCPM = totalCPS * 60.0
avgCPM = sumCPM / minutes
meanCPM = np.mean(cpmList)
print("Duration: %d minutes" % len(cpmList))
print("CPM: %5.3f +/- %5.2f%%" % (totalCPM,pctUncertain))    
print("std of cpmList: %5.3f" % np.std(cpmList))
print("sqrt of mean cpm: %5.3f" % np.sqrt(meanCPM))
