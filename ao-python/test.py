#!/usr/bin/python

import ao

dev = ao.AudioDevice('wav', bits=16)
f = open('test.wav', 'r')
data = f.read()
dev.play(data, len(data))

