#!/usr/bin/python3

# LoRa remote system range test JPB 05-SEP-2021
# based on Adafruit LoRa Pi bonnet and Feather M0 w/RFM95C

import time
import board
import busio
from digitalio import DigitalInOut, Direction, Pull
import adafruit_ssd1306 # Import the SSD1306 module.
import adafruit_rfm9x   # for RFM95c (LoRa TxRx)

fname="/dev/shm/gpsdat" # file with current time & lat/lon position
logFileName="/home/pi/GPS-log2.csv"  # combined GPS/LoRa data output logfile

flog = open(logFileName,"a")

# set the time interval (seconds) for sending packets
transmit_interval = 10

# Define radio parameters.
RADIO_FREQ_MHZ = 915.0  # Tx/Rx Freq in Mhz.

# Define pins connected to the chip.
CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)

# Initialize SPI bus.
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
# Initialze RFM radio
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, RADIO_FREQ_MHZ)

# set node addresses
rfm9x.node = 1
rfm9x.destination = 2
# initialize counter
counter = 0

# -----------------------------------------------------------
# Create the I2C interface for OLED display
i2c = busio.I2C(board.SCL, board.SDA)

# 128x32 OLED Display
reset_pin = DigitalInOut(board.D4)
display = adafruit_ssd1306.SSD1306_I2C(128, 32, i2c, reset=reset_pin)
display.fill(0)  # clear display
display.show()
width = display.width
height = display.height

print("Waiting for packets...")
now = time.monotonic()
linectr = 0   # count how many output lines
while True:
    packet = rfm9x.receive(with_header=True)
    if packet is not None:
        gdat=",,"
        try:
          with open(fname) as f:  # get GPS time,lat,lon
            gdat = f.readline().strip()
        finally:
          f.close()

        packet_text = packet[4:].decode('utf8', errors='ignore')
        RSSI_text = "{0}".format(rfm9x.last_rssi)
        ostring = ("%s,%s,%s" % (gdat,RSSI_text,packet_text)).replace('\00', '')
        flog.write(ostring+'\x0a')
        linectr += 1
        if (linectr > 20):
          flog.flush()
          linectr = 0

        display.fill(0) 	    # draw a box to clear the OLED image
        display.text('RX:', 0, 0, 1)
        display.text(packet_text, 20, 0, 1)
        display.text(RSSI_text, 25, 10, 1)
        display.text(gdat, 25,22,1)  # GPS time & location
                        
    display.show()  # refresh onboard display
    
