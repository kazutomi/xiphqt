# -*- Mode: python -*-
#
# add_file.py - utilities for file addition
#
# Copyright (C) 2003, Xiph.org Foundation
# Copyright (C) 2003 Brett Smith <bretts@canonical.org>
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

"Utility module for adding files to the Neuros.  Used by cmd_sync and cmd_add."

import sys
import os
from os import path
import audiofile
from neuros import Neuros
import util

def gen_filelist(neuros, prefix, suffix, target_prefix,
                 allowed_types, silent=False, status=True):
    filelist = []
    fullname = path.join(prefix, suffix)
    clearline =  "\r"+" "*70+"\r"
    
    if path.isdir(fullname):
        files = [path.join(suffix,name) for name in os.listdir(fullname)]
    else:
        files = [suffix]

    for name in files:
        fullname = path.join(prefix, name)
        
        if path.isfile(fullname):
            metadata = audiofile.detect(fullname)
            if metadata == None or metadata["type"] not in allowed_types:
                if not silent:
                    print "Skipping %s.  Not a supported audio format." \
                          % (fullname,)
            elif neuros.is_valid_hostpath(fullname):
                # Don't need to copy files already on the Neuros
                filelist.append((None, fullname, metadata))
            else:
                targetname = neuros.mangle_hostpath(path.join(target_prefix,
                                                              name))
                if path.exists(targetname):
                    if not silent:
                        print "Skipping %s because %s already exists." \
                              % (fullname, targetname)
                else:
                    filelist.append((fullname, targetname, metadata))
                    
        elif path.isdir(fullname):
            if status:
                sys.stderr.write(clearline)
                status = "Scanning %s" % (fullname,)
                if len(status) > 70:
                    status = status[:67] + "..." 
                sys.stderr.write(status+"\r")
                    
            filelist.extend(gen_filelist(neuros, prefix, name, target_prefix,
                                         allowed_types, silent))
        else:
            if not silent:
                print "Ignoring %s.  Not a file or directory." % (fullname)

    return filelist


def add_track(neuros, sourcename, targetname, metadata, recording=None):
    if sourcename != None:
        util.copy_file(sourcename, targetname)

    # Create DB entry
    destination = neuros.hostpath_to_neurospath(targetname)
    record = (metadata["title"], None, metadata["artist"], metadata["album"],
              metadata["genre"], recording, metadata["length"],
              metadata["size"] // 1024, destination)
    # Add entry to database
    neuros.db["audio"].add_record(record)
    
    try:
        filename = path.join(*neuros.mountpoint_parts +
                             [neuros.DB_DIR, 'tracks.txt'])
        tracknum_file = file(filename, 'a')
        tracknum = metadata.get('tracknumber', None)
        if tracknum is None:
            tracknum = metadata.get('title', targetname)
            try:
                tracknum.lower()
            except AttributeError:
                pass
        tracknum_file.write('%s\t%s\n' % (destination, tracknum))
        tracknum_file.close()
    except IOError:
        pass  # Fail silently if we can't write the file.
