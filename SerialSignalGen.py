# Python3.10  Generate waveform, send out on serial port
# also works with Windows com0com virtual serial port
# 18-Sep-2022 J.Beale

import serial  # access real or virtual serial port
import time    # delay between output, for a fixed rate ouput
import math    # generate a mix of sine waves

s = serial.Serial('COM3', 115200, timeout=0.5)
points = 300  # how many data points in one wave
pi2 = 2 * 3.14159265358979323  #  2 * Pi
off = 0.0;  # phase offset #1
off2 = 0.0; # phase offset #2

while (True):
    for i in range(points):
        x = i * (pi2 / points)
        off += 0.01;
        off2 += 0.001;
        y = math.sin(x) + 0.4 * math.sin((x+off)*3) + 0.2 * math.sin((x+off2)*2)
        outs = "{:.6f}".format(y) + "\n"  # float to string
        s.write(outs.encode())  # send bytes in string out serial port
        print(outs,end='')
        time.sleep(0.01)

ser.close()
