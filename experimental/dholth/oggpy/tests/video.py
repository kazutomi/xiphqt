#!/usr/bin/python
#
# Display theora video, silently, via pygame.
#
# Daniel Holth <dholth@fastmail.fm>, 2004

import pygame, os
import pygame.draw
from pygame.locals import *

import theorapy
import oggpy
import time
import pygame_bridge
import theoratest
import sys

def play():
    pygame.init()

    vidsrc = theoratest.test(file(sys.argv[1], "rb"))

    info, comment, state = vidsrc.next()
    print "info", info, "comment", comment, "state", state

    frame_duration = float(info.fps_denominator) / float(info.fps_numerator)
    print "frames per second:", 1/frame_duration
    
    width  = info.width
    height = info.height

    screen      = pygame.display.set_mode((width, height), HWSURFACE|DOUBLEBUF)
    surf        = pygame.display.get_surface()
    surfshot    = pygame.Surface((width, height))
    
    surf.fill((0,0,0))
    pygame.display.flip()

    bridge = pygame_bridge.bridge()

#    bridge.set_surface(surfshot)
    bridge.set_surface(surf)
    bridge.set_info(info)

    frames = 0
    basetime = time.time()

    for frame in vidsrc:
        t = time.time()
        bridge.render(frame)
        frames += 1
        time.sleep(max(0,frame_duration-(time.time()-t)))

if __name__ == "__main__":
    play()
