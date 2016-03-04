#!/usr/bin/env python

import time
import sys
import logging
from socketIO_client import SocketIO
from messenger import Messenger

logger = logging.getLogger('plug_status')
#APPKEY = '56a0a88c4407a3cd028ac2fe'
#TOPIC = 'officeofficeofficeoffice'
#PLUG_ALIAS = 'plug_plc'
APPKEY = '5697113d4407a3cd028abead'
TOPIC = 'yunba_smart_plug'
PLUG_ALIAS = 'plc_0'

class Status(Messenger):

    def __init__(self):
        Messenger.__init__(self, APPKEY, 'status', TOPIC, 'status')
        self.__logger = logging.getLogger('plug.Status')
        self.__logger.info('init')

    def __del__(self):
        self.__logger.info('del')

    def on_set_alias(self, args):
        self.__logger.debug('on_set_alias: %s', args)
        self.publish_to_alias(PLUG_ALIAS, '{"cmd": "plug_get", "devid": "' + PLUG_ALIAS + '"}')


if __name__ == '__main__':

    logging.basicConfig(level=logging.DEBUG)

    s = Status()

    while True:
        s.loop()
        time.sleep(0.02)

