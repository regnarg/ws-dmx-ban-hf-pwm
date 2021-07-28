#!/usr/bin/python3

import sys
import tty, termios
import time
import serial
from threading import Thread
import random
from ws_ban_lib import *

bus = Bus(sys.argv[1])
bus.start()

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



def blink():
    while True:
        bus.set_all(1, 0)
        time.sleep(1)
        bus.set_all(1, 16)
        time.sleep(1)

def keys():
    vals = [0] * 4
    def inc(ch):
        bus.set(1, ch, 1, relative=True)
        print(bus.settings[1])
    def dec(ch):
        bus.set(1, ch, -1, relative=True)
        print(bus.settings[1])
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
    #interact()
    if '--blink' in sys.argv:
        blink()
    else:
        keys()

