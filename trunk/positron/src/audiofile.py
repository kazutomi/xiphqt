import MP3Info
from os import path

def copy_file (src_filename, dest_filename):
    blocksize = 1048576
    src = file(src_filename, "rb")
    dest = file(dest_filename, "wb")

    block = src.read(blocksize)
    while block != "":
        dest.write(block)
        block = src.read(blocksize)

    src.close()
    dest.close()


class AudioFile:
    """Base class for various formats.  Defines common operatons and API.

    Do not instantiate this class.  It will not do what you want."""
    
    active = False

    def detect(self, filename):
        self._fail("detect")

    def get_info(self, filename):
        self._fail("open")

    def _fail(self, funcname):
        raise Error("Someone forgot to implement %s.%s()."
                    % (self.__class__.__name__, funcname))

class MP3File(AudioFile):
    def detect(self, filename):
        try:
            f = file(filename, "rb")
            m = MP3Info.MPEG(f)
        except MP3Info.Error:
            f.close()
            return False
        except IOError:
            return False
        
        return True

    def get_info(self, filename):
        f = file(filename, "rb")

        mp3info = MP3Info.MP3Info(f)

        header = mp3info.mpeg
        info = { "size" : header.filesize,
                 "length" : header.length,
                 "title" : None,
                 "artist" : None,
                 "album" : None,
                 "genre" : None }

        if mp3info.id3 != None:
            id3 = mp3info.id3

            info["title"] = id3.title
            info["artist"] = id3.artist
            info["album"] = id3.album
            info["genre"] = id3.genre

        # Convert empty string entries to nulls
        for key in info.keys():
            if info[key] == "":
                info[key] = None

        # Must have title
        if info["title"] == None:
            info["title"] = path.split(filename)[1]

        f.close()
        
        return info
