import types
from _vorbisfile import *

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
    
    def bitrate(self, link=-1):
        return ov_bitrate(self.vf, link)

    def bitrate_instant(self):
        return ov_bitrate_instant(self.vf)

    def links(self):
        return ov_streams(self.vf)

    def seekable(self):
        return ov_seekable(self.vf)

    def serialnumber(self, link=-1):
        return ov_serialnumber(self.vf, link)

    def raw_length(self, link=-1):
        return ov_raw_total(self.vf, link)
