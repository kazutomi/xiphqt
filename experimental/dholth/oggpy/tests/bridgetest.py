#!/usr/bin/python

import pygame
import pygame_bridge

pygame.init()
pygame.display.set_mode((320,320))
pygame.display.init()

b = pygame_bridge.bridge()

print "is a string a surface?", b.is_surface("foo")
s = b.make_surface()
print "is", s, "a surface?", b.is_surface(s)
