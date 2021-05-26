#!/usr/bin/python

import sys
import time
import serial

ser = serial.Serial(sys.argv[1], 9600)

def send_byte(b):
    print("send",b)
    assert 0 <= b <= 256
    for i in range(6):
        ser.write(bytes([b])) # repeat for redundancy
        time.sleep(0.003)
    #time.sleep(0.005)

def send_addr(a):
    assert 0 <= a < 128 or a==251
    send_byte(a)

def send_chan(c):
    assert 0 <= c < 32
    send_byte(c + 128)

def send_val(v):
    assert 0 <= v <= 64 # 0 to 64 inclusive!
    send_byte(v + 128 + 32)

if __name__ == '__main__':
    send_addr(int(sys.argv[2]))
    for arg in sys.argv[3:]:
        if ':' in arg:
            chans, val = arg.split(':')
            val = int(val)
            mask = sum( 1 << int(chan) for chan in chans.split(',') )
        else:
            mask = 31
            val = int(arg)
        send_chan(mask)
        send_val(val)
    send_byte(252)
