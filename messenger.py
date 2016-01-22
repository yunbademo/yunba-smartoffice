#!/usr/bin/env python

import time
import sys
import logging
from socketIO_client import SocketIO

APPKEY = '56a0a88c4407a3cd028ac2fe'
TOPIC = 'smart_office'

class Messenger:

    def __init__(self, alias):
        logging.info('init')

        self.appkey = APPKEY
        self.customid = alias
        self.topic = TOPIC
        self.alias = alias

        self.socketIO = SocketIO('sock.yunba.io', 3000)
        self.socketIO.on('socketconnectack', self.on_socket_connect_ack)
        self.socketIO.on('connack', self.on_connack)
        self.socketIO.on('puback', self.on_puback)
        self.socketIO.on('suback', self.on_suback)
        self.socketIO.on('message', self.on_message)
        self.socketIO.on('set_alias_ack', self.on_set_alias)
        self.socketIO.on('get_topic_list_ack', self.on_get_topic_list_ack)
        self.socketIO.on('get_alias_list_ack', self.on_get_alias_list_ack)
#        self.socketIO.on('puback', self.on_publish2_ack)
        self.socketIO.on('recvack', self.on_publish2_recvack)
        self.socketIO.on('get_state_ack', self.on_get_state_ack)
        self.socketIO.on('alias', self.on_alias)

    def __del__(self):
        logging.info('del')

    def loop(self):
        self.socketIO.wait(seconds=0.002)

    def on_socket_connect_ack(self, args):
        logging.debug('on_socket_connect_ack: %s', args)
        self.socketIO.emit('connect', {'appkey': self.appkey, 'customid': self.customid})

    def on_connack(self, args):
        logging.debug('on_connack: %s', args)
        self.socketIO.emit('set_alias', {'alias': self.alias})

    def on_puback(self, args):
        logging.debug('on_puback: %s', args)

    def on_suback(self, args):
        logging.debug('on_suback: %s', args)
        self.socketIO.emit('set_alias', {'alias': self.alias})

    def on_message(self, args):
        logging.debug('on_message: %s', args)

    def on_set_alias(self, args):
        logging.debug('on_set_alias: %s', args)

    def on_get_alias(self, args):
        logging.debug('on_get_alias: %s', args)

    def on_alias(self, args):
        logging.debug('on_alias: %s', args)

    def on_get_topic_list_ack(self, args):
        logging.debug('on_get_topic_list_ack: %s', args)

    def on_get_alias_list_ack(self, args):
        logging.debug('on_get_alias_list_ack: %s', args)

    def on_publish2_ack(self, args):
        logging.debug('on_publish2_ack: %s', args)

    def on_publish2_recvack(self, args):
        logging.debug('on_publish2_recvack: %s', args)

    def on_get_state_ack(self, args):
        logging.debug('on_get_state_ack: %s', args)

    def publish(self, msg, qos):
        logging.debug('publish: %s', args)
        self.socketIO.emit('publish', {'topic': self.topic, 'msg': msg, 'qos': qos})

if __name__ == '__main__':

    #logging.getLogger('requests').setLevel(logging.WARNING)
    logging.basicConfig(level=logging.DEBUG)

    m = Messenger('messenger', 'messenger')

    while True:
        m.loop()

