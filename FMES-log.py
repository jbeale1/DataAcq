#!/usr/bin/python3

# tested with Python 3.6.5 (default, Apr  1 2018, 05:46:30) 
import serial, datetime, time
import termios  # strange USB serial vodoo

path = "/home/pi/FMES/"    # path to log file

# device='/dev/ttyACM0'  # Teensy 3.2
device='/dev/ttyUSB0'      # USB-serial device
#baudrate=57600             # serial device baud rate
baudrate=115200             # serial device baud rate

FLIM = 100  # flush buffer after writing this many lines

# time.sleep(45)  # when run at bootup, delay to make sure network time has been set

t = datetime.datetime.utcnow()
ts = t.strftime("%y%m%d_%H%M%S")         # for example: 190516_183009
fname = path + ts + "_FMES.csv"          # data log filename with start date/time
f = open(fname, 'w')                        # open log file

fctr = FLIM - 5  # saved-line counter (flush buffer after FLIM lines, except at start)
pctr = 5  # sample decimation counter 
firstline = 1  # have not yet finished the first line

f.write("ADC1, Range1, ADC2, Range2\n") # write CSV file column header
oline = "# Start: " + str(datetime.datetime.utcnow()) + " UTC \n"
f.write(oline)
f.write("# ADS1115 Diff 01+23 channels 2019-04-07 JPB\n")

oldm = int(time.time()/60.0)  # time.time() = floating-point seconds since epoch

with serial.Serial(device, baudrate, timeout=1) as ser:
  oline = "# " +  str(ser) + "\n" # DEBUG: show state of serial port
  f.write(oline) # debug
  f.flush()
  for i in range(2):
    line = ser.readline()
    print(line) # DEBUG: show raw input line of text

  while True:
    line = ser.readline()
    # print(line) # DEBUG: show raw input line of text
    s0 = line.decode("utf-8").strip()  # bytes to string, without start/end whitespace
    s = s0.strip('\0')  # serial port open sometimes gives a null byte
    nc = len(s)  # how many useful characters in the input string?
    s = s
    if ( nc > 0 ):
      m = int(time.time()/60.0)  # time.time() = floating-point seconds since epoch
      f.write(s)
      if (m != oldm):  # minute marker at top of each minute
        oline = ", " + str(datetime.datetime.utcnow())
        f.write(oline)
        oldm=m

      f.write("\n")
      fctr += 1
      if (fctr > FLIM):
        f.flush()
        fctr = 0
      # print(s,end='')
