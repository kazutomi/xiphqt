import types
from _vorbisfile import *

MODE_RAW = 0
MODE_PCM = 1
MODE_TIME = 2
MODE_PAGE = 4
MODE_LAP = 8

FORMAT_INT = 0
FORMAT_FLOAT = 1

class VorbisFileHoleError(StandardError):
    pass

class VorbisFile(object):
    def __init__(self):
        self.vf = None
        self.testing = 0
        
    def open(self, file=None):
        if not self.testing and not file:
            raise StandardException, "Expected a file object or "\
                  "string, or test mode."
        elif self.testing:
            ret = ov_test_open(self.vf)
            # FIXME: check return value
        else:
            if type(file) == types.StringType:
                self.vf = ov_open(open(file))
            else:
                self.vf = ov_open(file)

        return self.vf != None

    def test(self, file):
        self.testing = 1
        
        if type(file) == types.StringType:
            self.vf = ov_test(open(file))
        else:
            self.vf = ov_test(file)

        return self.vf != None

    def clear(self):
        return ov_clear(self.vf)
    
    def bitrate(self, link=-1, instant=0):
        if instant:
            return ov_bitrate_instant(self.vf)
        else:
            return ov_bitrate(self.vf, link)

    def links(self):
        return ov_streams(self.vf)

    def seekable(self):
        return ov_seekable(self.vf)

    def serialnumber(self, link=-1):
        return ov_serialnumber(self.vf, link)

    def length(self, link=-1, mode=MODE_TIME):
        if mode == MODE_RAW:
            return ov_raw_total(self.vf, link)
        elif mode == MODE_PCM:
            return ov_pcm_total(self.vf, link)
        elif mode == MODE_TIME:
            return ov_time_total(self.vf, link)
        else:
            raise StandardError, "Unknown mode requested"

    def seek(self, pos, mode=MODE_TIME):
        if mode == MODE_RAW:
            return ov_raw_seek(self.vf, pos)
        elif mode == MODE_RAW | MODE_LAP:
            return ov_raw_seek_lap(self.vf, pos)
        elif mode == MODE_PCM:
            return ov_pcm_seek(self.vf, pos)
        elif mode == MODE_PCM | MODE_LAP:
            return ov_pcm_seek_lap(self.vf, pos)
        elif mode == MODE_PCM | MODE_PAGE:
            return ov_pcm_seek_page(self.vf, pos)
        elif mode == MODE_PCM | MODE_PAGE | MODE_LAP:
            return ov_pcm_seek_page_lap(self.vf, pos)
        elif mode == MODE_TIME:
            return ov_time_seek(self.vf, pos)
        elif mode == MODE_TIME | MODE_LAP:
            return ov_time_seek_lap(self.vf, pos)
        elif mode == MODE_TIME | MODE_PAGE:
            return ov_time_seek_page(self.vf, pos)
        elif mode == MODE_TIME | MODE_PAGE | MODE_LAP:
            return ov_time_seek_page_lap(self.vf, pos)
        else:
            raise StandardError, "Unknown mode requested"

    def tell(self, mode=MODE_TIME):
        if mode == MODE_RAW:
            return ov_raw_tell(self.vf)
        elif mode == MODE_PCM:
            return ov_pcm_tell(self.vf)
        elif mode == MODE_TIME:
            return ov_time_tell(self.vf)
        else:
            raise StandardError, "Unknown mode requested"

    def info(self):
        return ov_info(self.vf)

    def comments(self):
        return ov_comment(self.vf)

    def read(self, num, format=FORMAT_INT, wordsize=2, signed=1, bendian=0):
        if format == FORMAT_FLOAT:
            samps, data, cs = ov_read_float(self.vf, num)
            return data, cs
        elif format == FORMAT_INT:
            real_num = num * wordsize * 2 # FIXME: get number channels
            samps, data, cs = ov_read(self.vf, real_num, bendian,
                                      wordsize, signed)
            if samps < 0:
                raise VorbisFileHoleError, "vorbisfile error %d" % samps
            return data, cs
        else:
            raise StandardError, "Unknown format requested"

    def halfrate(self, flag):
        return ov_halfrate(self.vf, flag)

    def halfratable(self):
        return ov_halfrate(self.vf)
