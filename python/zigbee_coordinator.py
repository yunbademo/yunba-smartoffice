#!/usr/bin/env python

import time
import sys
import json
import logging
import argparse
import serial
import struct
from messenger import Messenger

logger = logging.getLogger('zigbee_coordinator')
logging.basicConfig(level=logging.DEBUG)

APPKEY = '5697113d4407a3cd028abead'
# TOPIC = 'smart_office'
HEADER_LEN = 4
FLAG_CHAR = 0xcc
MSG_TYPE_UP = 0x01
MSG_TYPE_DOWN = 0x02
MSG_TYPE_CONTROL = 0x03

class Proxy(Messenger):

    def __init__(self, appkey, alias, customid, topic, transfer, zigbee_addr):
        self.__logger = logging.getLogger('zigbee_coordinator.Proxy')
        self.__logger.debug('init')
        Messenger.__init__(self, appkey, alias, customid)
        self.transfer = transfer
        self.topic = topic
        self.zigbee_addr = zigbee_addr

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
        self.transfer.write(self.alias, MSG_TYPE_DOWN, msg)

    def publish(self, msg):
        Messenger.publish(self, msg, self.topic, 1)

    def on_set_alias(self, args):
        self.__logger.debug('on_set_alias: %s', args)
        self.transfer.write(self.alias, MSG_TYPE_CONTROL, self.alias)

class Transfer():
    def __init__(self, port, baud, topic):
        self.__logger = logging.getLogger('zigbee_coordinator.Transfer')
        self.__logger.debug('init')
        self.srl = serial.Serial(port)
        self.srl.baudrate = baud
        self.srl.timeout = 0.3
        self.srl.flushInput()
        self.srl.flushOutput()

        self.step = 1
        self.body_len = 0
        self.recv_len = -1
        self.body_data = ''
        self.proxys = {}

        self.topic = topic
        self.cur_target_addr = ''

    def __del__(self):
        self.__logger.debug('del')

    def recv_header(self):
        x = 0
        while self.srl.inWaiting() >= HEADER_LEN:
            ch = self.srl.read(1)
            x, = struct.unpack('>B', ch)
            # self.__logger.debug('recv: 0x%02x [%c]', x, ch)
            if x == FLAG_CHAR:
                self.__logger.debug('got a flag char')
                break
    
        if x != FLAG_CHAR:
            return
    
        ch = self.srl.read(1)
        x, = struct.unpack('>B', ch)
        if x == MSG_TYPE_DOWN:
            return

        self.msg_type = x
    
        data = self.srl.read(2)
        self.body_len, = struct.unpack('>H', data)
        self.__logger.debug('body length: %d', self.body_len)
    
        self.recv_len = 0
        self.step = 2
        self.body_data = ''
    
    def recv_body(self):
        if self.srl.inWaiting() <= 0:
            return
    
        data = self.srl.read(self.body_len - self.recv_len)
        self.body_data += data
        self.recv_len += len(data)
    
        if self.recv_len != self.body_len:
            return
    
        self.__logger.debug('get a msg:')
        self.__logger.debug(self.body_data)
      
        try:
            jso = json.loads(self.body_data)
        except Exception as e:
            self.step = 1
            return

        alias = jso['devid']

        if self.msg_type == MSG_TYPE_CONTROL:
            if self.proxys.has_key(alias):
                p = self.proxys[alias]
                p.zigbee_addr = jso['addr']
                self.write(alias, MSG_TYPE_CONTROL, alias)
            else:
                p = Proxy(APPKEY, alias, alias, self.topic, self, jso['addr'])
                self.add_proxy(alias, p)
        elif self.msg_type == MSG_TYPE_UP:
            if self.proxys.has_key(alias):
                self.proxys[alias].publish(self.body_data)
          
        self.step = 1

    def set_target_addr(self, alias):
        addr = self.proxys[alias].zigbee_addr
        if self.cur_target_addr == addr:
            return
            # pass

        self.cur_target_addr = addr

        int_addr = int(addr, 16)

        self.__logger.debug('set target addr: 0x%02x%02x', (int_addr >> 8) & 0xff, int_addr & 0xff)

        # 5a aa 07 02 xx xx
        data = struct.pack('>BBBBBB', 0x5a, 0xaa, 0x07, 0x02, (int_addr >> 8) & 0xff, int_addr & 0xff)

        self.srl.write(data)
        time.sleep(0.010) # wait for setting finished

    def write(self, alias, msg_type, msg):
        self.set_target_addr(alias)

        msg = msg.encode('utf-8')

        data = struct.pack('>BBH', FLAG_CHAR, msg_type, len(msg))
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
    parser.add_argument('topic', type=str, help='topic')
    parser.add_argument('--port', '-p', type=str, help='Serial port(/dev/ttyUSB0)', default='/dev/ttyUSB0')
    parser.add_argument('--baud', '-b', help='Baud rate(115200)', type=int, default=115200)
    args = parser.parse_args()

    transfer = Transfer(args.port, args.baud, args.topic)

    while True:
        transfer.loop()

if __name__ == '__main__':
    main()
