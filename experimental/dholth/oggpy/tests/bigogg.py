#!/usr/bin/python
#
# Find out what happens when big ogg packets are used.
# Hint: they are spread across several ogg pages.
#
# Daniel Holth <dholth@fastmail.fm>, 2004

import oggpy
import sys

granule  = 0

os = oggpy.stream(0x00FF00FF)
page = oggpy.page()

pack = oggpy.userpacket("OGGText", granule, True, False)
os.packetin(pack)

os.flush(page)
sys.stdout.write(page.header())
sys.stdout.write(page.body())


bigdata = " Ogg Data " * 1024

pack = oggpy.userpacket(bigdata, granule, False, False)
os.packetin(pack)

while os.pageout(page):
    sys.stdout.write(page.header())
    sys.stdout.write(page.body())

for data in sys.stdin.readlines():
    if data[0] == chr(12):
        while(os.flush(page) != 0):
            sys.stdout.write(page.header())
            sys.stdout.write(page.body())
    granule += 1
    pack = oggpy.userpacket(data, granule, False, False)
    os.packetin(pack)

pack = oggpy.userpacket("", granule, False, True)
os.packetin(pack)
os.pageout(page)
sys.stdout.write(page.header())
sys.stdout.write(page.body())
