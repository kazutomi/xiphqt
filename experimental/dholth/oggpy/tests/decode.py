#!/usr/bin/python
#
# Partial port of decoder_example.c from the vorbis library.
# Decodes an ogg/vorbis file from stdin, writing the first channel to disk.
#
# Daniel Holth <dholth@fastmail.fm>, 2004

import oss
import sys
import oggpy
import vorbispy
import numarray

BUFSIZE=8192

def test(ogg, outfile=file("decode.raw", "w")):
    numarray.Error.setMode(all="ignore")
    
    oy = oggpy.sync()
    og = oggpy.page()
    op = oggpy.packet()
    
    vi = vorbispy.info()
    vd = vorbispy.dsp()
    vb = vorbispy.block()
    vc = vorbispy.comment()

    while 1:
        buffer = ogg.read(BUFSIZE)
        oy.write(buffer)

        if oy.pageout(og) != 1:
            if len(buffer) < BUFSIZE: break
            print >> sys.stderr, "Input does not appear to be an Ogg bitstream."
            sys.exit(1)

        os = oggpy.stream(og.serialno())

        if os.pagein(og) < 0:
            print >> sys.stderr, "Error reading first Ogg bitstream page"
            sys.exit(1)

        if os.packetout(op) != 1:
            print >> sys.stderr, "Error reading initial header packet"
            sys.exit(1)

        if vi.synthesis_headerin(vc, op) < 0:
            print >> sys.stderr, "Not an ogg/vorbis bitstream."
            sys.exit(1)

        i = 0
        while i < 2:
            while i < 2:
                result = oy.pageout(og)
                if result == 0: break
                if result == 1:
                    os.pagein(og)
                    while i < 2:
                        result = os.packetout(op)
                        if result == 0: break
                        if result < 0:
                            print >> sys.stderr, "corrupt second header."
                            sys.exit(1)
                        vi.synthesis_headerin(vc, op)
                        i += 1
        
            buffer = ogg.read(BUFSIZE)
            if len(buffer) == 0 and i < 2:
                print >> sys.stderr, "EOF before finding all vorbis headers!"
                sys.exit(1)

            oy.write(buffer)

        print >> sys.stderr, vorbispy.get_comments(vc)

        channels = vi.get_channels()
        rate     = vi.get_rate()

        audio = oss.open_audio()
        if channels >= 2:
            audio.stereo(1)
        audio.format(oss.AFMT_S16_LE)
        audio.speed(rate)
        
        vd.synthesis_init(vi)
        vd.block_init(vb)

        eos = False
        while not eos:
            while not eos:
                result = oy.pageout(og)
                if result == 0: break           # need more data
                if result < 0:
                    print >> sys.stderr, "Yikes, corruption! continue..."
                else:
                    os.pagein(og)
                    p = 0
                    while 1:
                        result = os.packetout(op)
                        if result == 0: break   # need more data
                        if result > 0:          # we have a packet. decode it.
                            p += 1
                            if vb.synthesis(op) == 0:
                                vd.synthesis_blockin(vb)
                                                        
                            samples = vorbispy.oggpy_synthesis_pcmout(vd)
                            while samples != None:
                                samples *= 32767;
                                wav = samples.astype('Int16')

                                interleaved = numarray.zeros(samples.shape[1]*channels, numarray.Int16)
                                
                                for i in range(channels):
                                    interleaved[i::channels] = wav[i]
                                
                                # interleaved.tofile(outfile)
                                audio.write(interleaved.tostring())
                                        
                                samples = vorbispy.oggpy_synthesis_pcmout(vd)
                                
                if og.eos(): eos = True
            if not eos:
                buffer = ogg.read(BUFSIZE)
                oy.write(buffer)
                if len(buffer) == 0: eos = True
        

if __name__ == "__main__":
    if len(sys.argv) == 2:
        if sys.argv[1] == "-":
            test(sys.stdin)
        else:
            test(file(sys.argv[1], "rb"))
    else:
        test(file("music.ogg", "rb"))
