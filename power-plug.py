#!/usr/bin/env python

import time
import sys
import json
import logging
from messenger import Messenger

ALIAS = 'power_plug'

class PowerPlug(Messenger):

    def __init__(self, alias):
        logging.debug('init')
        Messenger.__init__(self, alias)

    def __del__(self):
        logging.debug('del')

    def on_message(self, args):
        logging.debug('on_message: %s', args)
        try:
            msg = json.loads(args['msg'])
        except Exception as e:
            logging.warning(e)
            return

        if msg['m'] == 'on':
            logging.debug('on')
        elif msg['m'] == 'off':
            logging.debug('off')

if __name__ == '__main__':

    #logging.getLogger('requests').setLevel(logging.WARNING)
    logging.basicConfig(level=logging.DEBUG)

    plug = PowerPlug(ALIAS)

    while True:
        plug.loop()
