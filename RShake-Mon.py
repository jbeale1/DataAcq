#!/usr/bin/python

# Print out data stream from 50 Hz single-channel Raspberry Shake
# See also: https://manual.raspberryshake.org/udp.html#udp
# https://groups.google.com/forum/#!topic/raspberryshake/vZOybDRDpHw

import socket as s
import math, time

host = ""                   # can be blank for localhost
port = 8888                             # Port to bind to
sock = s.socket(s.AF_INET, s.SOCK_DGRAM | s.SO_REUSEADDR)
sock.bind((host, port))     # connect to this socket

def plist( list ):
  "Print out elements of list, one per line"
  for elem in list:
    print elem

# from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
def stdev( dat ):
  "Calculate standard deviation of elements"
  n = int(0)
  mean= float(0)
  m2 = float(0)
  for elem in dat:
    x = float(elem)
    n += 1
    delta = x - mean
    mean += delta/n
    m2 += (delta * (x - mean))
  sdev = math.sqrt( m2/(n-1) )
  return sdev

def pspair( elems ):
  "Print out std.dev of 1st & 2nd half of list"
  n=len(elems)/2
  print("%.1f" % stdev(elems[:n]))
  print("%.1f" % stdev(elems[n:]))

def ps( elems ):
  "Print out std.dev of input list"
  print("%.1f" % stdev(elems))

data, addr = sock.recvfrom(1024)    # wait to receive data
e1 = data.translate(None, '{}').split(",") # remove brackets, separate elemen                                                                               ts
channel = e1[0]                  # first elem is channel, eg. 'SHZ'
tstamp = time.strftime("%a, %d %b %Y %H:%M:%S UTC", time.gmtime(float(e1[1])) )
print("# " + channel + " " + e1[1] + " " + tstamp)

while 1:                                # loop forever, ping-pong buffer (e1,e2)
    data, addr = sock.recvfrom(1024)    # wait to receive data from R-Shake
    e2 = data.translate(None, '{}').split(",") # remove brackets, separate elements
    ps(e1[2:] + e2[2:])  # print sdt.dev of 2 seconds of data (previous 1 and current)
    data, addr = sock.recvfrom(1024)    # wait to receive data
    e1 = data.translate(None, '{}').split(",") # remove brackets, separate elements
    ps(e2[2:] + e1[2:])

# ===============================================================
