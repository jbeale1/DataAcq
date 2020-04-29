#!/usr/bin/python

# read two CSV files with seismic event summaries to match coincident events
# J.Beale 28-April-2020

import csv
import sys			# command-line params
import numpy as np
import matplotlib.pyplot as plt

data_path = '/home/john/RShake/'
fbase1='RF7DC.2020.'
fbase2='R79D5.2020.'

tdThresh = 0.10  # in minutes

pEvent = False
fnum = '115'                 # default dataset number
if __name__ == "__main__":
  if (len(sys.argv) > 1):
    fnum=sys.argv[1]       # day number in dataset filename
  if (len(sys.argv) > 2):
    pEvent = True          # print data for each confirmed event

fname1 = fbase1 + fnum + '.csv'
fname2 = fbase2 + fnum + '.csv'

ffname1=data_path + fname1
ffname2=data_path + fname2
# ==========================================

with open(ffname1, 'r') as f:
    reader = csv.reader(filter(lambda row: row[0]!='#', f), delimiter=',')
    header1 = next(reader, None)  # read in the header line
    dat1 = list(reader)

with open(ffname2, 'r') as f:
    reader = csv.reader(filter(lambda row: row[0]!='#', f), delimiter=',')
    header2 = next(reader, None)  # read in the header line
    dat2 = list(reader)

row1 = 1
row2 = 1
row1Max = len(dat1)
row2Max = len(dat2)
# print("# Lines: %d , %d" % (row1Max, row2Max))

tCount = 0            # total number of matching (confirmed) events found
while True:

  flist1 = [ float(i) for i in dat1[row1] ]  # convert list of strings to list of floats
  flist2 = [ float(i) for i in dat2[row2] ]
  # print(flist1[1], flist2[1])

  E1 = flist1[1]                    # minute-of-day for event #1
  E2 = flist2[1]                    # minute-of-day for event #2

  tdThresh = flist1[3]/(2*60)            # other peak within 1/2 width of this one
  tDiff = (E1 - E2)   # difference in minutes between event times
  if (abs(tDiff) < tdThresh):
    tCount += 1                     # total number of confirmed events
    avgTime = (flist1[1] + flist2[1])/2.0
    # print("%d: %5.2f (%5.2f): %5.2f, %5.2f" % (tCount,tDiff,tdThresh,flist1[1], flist2[1]) )
    if (pEvent):
      print("%d: T=%5.2f : Width= %4.1f,%4.1f Mag= %4.2f,%4.2f (%5.2f)" % 
          (tCount,avgTime,flist1[3],flist2[3],flist1[2],flist2[2],flist2[2]-flist1[2] ) )
  
  if (E1 < E2):  # get next sensor1 record if current sensor1 event is earlier in time
    row1 += 1
  else:
    row2 += 1

  if (row1 >= row1Max):
    break
  if (row2 >= row2Max):
    break

mCount = min(row1Max, row2Max)
pct = (tCount / mCount) * 100.0
# print("# Files: %s , %s" % (fname1 , fname2) )
print("# %s Total: %d matching is %5.1f %% of orig events" % (fnum,tCount, pct) )

