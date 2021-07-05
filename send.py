#!/usr/bin/python3

import sys
import time
import serial
import random

ser = serial.Serial(sys.argv[1], 9600)

def RL(a,k):
    return (((a) << (k)) | ((a) >> (8-(k)))) & 0xFF

def send_bytes(bs):
    print("send", list(bs))
    #for i in range(50):
    #    ser.write(bytes(bs))
    #    time.sleep(0.001)
    for b in bs:
        ser.write(bytes([b]))
        ser.flush()
        time.sleep(0.05)
    ser.flush()

def compute_csum(msg):
    csum = 0
    for b in msg:
        csum = RL(csum, 3)
        csum ^= b
    return csum

def wrap_msg(msg):
    msg = bytes(list(msg) + [random.randrange(256)])
    assert len(msg) == 6
    return bytes([253]) + bytes(msg) + bytes([compute_csum(msg)])

def send_msg(msg):
    send_bytes(wrap_msg(msg))

if __name__ == '__main__':
    addr = int(sys.argv[2])
    vals = [0, 0, 0, 0]
    for arg in sys.argv[3:]:
        if ':' in arg:
            chans, val = arg.split(':')
            val = int(val)
            for chan in map(int, chans.split(',')):
                vals[chan] = val
        else:
            vals = [int(arg)]*4
    send_msg([addr] + vals)
