import ossaudiodev
import _vorbisfile

adev = ossaudiodev.open('w')
adev.setfmt(ossaudiodev.AFMT_S16_BE)
adev.channels(2)
adev.speed(44100)
vf = _vorbisfile.ov_open(open('/home/jack/test.ogg'))

len, data, cs = _vorbisfile.ov_read(vf, 4096, 1, 2, 1)
while len > 0:
    adev.write(data)
    len, data, cs = _vorbisfile.ov_read(vf, 4096, 1, 2, 1)
