#!/usr/bin/python

import sys
import time
import serial

ser = serial.Serial('/dev/ttyUSB0', 9600)

def send_byte(b):
    assert 0 <= b <= 256
    for i in range(3):
        ser.write(bytes([b])) # repeat for redundancy
        time.sleep(0.05)

def send_addr(a):
    assert 0 <= a < 128
    send_byte(a)

def send_chan(c):
    assert 0 <= c < 32
    send_byte(c + 128)

def send_val(v):
    assert 0 <= v <= 64 # 0 to 64 inclusive!
    send_byte(v + 128 + 32)

if __name__ == '__main__':
    send_addr(int(sys.argv[1]))
    send_chan(int(sys.argv[2]))
    send_val(int(sys.argv[3]))
