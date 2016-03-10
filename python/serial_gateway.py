#!/usr/bin/env python

import time
import sys
import json
import logging
import argparse
import serial
from messenger import Messenger

logger = logging.getLogger('serial_proxy')

APPKEY = '5697113d4407a3cd028abead'
TOPIC = 'smart_office'

class SerialProxy(Messenger):

    def __init__(self, appkey, alias, customid, topic, serial):
        self.__logger = logging.getLogger('serial_proxy.SerialProxy')
        self.__logger.debug('init')
        Messenger.__init__(self, appkey, alias, customid)
        self.serial = serial
        self.topic = topic

    def __del__(self):
        self.__logger.debug('del')

    def on_message(self, args):
        self.__logger.debug('on_message: %s', args)

        self.serial.write(args)

    def loop(self):
        Messenger.loop()
        data = self.serial.read()
        if len(data) > 0:
            Messenger.publish(data, self.topic, 1);

if __name__ == '__main__':

    logging.basicConfig(level=logging.DEBUG)

    parser = argparse.ArgumentParser(description='Serial proxy')
    parser.add_argument('alias', type=str, nargs='+', help='alias')
    args = parser.parse_args()

    
    proxy = []
    for a in args.alias:
        p = SerialProxy(APPKEY, a, a, TOPIC, serial)
        proxy.append(p)

    while True:
        for p in proxy:
           p.loop()
