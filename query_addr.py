#!/usr/bin/python

import sys
import time
import serial

ser = serial.Serial('/dev/ttyUSB0', 25000)

ser.write(bytes([250]))
time.sleep(0.1)
ser.write(bytes([250]))
time.sleep(0.1)
addr = ser.read(1)

print(addr[0])
