#!/usr/bin/python
#
# Encoding+streaming example.
# This is approximately a port of libvorbis's encoder_example.c
# It expects 44kHz 16-bit stereo input.
#
# Daniel Holth <dholth@fastmail.fm>, 2004

import numarray
import oggpy
import random
import shoutpy
import sys
import vorbispy
from numarray import array

# samples to read at one time, halving or quartering doesn't slow things down
READ = 8192

def test(indata):
    numarray.Error.setMode(all="ignore")

    s = shoutpy.Shout()
    s.user      = "source"
    s.password  = "hackme"
    s.mount     = "/shoutpy.ogg"
    s.port      = 8001
    s.format    = shoutpy.FORMAT_VORBIS
    s.open()
    
    og = oggpy.page()
    op = oggpy.packet()
    vi = vorbispy.info()
    vc = vorbispy.comment()
    vd = vorbispy.dsp()
    vb = vorbispy.block()

    vi.encode_init_vbr(2, 44100, 0.0)
    vc.add_tag("ENCODER", "stream.py")
    vc.add_tag("ARTIST", "unknown")
    vd.analysis_init(vi)
    vd.block_init(vb)
    
    randomnumber = random.randrange(0,2**30)
    os = oggpy.stream(randomnumber)

    header      = oggpy.packet()
    header_comm = oggpy.packet()
    header_code = oggpy.packet()

    vd.analysis_headerout(vc, header, header_comm, header_code)

    os.packetin(header)
    os.packetin(header_comm)
    os.packetin(header_code)

    # ensure the actual audio data will start on a new page
    eos = False
    while not eos:
        result = os.flush(og)
        if result == 0: break
        s.send(og.header())
        s.send(og.body())

    while not eos:
        data  = indata.read(READ*4)
        if not data:
            vd.analysis_write(None)
        else:
            interleaved = numarray.fromstring(data, numarray.Int16)
            # better convert to float, from -1.0 to 1.0
            deinterleaved = numarray.array((interleaved[0::2], interleaved[1::2]), numarray.Float32)
            deinterleaved *= 1.0/32768.0
            vd.analysis_write(deinterleaved)
 
        while(vd.analysis_blockout(vb)==1):
            # will have to be able to take null/None to use bitrate management:
            vb.analysis(op)
            os.packetin(op)
            
            while not eos:
                result = os.pageout(og)
                if result == 0: break

                s.send(og.header())
                s.send(og.body())
                s.sync()

                if og.eos(): eos = 1
    
    del og


if __name__ == "__main__":
    test(file("./decode.raw", 'rb'))
