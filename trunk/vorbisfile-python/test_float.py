import struct
import ossaudiodev
from vorbisfile import *

adev = ossaudiodev.open('w')
adev.setfmt(ossaudiodev.AFMT_S16_BE)
adev.channels(2)
adev.speed(44100)

vf = VorbisFile()
vf.open('/home/jack/test.ogg')


data, cs = vf.read(8192, format=FORMAT_FLOAT)
while len(data) > 0:
    buffer = ''
    for j in range(len(data[0])):
        l = int(data[0][j]*32768)
        r = int(data[1][j]*32768)
        if l > 32767: l = 32767
        if r > 32767: r = 32767
        if l < -32768: l = -32768
        if r < -32768: r = -32768
        buffer += struct.pack("hh", l, r)

    adev.write(buffer)
    data, cs = vf.read(8192, format=FORMAT_FLOAT)
