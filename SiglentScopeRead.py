#!/usr/bin/python3

# pyVISA to read measurement from SIGLENT SDS scope - J.Beale 18-Feb-2019
# https://www.siglentamerica.com/wp-content/uploads/dlm_uploads/2017/10/ProgrammingGuide_forSDS-1-1.pdf
# https://www.siglentamerica.com/operating-tip/remotely-retrieve-measurements-sds-x-xe-series-oscilloscope/

import visa  # pyVISA library for instrument control
import time,datetime  # time delay, and time/date stamp

outbuf = ''
fctr = 0         # counter of lines written to log file

# ======================================================================================

f = open('Scope-log.csv', 'w')

resources = visa.ResourceManager('@py')
oscope = resources.open_resource('TCPIP::192.168.1.153::INSTR')

f.write("date time, dT\n")  # column headers
f.write("# SDS Scope log v0.1 18 Feb 2019 JPB \n")
f.write("# " + oscope.query('*IDN?')) #  Siglent Technologies,SDS1202X-E,SDS1ECDX2R3696,7.1.3.17R1
f.write("# " + oscope.query('ALL_STATUS?')) # ALST STB,0,ESR,0,INR,8193,DDR,0,CMR,EXR,0,URR,0

#  â€œMEAD FRR,C1-C2â€ add measurement rising edge to rising edge, Chan.1 - Chan.2
buf = oscope.query('MEAD FRR,C1-C2') 
buf = oscope.query('MEAD FRR,C2-C1') 

print("# " + oscope.query('*IDN?'), end='') 
print("# " + oscope.query('ALL_STATUS?'), end='') 

while True:
  buf = oscope.query('C1-C2:MEAD? FRR')  #  C1-C2:MEAD FRR,1.16E-07S
  len1 = len(buf)
  print("C1-C2 FRR result: " + buf)
  # print(len1)
  if (len1 > 0):
    try:
      buffer1 = buf.split(',')[1]  # separate command parts
    except:
      print("Error: " + buf)
  buf = oscope.query('C2-C1:MEAD? FRR')  #  C2-C1:MEAD FRR,1.23E-07S
  len2 = len(buf)
  print("C2-C1 FRR result: " + buf)
  # print(len2)
  if (len2 > 0):
    buffer2 = buf.split(',')[1]  # separate command parts

  outbuf = str(datetime.datetime.now())
  f.write(outbuf)
  if (len1 > 0):
    outbuf = ', ' + buffer1.split()[0][:-1] # remove \r\n at end, then remove 'S'
    f.write(outbuf)
  if (len2 > 0):
    outbuf = ', ' + buffer2.split()[0][:-1] # remove \r\n at end, then remove 'S'
    f.write(outbuf)
  f.write("\n")
  time.sleep(2)
  fctr += 1
  if (fctr > 10):
      f.flush()
      fctr = 0

oscope.close()
