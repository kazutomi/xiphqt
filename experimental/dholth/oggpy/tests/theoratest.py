#!/usr/bin/python
#
# Display some basic information about ogg/theora files.
#
# Daniel Holth <dholth@fastmail.fm>, 2004

import oggpy
import vorbispy
import theorapy
import sys
import numarray
import inspect

BUFSIZE=8192

def packetsource(ogg):
    """Generator to take ogg packets out of a file.  Only returns packets from the first stream."""
    
    state = 0
    
    oy = oggpy.sync()
    og = oggpy.page()
    op = oggpy.packet()

    buffer = True   # prime the pump
    while buffer:
        buffer = ogg.read(BUFSIZE)
        oy.write(buffer)
        while(oy.pageout(og)):
            if state == 0:
                stream = oggpy.stream(og.serialno())
                state += 1
            stream.pagein(og)
            while(stream.packetout(op) == 1):
                yield op


def test(ogg):
    """Display some basic information about a theora stream."""

    numarray.Error.setMode(all="ignore")

    oggpy.sync.write = oggpy.sync_write # hack until the wrapper can do this itself
    
    ti = theorapy.info()
    tc = theorapy.comment()
    td = theorapy.state()

    yuv = theorapy.yuv_image()

    theora = 0

    for op in packetsource(ogg):
        if theora == 0:
            head = ti.decode_header(tc, op)
            if head >= 0:
                print "got theora", head
                theora += 1
            else:
                print "no theora"
                return
                
        elif theora < 3:   # should handle packets that are more than one page long.
            h = ti.decode_header(tc, op)
            print "header ", h
            theora += 1

        elif theora == 3:
            print "This theora encoded by: ", tc.get_vendor()
            print "Trying to decode with: ", theorapy.version_string(), theorapy.version_number()
            for i in dir(ti):
                if i[0:2] != "__":
                    try:
                        value = ti.__getattribute__(i)
                        if not inspect.ismethod(value):
                            print i, value
                    except TypeError:
                        pass
            print "decode init returns", td.decode_init(ti)

            print "comments:"
            for i in range(tc.size()):
                print tc.query_index(i)
            print ":end comments"

            theora += 1
           
        elif theora > 3 and theora < 10:
            print theora
            theora += 1
            assert(td.decode_packetin(op) == 0)
            assert(td.decode_YUVout(yuv) == 0)
            print "yuv info:"
            print "y\t%d\t%d\t%d" % (yuv.y_width, yuv.y_height, yuv.y_stride)
            print "uv\t%d\t%d\t%d" % (yuv.uv_width, yuv.uv_height, yuv.uv_stride)
        else:
            break


if __name__ == "__main__":
    if len(sys.argv) == 2:
        filename = sys.argv[1]
    else:
        filename = "video.ogg"
        
    test(file(filename, "rb"))
