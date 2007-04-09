#!/usr/bin/python
#
# The new line-based text format, oggtext!
#
# Reads lines of text from an ogg-format stream.
#
# Daniel Holth <dholth@fastmail.fm>, 2004
# Charles Duffy <cduffy@spamcop.net>, 2007

import oggpy
import sys

BUFSIZE=8192

def packetsource(f, header):
    """Generator to take ogg packets out of a file; filters for streams
    whose header begins with a specific string, and returns tuples of the
    form (stream_num, page)."""
    streams = {}
    ignore_streams = set()
    
    oy = oggpy.sync()
    og = oggpy.page()
    op = oggpy.packet()
    
    packet_buffer = True   # prime the pump
    while packet_buffer:
        packet_buffer = f.read(BUFSIZE)
        oy.write(packet_buffer)
        while(oy.pageout(og)):
            stream_num = og.serialno()
            if stream_num in ignore_streams:
                continue
            if not streams.has_key(stream_num):
                stream = oggpy.stream(stream_num)
                stream.pagein(og)
                stream.packetout(op)
                if op.bos() and op.get_bytes()[:len(header)] == header:
                    streams[stream_num] = stream
                    yield (stream_num, op)
                else:
                    ignore_streams.add(stream_num)
                continue
            if streams.has_key(stream_num):
                stream = streams[stream_num]
                stream.pagein(og)
                while(stream.packetout(op) == 1):
                    yield (stream_num, op)

def test(f):
    ps = packetsource(f, 'OGGText')
    for streamnum, packet in ps:
        if not packet.bos():
            sys.stdout.write(packet.get_bytes())

if __name__ == '__main__':
    infile = None
    if len(sys.argv) < 2 or sys.argv[1] == '-':
        infile = sys.stdin
    else:
        infile = open(sys.argv[1], 'r')
    test(infile)
