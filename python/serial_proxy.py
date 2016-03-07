#!/usr/bin/env python

import time
import sys
import json
import logging
import argparse
from messenger import Messenger

logger = logging.getLogger('serial_proxy')

APPKEY = '5697113d4407a3cd028abead'
#ALIAS = 'power_plug'

class SerialProxy(Messenger):

    def __init__(self, appkey, alias, customid):
        self.__logger = logging.getLogger('serial_proxy.SerialProxy')
        self.__logger.debug('init')
        Messenger.__init__(self, appkey, alias, customid)

    def __del__(self):
        self.__logger.debug('del')

    def on_message(self, args):
        self.__logger.debug('on_message: %s', args)
        try:
            msg = json.loads(args['msg'])
        except Exception as e:
            self.__logger.warning(e)
            return

if __name__ == '__main__':

    logging.basicConfig(level=logging.DEBUG)

    parser = argparse.ArgumentParser(description='Serial proxy')
    parser.add_argument('alias', type=str, nargs='+', help='alias')
    args = parser.parse_args()

    proxy = []
    for a in args.alias:
        p = SerialProxy(APPKEY, a, a)
        proxy.append(p)

    while True:
        for p in proxy:
           p.loop()
