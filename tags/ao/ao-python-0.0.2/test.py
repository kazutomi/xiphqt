#!/usr/bin/python

import ao

id = ao.get_driver_id('oss')
dev = ao.AudioDevice(id)
f = open('test.wav', 'r')
data = f.read()
dev.play(data, len(data))
