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
import wave

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



def test(ogg, outfile = None):
    numarray.Error.setMode(all="ignore")
    
    vi = vorbispy.info()
    vd = vorbispy.dsp()
    vb = vorbispy.block()
    vc = vorbispy.comment()

    packets = packetsource(ogg)

    vorbis = 0

    sizes = {}

    for packet in packets:
        
        if vorbis < 3:
            rc = vi.synthesis_headerin(vc, packet)
            if rc < 0:
                print "Faulty vorbis header #%d, error %d" % (vorbis, rc)                
                return
            
        elif vorbis == 3: 
            print >> sys.stderr, vc.get_comments()

            channels = vi.get_channels()
            rate     = vi.get_rate()

            if not outfile:
                audio = oss.open_audio()
                if channels >= 2:
                    audio.stereo(1)
                audio.format(oss.AFMT_S16_LE)
                audio.speed(rate)
                samplewriter = audio.write
            else:   # outfile will be a wave_writer object
                audio = wave.open(outfile, "wb")
                audio.setsampwidth(2)
                audio.setframerate(rate)
                audio.setnchannels(channels)
                samplewriter = audio.writeframesraw
        
            vd.synthesis_init(vi)
            vd.block_init(vb)

        if vorbis >= 3:
            if vb.synthesis(packet) == 0:
                vd.synthesis_blockin(vb)

                samples = vd.synthesis_pcmout()
                if samples != None:
                    samples *= 32767;
                    wav = samples.astype('Int16')

                    interleaved = numarray.zeros(samples.shape[1]*channels, numarray.Int16)

                    shape = samples.shape[1]
                    try:
                        sizes[shape] += 1
                    except:
                        sizes[shape] = 1

                    for i in range(channels):
                        interleaved[i::channels] = wav[i]

                    samplewriter(interleaved.tostring())
                    
                    # seems to clean them out the first time.
                    # samples = vd.synthesis_pcmout()
                    # if samples != None:
                    #    print "synthesis_pcmout foo!"


        vorbis += 1

    # set total frames in wav header when done
    if not outfile:
        audio.writeframes('')

    # see how the block sizes go
    print sizes
                                

if __name__ == "__main__":
    outfile = None
    if len(sys.argv) == 3:
        sys.argv.pop()
        outfile = file("decode.wav", "wb")
    if len(sys.argv) == 2:
        if sys.argv[1] == "-":
            test(sys.stdin, outfile)
        else:
            test(file(sys.argv[1], "rb"), outfile)
    else:
        test(file("music.ogg", "rb"), outfile)
