import os
from os import path
import zipfile
import StringIO

import audio_zip
import pcaudio_zip
import idedhisi_zip
import unidedhisi_zip
import failedhisi_zip

class Error(Exception):
    """Base class for exceptions in this module."""
    pass


zips = { "audio"      : audio_zip.data,
         "pcaudio"    : pcaudio_zip.data,
         "idedhisi"   : idedhisi_zip.data,
         "unidedhisi" : unidedhisi_zip.data,
         "failedhisi" : failedhisi_zip.data }


def unpack(dbname, target_dir):
    """Unzips dbname and writes the contents to target_dir"""

    if dbname not in zips.keys():
        raise Error("%s is not a valid database name" % (dbname,))

    f = StringIO.StringIO(zips[dbname])
    zip = zipfile.ZipFile(f)

    for item in zip.namelist():
        fullpath = path.join(target_dir, item)

        if fullpath.endswith("/"):
            os.mkdir(fullpath)
        else:
            target_file = file(fullpath, "wb")
            data = zip.read(item)
            target_file.write(data)
            target_file.close()

    zip.close()
