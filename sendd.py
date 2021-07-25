#!/usr/bin/python3

import sys
import tty, termios
import time
import serial
from threading import Thread
import random

ser = serial.Serial(sys.argv[1], 9600)

SETTINGS = {}
ADD_BYTE = {}
LAST = {}


def send_thread(*a):
    while True:
        for addr, chans in list(SETTINGS.items()):
            chans = tuple(chans)
            if addr in LAST and addr in ADD_BYTE and LAST[addr] == chans:
                add_b = ADD_BYTE[addr]
            else:
                add_b = ADD_BYTE[addr] = random.randrange(256)
            send_msg((addr,)+chans+(add_b,))
            LAST[addr] = tuple(chans)
        ser.flush()
        #time.sleep(0.005)

def RL(a,k):
    return (((a) << (k)) | ((a) >> (8-(k)))) & 0xFF

def send_bytes(bs):
    #print("send", list(bs))
    #ser.write(bytes(bs))
    for b in bs:
        ser.write(bytes([b]))
        ser.flush()
        time.sleep(0.01)

def compute_csum(msg):
    csum = 0
    for b in msg:
        csum = RL(csum, 3)
        csum ^= b
    return csum

def wrap_msg(msg):
    return bytes([253]) + bytes(msg) + bytes([compute_csum(msg)])

def send_msg(msg):
    #if msg[0] ==1:
    #    print(list(msg), '\r\n')
    send_bytes(wrap_msg(msg))


def getch():
    import sys, tty, termios
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch


SETTINGS = { x: (0, 0, 0, 0) for x in range(10) }

#def interact():
#    while True:
#        line = sys.stdin.readline()
#        vals = [0]*4
#        addr, *args = line.strip().split()
#        addr = int(addr)
#        for arg in args:
#            if ':' in arg:
#                chans, val = arg.split(':')
#                val = int(val)
#                for chan in map(int, chans.split(',')):
#                    vals[chan] = val
#            else:
#                vals = (int(arg),)*4
#        SETTINGS[addr] = tuple(vals)


def blink():
    while True:
        SETTINGS[1] = (0, 0, 0, 0)
        time.sleep(1)
        SETTINGS[1] = (16, 16, 16, 16)
        time.sleep(1)

def keys():
    vals = [0] * 4
    def inc(ch):
        vals[ch] += 1
        if vals[ch] > 64: vals[ch] = 64
        print(vals)
        SETTINGS[1] = tuple(vals)
    def dec(ch):
        vals[ch] -= 1
        if vals[ch] < 0: vals[ch] = 0
        print(vals)
        SETTINGS[1] = tuple(vals)
    while True:
        c = getch()
        if c == 'q':
            inc(0)
            inc(2)
        if c == 'w':
            inc(1)
            inc(3)
        elif c == 'a':
            dec(0)
            dec(2)
        elif c == 's':
            dec(1)
            dec(3)
        elif c in ('\r', '\n'): break

if __name__ == '__main__':
    t = Thread(target=send_thread)
    t.daemon = True
    t.start()
    #interact()
    if '--blink' in sys.argv:
        blink()
    else:
        keys()

