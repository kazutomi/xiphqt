# -*- Mode: python -*-
#
# cmd_del.py - del command
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

"""positron del:\tRemoves files from the database and disk

  positron del [files or directories]

     [files or directories] - A list of files and/or directories on
     the Neuros itself (ex: /mnt/neuros/music/rock) to be removed.
     Both the file on disk and the database entries are deleted.

Note that only files referenced in the database will be removed,
others will not be affected.  Empty directories are also removed.
"""

import os
from os import path
from neuros import Neuros
import db
import util


def gen_filelist(neuros, pathname):
    filelist = []
    fullname = path.abspath(pathname)
    empty = True

    if not neuros.is_valid_hostpath(fullname):
        print "Warning: Ignoring %s because it is not a path on the Neuros." \
              % (pathname, )
    elif path.isfile(pathname):

        neuros_path = neuros.hostpath_to_neurospath(fullname)

        # Search for this pathname in the last field
        sai_index = neuros.db["audio"].find(neuros_path, 8)
        if sai_index == None:
            empty = False
        else:
            filelist.append((fullname, sai_index))
                    
    elif path.isdir(pathname):
        subs = os.listdir(pathname)

        for sub in subs:
            (sub_list, sub_empty) = gen_filelist(neuros,
                                                 path.join(pathname, sub))
            filelist.extend(sub_list)
            empty = empty and sub_empty

        # If all the contents of this directory are (or are going to
        # be empty), then this directory can be deleted
        if empty:
            filelist.append((fullname, None))
    else:
        print "Ignoring %s.  Not a file or directory." % (pathname)

    return (filelist, empty)


def del_track(config, neuros, sourcename, sai_index):
    try:
        os.remove(sourcename)
        neuros_path = neuros.db["audio"].get_record(sai_index)[8].lower()
        
        # Remove entry from database
        neuros.db["audio"].delete_record(sai_index)
        config.add_deleted(neuros_path)
        
    except os.error, e:
        print "Error:", e
    except db.util.Error, e:
        print "Error:", e

def run(config, neuros, args):
    audio_db = neuros.open_db("audio")

    filelist = []

    if len(args) == 0:
        print __doc__
        return

    for arg in args:
        (new_list, empty) = gen_filelist(neuros, arg)
        filelist.extend(new_list)
            
    if len(filelist) == 0:
        print "No files to remove are present in the database!"
    elif len(filelist) == 1:
        (sourcename, sai_index) = filelist[0]
        basename = path.basename(sourcename)
        print "Removing %s from the Neuros..." % (basename,),
        del_track(config, neuros, sourcename, sai_index)
        print "  Done!"
    else:
        tracks = len([item for item in filelist if item[1] != None])
        print "Removing %d tracks from the Neuros..." % (tracks,)
        for (sourcename, sai_index) in filelist:
            basename = path.basename(sourcename)
            if sai_index != None:
                print "  %s..." % (basename, )
                del_track(config, neuros, sourcename, sai_index)
            else:
                print "  Removing empty directory %s..." % (sourcename, )
                try:
                    os.rmdir(sourcename)
                except os.error, e:
                    print "Error:", e

        print "\nDone!"
    neuros.close_db("audio")
    
