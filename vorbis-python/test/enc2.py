#!/usr/bin/env python
'''An example of encoding using the Python wave module'''
import ogg.vorbis, wave


# Used to switch between different ways of writing the file out
write_as_string = 1

fout = open('out.ogg', 'wb')
inwav = wave.open('in.wav','rb')
channels = inwav.getnchannels()
vd = ogg.vorbis.VorbisInfo(channels = channels,
                           rate = inwav.getframerate(),
                           quality = 0.2).analysis_init()
os = ogg.OggStreamState(5)
map(os.packetin, vd.headerout())
og = os.flush()
while og:
    og.writeout(fout)
    og = os.flush()
nsamples = 1024

def FinishPage(ogg_page):
    """Take a finished ogg page object and write it out to a file."""
    if not write_as_string:
        ogg_page.writeout(fout)  # write it to a file
    else:
        # We'll do this the longer way
        data = ogg_page.tostring()
        fout.write(data)

eos = 0
total = 0
while not eos:
    data = inwav.readframes(nsamples)
    total = total + nsamples
    if not data:
        vd.write(None)
    else:
        vd.write_wav(data)
    print 100.0 * total / inwav.getnframes()
    vb = vd.blockout()
    while vb:
        vb.analysis()
        vb.addblock()

        op = vd.bitrate_flushpacket()
        while op:
            os.packetin(op)
            while not eos:
                og = os.pageout()
                if not og: break
                FinishPage(og)
                eos = og.eos()
            op = vd.bitrate_flushpacket()
        vb = vd.blockout()





