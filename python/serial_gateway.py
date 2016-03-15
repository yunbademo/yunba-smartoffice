#!/usr/bin/env python

import time
import sys
import json
import logging
import argparse
import serial
import struct
from messenger import Messenger

logger = logging.getLogger('serial_gateway')
logging.basicConfig(level=logging.DEBUG)

APPKEY = '5697113d4407a3cd028abead'
TOPIC = 'smart_office'
HEADER_LEN = 4
FLAG_CHAR = '0xaa'

class Proxy(Messenger):

    def __init__(self, appkey, alias, customid, topic, transfer):
        self.__logger = logging.getLogger('serial_gateway.Proxy')
        self.__logger.debug('init')
        Messenger.__init__(self, appkey, alias, customid)
        self.transfer = transfer
        self.topic = topic

    def __del__(self):
        self.__logger.debug('del')

    def on_message(self, args):
        self.__logger.debug('on_message: %s', args)

        if not isinstance(args, dict):
            return

        if args['topic'] != self.alias:
            return

        msg = args['msg']
        self.__logger.debug('transfer msg: %s', msg)
        self.transfer.write(msg)

    def publish(self, msg):
        Messenger.publish(self, msg, self.topic, 1)

class Transfer():
    def __init__(self, port, baud):
        self.__logger = logging.getLogger('serial_gateway.Transfer')
        self.__logger.debug('init')
        self.srl = serial.Serial(port)
        self.srl.baudrate = baud
        self.srl.timeout = 0.3
        self.srl.flushInput()
        self.srl.flushOutput()

        self.step = 1
        self.body_len = 0;
        self.recv_len = -1
        self.body_data = ''
        self.proxys = {}

    def __del__(self):
        self.__logger.debug('del')

    def recv_header(self):
        ch = ''
        while self.srl.inWaiting() >= HEADER_LEN:
            ch = self.srl.read(1)
            self.__logger.debug('serial read: %c', ch)
            if ch == FLAG_CHAR:
                break
    
        if ch != FLAG_CHAR:
            return
    
        ch = self.srl.read(1)
        if ch != '0x01': # not a upstream message
            return;
    
        data = self.srl.read(2)
        self.body_len = struct.unpack('>H', data)
        self.__logger.debug('body length: ' + self.body_len)
    
        self.recv_len = 0
        self.step = 2
        self.body_data = ''
    
    def recv_body(self):
        if self.srl.inWaiting() <= 0:
            return
    
        data = self.srl.read(self.body_len - self.recv_len)
        self.body_data += data
        self.recv_len += len(data);
    
        if self.recv_len != self.body_len:
            return
    
        self.__logger.debug('get a msg:')
        self.__logger.debug(self.body_data)
      
        try:
            jso = json.loads(self.body_data)
        except Exception as e:
            self.step = 1;
            return

        alias = jso['devid']
        if self.proxys.haskey(alias):
            self.proxys[alias].publish(self.body_data)
      
        self.step = 1;

    def write(self, msg):
        msg = msg.encode('utf-8')

        #l = len(msg)
        #fmt = '>BBH{0}s'.format(l)
        #self.__logger.debug('format string: ' + fmt)
        #data = struct.pack(fmt, 0xaa, 2, l, msg)
        #self.srl.write(data)

        data = struct.pack('>BBH', 0xaa, 2, len(msg))
        data += msg
        self.__logger.debug('serial write len: %d', len(data))
        self.srl.write(data)

    def add_proxy(self, alias, proxy):
        self.proxys[alias] = proxy

    def loop(self):
        if self.step == 1:
            self.recv_header()
        elif self.step == 2:
            self.recv_body()

        for p in self.proxys.values():
            p.loop()


def main():
    parser = argparse.ArgumentParser(description='Serial gateway')
    parser.add_argument('alias', type=str, nargs='+', help='alias')
    parser.add_argument('--port', '-p', type=str, help='Serial port(/dev/ttyUSB0)', default='/dev/ttyUSB0')
    parser.add_argument('--baud', '-b', help='Baud rate(115200)', type=int, default=115200)
    args = parser.parse_args()

    transfer = Transfer(args.port, args.baud)
    for a in args.alias:
        p = Proxy(APPKEY, a, a, TOPIC, transfer)
        transfer.add_proxy(a, p)

    while True:
        transfer.loop()

if __name__ == '__main__':
    main()

