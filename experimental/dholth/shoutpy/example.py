#!/usr/bin/python
#
# Simple streaming to icecast.
#
# Daniel Holth, 2004

import sys
import shoutpy

s           = shoutpy.Shout()
s.user      = "source"
s.password  = "hackme"
s.mount     = "/shoutpy.ogg"
s.port      = 8001
s.format    = shoutpy.FORMAT_VORBIS

s.open()

for arg in sys.argv[1:]:
    f = open(arg, "rb")

    buf = f.read(4096)

    while buf:
        s.send(buf)
        buf = f.read(512)
        s.sync()

    f.close()

s.close()
