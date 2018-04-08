#!/usr/bin/python

# Print out data stream from 50 Hz single-channel Raspberry Shake
# J.Beale 07-April-2018
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
# standard deviation is square-root of variance
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
  return (var,mean)

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
  "Print out RMS energy in 3 frequency bands"
  global LPbuf    # circular buffer of 1-second averages
  global LPidx
  global LPelems
  n = len(elems)
  if (n != 100):  # all input should always have 100 elements
    print("# ERROR: n= %d" % n)

  (LPsq,mean) = variance(avg10(elems))  # low-pass 0.5-2.5 Hz
  (APsq,mean2) =  variance(elems)       # all-pass 0.5-25 Hz
  HP = math.sqrt(abs(APsq - LPsq))   # high-pass 2.5-25 Hz
  LP = math.sqrt(LPsq)
  LPbuf[LPidx] = mean
  LPidx += 1           # crank handle on circular buffer
  if LPidx == LPelems:
    LPidx = 0  
  (LLPvar, mean3) = variance(LPbuf)
  LLP = math.sqrt( LLPvar ) # low-low-pass: 0.05 - 0.5 Hz
  print("%.1f, %.1f, %.2f" % (HP,LP,LLP ) )

# --------------------------------------------------------------------
def force50( L ):        # crop or extend L so it contains exactly 50 elements
    Lmiss = 50 - len(L)
    if (Lmiss > 0):      # if less than 50 elements, extend last element through 50
      for i in range(0, Lmiss):
        L.append(L[-1])    # add on copy of last element
    if (Lmiss < 0):      # if more than 50 elements, crop off any beyond 50
      L = L[0:50]        # crop off excess elements 
    return L

# --------------------------------------------------------------------
def get50( sock ):                      # get exactly 50 elements of data from R-Shake
    data, addr = sock.recvfrom(1024)    # wait to receive data from R-Shake
    LL = data.translate(None, '{}').split(",") # remove brackets, separate elements
    L = force50( LL[2:] )               # force list to have exactly 50 elements
    return L

# ----------------------------------------------------------------------------

LPelems = 20                  # how many 1-second averages, sets lower freq limit
LPidx = 0                     # index into circular buffer

data, addr = sock.recvfrom(1024)    # wait to receive data
AL = data.translate(None, '{}').split(",") # remove brackets, separate elemen                                                                               ts
channel = AL[0]                  # first elem is channel, eg. 'SHZ'
tstamp = time.strftime("%a, %d %b %Y %H:%M:%S UTC", time.gmtime(float(AL[1])) )
print("HP50, LP2p5, LLP05")
print("# HP50=2.5-25 Hz, LP=0.5-2.5 Hz, LLP05=0.05-0.5 Hz " + channel)
print("# Epoch: " + AL[1] + " " + tstamp)
tstamp2 = time.strftime("%a, %d %b %Y %H:%M:%S ", time.localtime(float(AL[1])) )
tzone = time.tzname[time.localtime().tm_isdst]
print("# Local: " + tstamp2 + tzone)
A = force50(AL[2:])          # make sure we have exactly 50 data samples
(waste,mean) =  variance(A)  # find average of 1st 50 samples
LPbuf = [mean] * LPelems     # initialize circular buffer of averages

while 1:                            # loop forever, ping-pong buffer (A, B)
    B = get50(sock)      # get next 50 data elements (blocks until received)
    ps(A + B)  # print sdt.dev of 2 seconds of data (previous 1 and current)
    A = get50(sock)
    ps(B + A)

# ===============================================================
