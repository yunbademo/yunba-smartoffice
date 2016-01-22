#!/usr/bin/env python

import time
import sys
import json
import logging
import RPi.GPIO as GPIO
from messenger import Messenger

ALIAS = 'power_plug'
GPIO_NUM = 4

class PowerPlug(Messenger):

    def __init__(self, alias, gpio_num):
        logging.debug('init')
        Messenger.__init__(self, alias)

        self.gpio_num = gpio_num
        GPIO.setup(self.gpio_num, GPIO.OUT)

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
            GPIO.output(self.gpio_num, GPIO.HIGH)
        elif msg['m'] == 'off':
            logging.debug('off')
            GPIO.output(self.gpio_num, GPIO.LOW)

if __name__ == '__main__':

    #logging.getLogger('requests').setLevel(logging.WARNING)
    logging.basicConfig(level=logging.DEBUG)

    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(GPIO_NUM, GPIO.OUT)

    plug = PowerPlug(ALIAS, GPIO_NUM)

    while True:
        plug.loop()
