# -*- Mode: python -*-
#
# cmd_rebuild.py - rebuild command
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

"""positron rebuild:\tRecreates the Positron databases

  positron rebuild

     Rebuilds all the databases from scratch.

Note that positron will locate all of the music files already on the
Neuros and put their entries back into the database as best as it can
guess.  The database of music stored on your computer (\"pcaudio\") is
cleared and all HiSi clips will be marked as unidentified.
"""

from neuros import Neuros
import neuros as neuros_module
import util
from add_file import *
from os import path
import re

hisi_source_regex = re.compile(r"^hisi(\d+)")

def recording_source(p):
    basename = path.basename(p).lower()

    if basename.startswith("fm"):
        return "FM Radio"
    elif basename.startswith("mic"):
        return "Microphone"
    else:
        return None

def hisi_source(p):
    basename = path.basename(p).lower()

    match = hisi_source_regex.search(basename)
    if match == None:
        return None
    else:
        str = match.group(1)
        return "FM "+str[:-1]+"."+str[-1:]
    
def test_recording(neuros, s):
    return neuros.hostpath_to_neurospath(s[0]).lower().startswith("c:/woid_record")

def test_hisi(neuros, s):
    return neuros.hostpath_to_neurospath(s[0]).lower().startswith("c:/woid_hisi")


def list_split(list, test_func):
    match = []
    no_match = []
    for item in list:
        if test_func(item):
            match.append(item)
        else:
            no_match.append(item)

    return (match, no_match)


def run(config, neuros, args):

    try:
        print "Creating empty databases..."

        for d in neuros.db_formats.keys():
            print "  "+d
            neuros.new_db(d)

        print "Clearing track number cache..."

        try:
            os.remove(path.join(*neuros.mountpoint_parts +
                                [neuros.DB_DIR, 'tracks.txt']))
        except Exception:
            pass    # Silently fail
        
        print "\nFinding existing audio files on the Neuros..."
        # Now we need to find all the files to readd them to the database.
        filelist = [item[1:] for item in
                    gen_filelist(neuros,"",neuros.mountpoint,"",
                                 config.supported_music_types(),
                                 silent=True)]

        test_rec_func = lambda s: test_recording(neuros, s)
        test_hisi_func = lambda s: test_hisi(neuros, s)
        
        (recordings, rest) = list_split(filelist, test_rec_func)
        (hisi, rest) = list_split(rest, test_hisi_func)

        print "  %d found." % (len(filelist),), " "*60

        print "\nAdding music tracks to audio database..."
        audio_db = neuros.open_db("audio")
        for track,metadata in rest:
            print "  "+path.basename(track)
            add_track(neuros, None, track, metadata)

        print "\nAdding recordings to audio database..."
        for track,metadata in recordings:
            print "  "+path.basename(track)
            add_track(neuros, None, track, metadata,
                      recording=recording_source(track))
        if config.sort_database:
            audio_db.sort(path.join(*neuros.mountpoint_parts +
                                    [neuros.DB_DIR, 'tracks.txt']))
        neuros.close_db("audio")

        print "\nAdding HiSi clips to unidedhisi database..."
        unidedhisi_db = neuros.open_db("unidedhisi")
        for track,metadata in hisi:
            basename = path.basename(track)
            print "  "+basename
            record = (basename, hisi_source(track),
                      neuros.hostpath_to_neurospath(track))
            unidedhisi_db.add_record(record)
        if config.sort_database:
            unidedhisi_db.sort(path.join(*neuros.mountpoint_parts +
                                         [neuros.DB_DIR, 'tracks.txt']))
        neuros.close_db("unidedhisi")
        
    except neuros_module.Error, e:
        print "Error:", e
