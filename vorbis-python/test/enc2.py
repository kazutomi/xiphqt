#!/usr/bin/env python
import ogg.vorbis, audiofile
vd = ogg.vorbis.VorbisInfo(nominal_bitrate=150000).analysis_init()
os = ogg.OggStreamState(5)
map(os.packetin, vd.headerout())
fout = open('out.ogg', 'w')
inwav = audiofile.WavReader('in.wav')
og = os.flush()
while og:
    og.writeout(fout)
    og = os.flush()
while 1:
    channel_data = inwav.read_channels(1024)
    if not channel_data[0]: break
    apply(vd.write, channel_data) 
    vb = vd.blockout()
    while vb:
        os.packetin(vb.analysis())
        while 1:
            og = os.pageout()
            if not og: break
            og.writeout(fout)
        vb = vd.blockout()





