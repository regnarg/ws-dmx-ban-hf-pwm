#!/usr/bin/python

import sys
import time
import serial

ser = serial.Serial(sys.argv[1], 25000)
last = 0

while True:
    s = ser.read(1)
    if time.time() - last > 0.5:
        print()
    last = time.time()
    val = s[0]
    print('xxxxx' if val == 222 else val)
