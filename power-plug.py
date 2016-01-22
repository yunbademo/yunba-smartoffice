#!/usr/bin/env python

import time
import sys
import logging
from messenger import Messenger

ALIAS = 'smart_power_plug'

class SmartPowerPlug(Messenger):

    def __init__(self, alias):
        logging.debug('init')
        Messenger.__init__(self, alias)

    def __del__(self):
        logging.debug('del')

if __name__ == '__main__':

    #logging.getLogger('requests').setLevel(logging.WARNING)
    logging.basicConfig(level=logging.DEBUG)

    plug = SmartPowerPlug(ALIAS)

    while True:
        plug.loop()
