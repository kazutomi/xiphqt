import struct
import ossaudiodev
import _vorbisfile

adev = ossaudiodev.open('w')
adev.setfmt(ossaudiodev.AFMT_S16_BE)
adev.channels(2)
adev.speed(44100)
vf = _vorbisfile.ov_open(open('/home/jack/test.ogg'))

num, data, cs = _vorbisfile.ov_read_float(vf, 4096)
while num > 0:
    buffer = ''
    for j in range(num):
          l = int(data[0][j]*32768)
	  r = int(data[1][j]*32768)
	  if l > 32767: l = 32767
	  if r > 32767: r = 32767
	  if l < -32768: l = -32768
	  if r < -32768: r = -32768
          buffer += struct.pack("hh", l, r)

    adev.write(buffer)

    num, data, cs = _vorbisfile.ov_read_float(vf, 4096)
