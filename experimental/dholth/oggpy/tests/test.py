#!/usr/bin/python
#
# Grab comments from an ogg/vorbis file.
#
# Daniel Holth <dholth@fastmail.fm>
# 20 Jan. 2004

import oggpy
import vorbispy
import sys

def test(ogg = open("music.ogg", "rb")):
    sync = oggpy.sync()
    page = oggpy.page()
    pkt  = oggpy.packet()

    sync.write(ogg.read(4096))

    if sync.pageout(page) != 1:
        raise "Input doesn't seem to be an ogg bitstream."

    strm = oggpy.stream(page.serialno())
    info = vorbispy.info()
    cmt  = vorbispy.comment()

    print "pagein",     strm.pagein(page)
    print "packetout",  strm.packetout(pkt)
    print "headerin",   info.synthesis_headerin(cmt, pkt)

    i = 0

    while i<2:
        while i<2:
            result = sync.pageout(page)
            if result == 0: break   # get more yummy data
            strm.pagein(page)
            while i<2:
                result = strm.packetout(pkt)
                if result == 0: break # mmm... data
                if result < 0:
                    raise "can't handle ogg/vorbis corruption this early."
                info.synthesis_headerin(cmt, pkt)
                i = i + 1
        sync.write(ogg.read(4096))

    print cmt.query_count("artist"), " 'artist' tag[s]"
    print cmt.get_comments()


if __name__ == "__main__":
    if len(sys.argv) == 2:
        test(open(sys.argv[1]))
    else:
        test()
