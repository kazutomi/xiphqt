# -*- Mode: python -*-
#
# cmd_list.py - list command
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

"""positron list:\tLists files in the database

  positron list

     Lists files in the audio database

  positron list <database1> <database2> ...

     Packs all entries from a particular database.  Valid database names
     are: audio, pcaudio, unidedhisi, idedhisi, failedhisi
"""

from neuros import Neuros
import neuros as neuros_module
import util


def display_field(field):
    if len(field) == 0:
        print "None",
    else:
        print field[0],
        for item in field[1:]:
            print ",", item,

def display_audio_record(neuros, record):
    print "Title: %s" % (record[0],)
    print "Artist: %-30s\tAlbum: %-30s" % (record[2], record[3])
    print "Genre: %-12s Time: %4ds  Size: %5dkB" % (record[4], record[6],
                                                   record[7])
    if record[5] != None:
        print "Recording Source: %s" % (record[5],)
        
    if len(record[1]) > 0:
        print "Playlist: ",
        display_field(record[1])

    print "Filename: %s" % (neuros.neurospath_to_hostpath(record[8]),)

    
def run(config, neuros, args):
    if len(args) == 0:
        args = ["audio"]

    for arg in args:

        if arg == "audio":
            display_record = display_audio_record
        else:
            print "Listing database \"%s\" unsupported at this time."
            continue
        
        try:
            database = neuros.open_db(arg)
            print "===== Database \"%s\" =====" % (arg,)

            
            records = database.get_records()

            for record in records:
                if record != None:
                    display_record(neuros, record)
                    print

            neuros.close_db(arg)
        except neuros_module.Error, e:
            print "Error:", e
