#!/usr/bin/env python

import time
import sys
import json
import logging
import RPi.GPIO as GPIO
from messenger import Messenger

logger = logging.getLogger('power_plug')

APPKEY = '5697113d4407a3cd028abead'
ALIAS = 'power_plug'
GPIO_NUM = 4

class PowerPlug(Messenger):

    def __init__(self, appkey, alias, customid, gpio_num):
        self.__logger = logging.getLogger('power_plug.PowerPlug')
        self.__logger.debug('init')
        Messenger.__init__(self, appkey, alias, customid)

        self.gpio_num = gpio_num
        GPIO.setup(self.gpio_num, GPIO.OUT)

    def __del__(self):
        self.__logger.debug('del')

    def on_message(self, args):
        self.__logger.debug('on_message: %s', args)
        try:
            msg = json.loads(args['msg'])
        except Exception as e:
            self.__logger.warning(e)
            return

        if msg['m'] == 'on':
            self.__logger.debug('on')
            GPIO.output(self.gpio_num, GPIO.HIGH)
        elif msg['m'] == 'off':
            self.__logger.debug('off')
            GPIO.output(self.gpio_num, GPIO.LOW)

if __name__ == '__main__':

    logging.basicConfig(level=logging.DEBUG)

    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(GPIO_NUM, GPIO.OUT)

    plug = PowerPlug(APPKEY, ALIAS, ALIAS, GPIO_NUM)

    while True:
        plug.loop()
