#!/usr/bin/python
#
# The new line-based text format, oggtext!
#
# Puts lines of text into a new ogg-format stream.
#
# Daniel Holth <dholth@fastmail.fm>, 2004

import oggpy
import sys

granule  = 0

os = oggpy.stream(0x00FF00FF)
page = oggpy.page()

pack = oggpy.userpacket("OGGText", granule, True, False)
os.packetin(pack)

os.pageout(page)
sys.stdout.write(page.header())
sys.stdout.write(page.body())

for data in sys.stdin.readlines():
    if(os.pageout(page) != 0):
        sys.stdout.write(page.header())
        sys.stdout.write(page.body())
    granule += len(data)
    pack = oggpy.userpacket(data, granule, False, False)
    os.packetin(pack)

pack = oggpy.userpacket("END", granule, False, True)
os.packetin(pack)
os.pageout(page)
sys.stdout.write(page.header())
sys.stdout.write(page.body())
