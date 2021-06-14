#!/usr/bin/python

# download weather station data from Wunderground

import urllib2 # retrieve data from URL (Python 2.x)
import sys     # command line argument parsing
import os      # call external shell script
from datetime import datetime, timedelta  # date stuff

wdir="/home/pi/"

y = datetime.now() - timedelta(1)  # find yesterday's date
tDate = datetime.strftime(y, '%Y%m%d') # YYYYMMDD format

print("Retrieve Data for Date: %s" % (tDate))

myURL="https://api.weather.com/v2/pws/history/all?stationId=MyID"+\
  "&format=json&units=m&numericPrecision=decimal&date="+\
  tDate + "&apiKey=MyKey"

wData = urllib2.urlopen(myURL) # get data from this location

fname=wdir+"/"+tDate+"_RawData.json"
with open(fname,'w') as output: # write data to local file
  output.write(wData.read())

cmd="/home/pi/sonar/weather/doConvCSV.sh"

os.system('{} {}'.format(str(cmd), str(fname)) ) # command and argument
