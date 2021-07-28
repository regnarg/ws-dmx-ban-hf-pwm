#!/usr/bin/python3

import sys
import tty, termios
import time
import serial
from threading import Thread
import random


def RL(a,k):
    return (((a) << (k)) | ((a) >> (8-(k)))) & 0xFF
def compute_csum(msg):
    csum = 0
    for b in msg:
        csum = RL(csum, 3)
        csum ^= b
    return csum
def wrap_msg(msg):
    return bytes([253]) + bytes(msg) + bytes([compute_csum(msg)])


class Bus:
    def __init__(self, port):
        self.serial = serial.Serial(port, 9600)
        self.settings = { x: [0, 0, 0, 0] for x in range(10) }
        self.add_byte = {}
        self.last = {}

    def set(self, addr, channel, value, *, relative=False):
        if relative:
            new = self.settings[addr][channel] + value
        else:
            new = value
        if new > 64: new = 64
        if new < 0: new = 0
        self.settings[addr][channel] = new

    def set_all(self, addr, values):
        if isinstance(values, int): values = [values] * 4
        self.settings[addr] = list(values)

    def send_thread(self, *a):
        while True:
            for addr, chans in list(self.settings.items()):
                chans = tuple(chans)
                if addr in self.last and addr in self.add_byte and self.last[addr] == chans:
                    add_b = self.add_byte[addr]
                else:
                    add_b = self.add_byte[addr] = random.randrange(256)
                self.send_msg((addr,)+chans+(add_b,))
                self.last[addr] = tuple(chans)
            self.serial.flush()
            #time.sleep(0.005)


    def send_bytes(self, bs):
        #print("send", list(bs))
        #self.serial.write(bytes(bs))
        for b in bs:
            self.serial.write(bytes([b]))
            self.serial.flush()
            time.sleep(0.01)


    def send_msg(self, msg):
        #if msg[0] ==1:
        #    print(list(msg), '\r\n')
        self.send_bytes(wrap_msg(msg))

    def start(self):
        self.thread = Thread(target=self.send_thread)
        self.thread.daemon = True
        self.thread.start()


