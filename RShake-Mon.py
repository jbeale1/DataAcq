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

def variance( dat ):
  "Calculate variance of elements"
  n = int(0)
  mean= float(0)
  m2 = float(0)
  for elem in dat:
    x = float(elem)
    n += 1
    delta = x - mean
    mean += delta/n
    m2 += (delta * (x - mean))
  var = ( m2/(n-1) )
  return var

def pspair( elems ):
  "Print out std.dev of 1st & 2nd half of list"
  n=len(elems)/2
  print("%.1f" % stdev(elems[:n]))
  print("%.1f" % stdev(elems[n:]))

def avg10( elems ):
  "Return new list 1/10 size, each elem avg of 10 elements input"
  i = int(0)
  j = int(0)
  sum = float(0.0)
  ret = [float(0)] * 10
  for e in elems:
    i += 1
    sum += float(e)
    if (i == 10):
      ret[j] = sum/10
      i = 0
      j += 1
      sum = 0.0
  return ret

def ps( elems ):
  "Print out RMS energy in HighPass and LowPass bands"
  LPsq = variance(avg10(elems))  # low-pass 0.5-2.5 Hz
  HP = math.sqrt(abs(variance(elems) - LPsq))   # high-pass 2.5-50 Hz
  LP = math.sqrt(LPsq)
  print("%.1f, %.1f" % (HP,LP ) )

# ----------------------------------------------------------------------------
data, addr = sock.recvfrom(1024)    # wait to receive data
e1 = data.translate(None, '{}').split(",") # remove brackets, separate elemen                                                                               ts
channel = e1[0]                  # first elem is channel, eg. 'SHZ'
tstamp = time.strftime("%a, %d %b %Y %H:%M:%S UTC", time.gmtime(float(e1[1])) )
print("HP50, LP2p5")
print("# Full=0.5-25 Hz, LP=0.5-2.5 Hz " + channel)
print("# Epoch: " + e1[1] + " " + tstamp)
tstamp2 = time.strftime("%a, %d %b %Y %H:%M:%S ", time.localtime(float(e1[1])) )
tzone = time.tzname[time.localtime().tm_isdst]
print("# Local: " + tstamp2 + tzone)

# print(sum10(e1[2:]))

while 1:                                # loop forever, ping-pong buffer (e1,e2)
    data, addr = sock.recvfrom(1024)    # wait to receive data from R-Shake
    e2 = data.translate(None, '{}').split(",") # remove brackets, separate elements
    # print( sum10(e1[2:] + e2[2:]) )
    ps(e1[2:] + e2[2:])  # print sdt.dev of 2 seconds of data (previous 1 and current)
    data, addr = sock.recvfrom(1024)    # wait to receive data
    e1 = data.translate(None, '{}').split(",") # remove brackets, separate elements
    ps(e2[2:] + e1[2:])

# ===============================================================
