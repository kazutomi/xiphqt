import ossaudiodev
from vorbisfile import *

adev = ossaudiodev.open('w')
adev.setfmt(ossaudiodev.AFMT_S16_BE)
adev.channels(2)
adev.speed(44100)
vf = VorbisFile()
vf.open('/home/jack/test.ogg')

data, cs = vf.read(4096, bendian=1)
while len > 0:
    adev.write(data)
    data, cs = vf.read(4096, bendian=1)
