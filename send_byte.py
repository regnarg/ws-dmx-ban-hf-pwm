#!/usr/bin/python

import sys
import time
import serial

ser = serial.Serial(sys.argv[1], 25000)

def send_byte(b):
    print("send",b)
    assert 0 <= b <= 256
    for i in range(5):
        ser.write(bytes([b])) # repeat for redundancy
        time.sleep(0.001)
    time.sleep(0.003)

if __name__ == '__main__':
    send_byte(int(sys.argv[2]))
