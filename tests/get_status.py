#!/usr/bin/env python

import time
import sys
import logging
import argparse
from socketIO_client import SocketIO
from messenger import Messenger

logger = logging.getLogger('get_status')
logging.basicConfig(level=logging.DEBUG)

APPKEY = '5697113d4407a3cd028abead'
#TOPIC = 'yunba_smart_plug'
#self.alias = 'plc_0'

class Status(Messenger):

    def __init__(self, topic, alias, cmd):
        self.__logger = logging.getLogger('get_status.Status')
        self.__logger.info('init')
        Messenger.__init__(self, APPKEY, 'status', 'status')
        self.topic = topic
        self.alias = alias
        self.cmd = cmd 

    def __del__(self):
        self.__logger.info('del')

    def on_connack(self, args):
        self.__logger.debug('on_connack: %s', args)
        self.socketIO.emit('subscribe', {'topic': self.topic})

    def on_set_alias(self, args):
        self.__logger.debug('on_set_alias: %s', args)
        self.publish_to_alias(self.alias, '{"cmd": '+ self.cmd + ', "devid": "' + self.alias + '"}')

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Status')
    parser.add_argument('topic', type=str, help='topic')
    parser.add_argument('alias', type=str, help='alias')
    parser.add_argument('cmd', type=str, help='cmd')
    args = parser.parse_args()

    s = Status(args.topic, args.alias, args.cmd)

    while True:
        s.loop()
        time.sleep(0.1)

