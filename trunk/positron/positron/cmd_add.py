# -*- Mode: python -*-
#
# cmd_add.py - add command
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

"""positron add:\tAdds files to the Neuros database, copying as necessary

  positron add <sourcefile> <targetfile>

     Copies sourcefile to targetfile (a path on the Neuros that does not exist)

  positron add <sourcefile1> <sourcefile2> ... <targetdir>

     Copies sourcefiles to targetdir (a directory on the Neuros that already
     exists)

  positron add <sourcefile1> <sourcefile2> ...

     Copies sourcefiles to the default music directory.

Files that are already on the Neuros will not be overwritten, but just
added to the database if not already present.  Non-music files are
ignored.
"""

import os
from os import path
from neuros import Neuros
from add_file import *
import util

def run(config, neuros, args):
    audio_db = neuros.open_db("audio")

    # There are a couple different cases we could be dealing with:
    # 1) Last argument is a filename on the Neuros that does not exist.
    #    => First argument is the source name, last is the target name
    # 2) Last argument is an existing directory on the Neuros
    #    => Previous arguments are all source names, target names are
    #       last plus basename of the source
    # 3) Last argument is not one of the previous two cases
    #    => Use case 2, but create a new argument that is the default music
    #       directory
    # *) Special case: Any source names that resolve to Neuros paths
    #    automatically have the same target name as source name (and then
    #    the source name is nulled to signal that no copying is required)

    filelist = []

    if len(args) == 0:
        print __doc__
        return
    
    # Deal with case 3 first
    if not neuros.is_valid_hostpath(args[-1]) or path.isfile(args[-1]):
        musicdir = path.join(neuros.mountpoint, config.neuros_musicdir)

        if not path.exists(musicdir):
            os.makedirs(musicdir)  # Does not work on Win32 UNC paths

        args.append(musicdir)

    # Now we are only in case 1 or 2
    if neuros.is_valid_hostpath(args[-1]) and not path.exists(args[-1]):
        if len(args) != 2 or path.isdir(args[0]):
            print "Error: Cannot copy multiple files to %s if it is not a directory." % (args[-1],)
            return

        filelist.append((args[0], args[1]))
    elif neuros.is_valid_hostpath(args[-1]) and path.isdir(args[-1]):
        print "Generating file list..."
        for arg in args[:-1]:
            (prefix, suffix) = path.split(arg)
            filelist.extend(gen_filelist(neuros, prefix, suffix, args[-1],
                                         config.supported_music_types()))
            

    if len(filelist) == 0:
        print "No files left to add!"
    elif len(filelist) == 1:
        (sourcename, targetname, metadata) = filelist[0]
        print "Adding %s to the Neuros..." % (sourcename,),
        add_track(neuros, sourcename, targetname, metadata)
        print
    else:
        print "Adding %d tracks to the Neuros..." % (len(filelist),)
        i = 1
        for sourcename, targetname, metadata in filelist:
            print "  %d. %s..." % (i, sourcename)
            i += 1
            add_track(neuros, sourcename, targetname, metadata)

    if config.sort_database:
        print "\nSorting tracks..."
        audio_db.sort()
        
    print "Done!"
    neuros.close_db("audio")
    
