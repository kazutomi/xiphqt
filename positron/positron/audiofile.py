# -*- Mode: python -*-
#
# audiofile.py - audio file handling
#
# Copyright (C) 2003, Xiph.org Foundation
#
# This file is part of positron.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of a BSD-style license (see the COPYING file in the
# distribution).
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the license for more details.

from types import *
import os
from os import path
import string

import MP3Info

def detect(filename):
    metadata = None

    for detect_func in detect_functions:
        metadata = detect_func(filename)
        if metadata != None:
            break
    else:
        metadata = None

    if metadata != None and metadata["title"] == None:
        # Must have title
        metadata["title"] = path.split(filename)[1]

    return metadata

def detect_mp3(filename):
    try:
        f = file(filename, "rb")

        mp3info = MP3Info.MP3Info(f)

        header = mp3info.mpeg
        info = { "type" : "mp3",
                 "size" : header.filesize,
                 "length" : header.length,
                 "title" : mp3info.title,
                 "artist" : mp3info.artist,
                 "album" : mp3info.album,
                 "genre" : mp3info.genre }

        # Convert empty string entries to nulls
        for key in info.keys():
            if info[key] == "":
                info[key] = None

        f.close()

    except MP3Info.Error:
        f.close()
        return None
    except IOError:
        return None
    except OSError:
        return None            

    return info

def detect_oggvorbis(filename):
    try:
        vf = ogg.vorbis.VorbisFile(filename)
        vc = vf.comment()
        vi = vf.info()        

        info = { "type" : "oggvorbis",
                 "size" : os.stat(filename).st_size,
                 "length" : int(vf.time_total(-1)),
                 "title" : None,
                 "artist" : None,
                 "album" : None,
                 "genre" : None }

        actual_keys = map(string.lower, vc.keys())
        
        for tag in ("title","artist","album","genre"):
            if tag in actual_keys:
                value = vc[tag]
                # Force these to be single valued
                if type(value) == ListType or type(value) == TupleType:
                    value = value[0]

                # Convert from Unicode to ASCII since the Neuros can't
                # do Unicode anyway.
                #
                # I will probably burn in i18n hell for this.
                info[tag] = value.encode('ascii','replace')

    except ogg.vorbis.VorbisError:
        return None
    except IOError:
        return None
    except OSError:
        return None

    return info


# Only put the ogg vorbis detection code in the list if
# we have the python module needed.

detect_functions = [detect_mp3]

try:
     import ogg.vorbis
     detect_functions.insert(0, detect_oggvorbis)
except ImportError:
    pass
