# -*- Mode: python -*-
#
# neuros.py - neuros interface
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

import db
import db.new
import os
from os import path
import util
import ports

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

def _total_path_split(pathname):
    path_parts = []
    (dirname, basename) = path.split(pathname)
    while dirname != "" and dirname != "/":
        if basename != "":
            path_parts.insert(0, basename)
        (dirname, basename) = path.split(dirname)

    if basename != "":
        path_parts.insert(0, basename)
    if dirname != "":
        path_parts.insert(0, dirname)

    return path_parts

def FAT_mangle_filename(filename):
    """Creates a Neuros and FAT-friendly filename.

    Substitute replacement characters (usually '_') for characters in
    filename that are not allowed on FAT filesystems.  Double quotes are
    replaced with single quotes.  Also replaces characters that cause
    problems with the Neuros firmware (like accented characters)."""

    # Keys are bad characters for FAT filesystems, and values are the
    # replacement
    mangle_table = { '\\' : '_',
                     ':' : '_',
                     '*' : '_',
                     '?' : '_',
                     '"' : "'",
                     '<' : '_',
                     '>' : '_',
                     '|' : '_' }
    
    new_filename = ""
    mangle_table_keys = mangle_table.keys()
    for old_char in filename:
        if ord(old_char) >= 0x7F:
            new_char = "_"
        elif old_char in mangle_table_keys:
            new_char = mangle_table[old_char]
        else:
            new_char = old_char

        new_filename += new_char

    return new_filename

class Neuros:

    DB_DIR = path.normcase("WOID_DB")

    # Constants for describing the type of Neuros
    TYPE_UNKNOWN = 0
    TYPE_FLASH   = 1
    TYPE_HD      = 2


    db_formats = {
        "audio"      : { "no_flatten"   : (1, ),
                         "extra_format" : (">I",">I","z"),
                         "extra_names"  : ("Time", "Size", "Path") },

        "pcaudio"    : { "no_flatten"   : (1, ),
                         "extra_format" : (">I",">I","z"),
                         "extra_names"  : ("Time", "Size", "Path") },
        
        "unidedhisi" : { "no_flatten"   : (),
                         "extra_format" : ("z","z"),
                         "extra_names"  : ("Source", "Path") },

        "idedhisi"   : { "no_flatten"   : (),
                         "extra_format" : ("z","z","z","z","z",
                                           ">I",">I","z"),
                         "extra_names"  : ("Source", "Artist",
                                           "Album", "Genre", "Track Name",
                                           "Time", "Size", "Path") },
        
        "failedhisi" : { "no_flatten"   : (),
                         "extra_format" : ("z","z"),
                         "extra_names"  : ("Source", "Path") }
        }


    def __init__(self, mountpoint):
        self.mountpoint = path.abspath(mountpoint)

        # Check and see if the mountpoint looks legit
        dbpath = path.join(self.mountpoint, Neuros.DB_DIR)
        try:
            os.listdir(dbpath)
        except OSError:
            raise Error("%s does not look like a Neuros mountpoint"
                        % (mountpoint,))

        # Handy to keep around
        self.mountpoint_parts = _total_path_split(self.mountpoint)

        self.db = {}
        for db_name in self.db_formats.keys():
            self.db[db_name] = None

        # Figure out what kind of Neuros this is
        (self.neuros_type, self.neuros_size) = self._detect_neuros_type(self.mountpoint)


    def _detect_neuros_type(self, mountpoint):

        # There is no direct way to determine the type of Neuros
        # (either flash or HD) so we have to infer it from the size of
        # the volumes connected to the system.
        
        size = ports.filesystem_size(mountpoint)

        if size == None:
            return (Neuros.TYPE_UNKNOWN, -1)
        elif size < 2147483648:  # Use 2 GB as an arbitrary size cutoff
            return (Neuros.TYPE_FLASH, size)
        else:
            return (Neuros.TYPE_HD, size)


    def open_db(self, name):

        if name not in self.db.keys():
            raise Error("Database %s is not a valid Neuros database"
                        % (name, ))

        rootpath = path.join(self.mountpoint, Neuros.DB_DIR, name, name)

        self.db[name] = db.WOID()
        self.db[name].open(rootpath, self.db_formats[name]["extra_format"],
                           self.db_formats[name]["no_flatten"])
        return self.db[name]

    def close_db(self, name):
        if name not in self.db.keys():
            raise Error("Database %s is not a valid Neuros database"
                        % (name, ))

        if self.db[name] == None:
            raise Error("Database %s is not open" % (name, ))

        self.db[name].close()
        self.db[name] = None

    def new_db(self, name):
        # Close database before we trash it
        if self.db[name] != None:
            self.close_db(name)
            reopen = True
        else:
            reopen = False

        # Erase database dir if present
        dbpath = path.join(self.mountpoint, Neuros.DB_DIR, name)
        
        if path.exists(dbpath):
            util.recursive_delete(dbpath)

        # Write new database
        db.new.unpack(name, path.join(self.mountpoint, Neuros.DB_DIR))
        
        if reopen:
            self.open_db(name)

    def get_serial_number(self):

        serial_filename = path.join(self.mountpoint, "sn.txt")
        
        f = file(serial_filename, "r")
        serial = f.readline()

        return serial

    def is_valid_hostpath(self, hostpath):
        """Checks if a given path on the host is a subdirectory (or the root)
        of the Neuros mountpoint."""
        
        hostpath = path.abspath(hostpath)
        hostpath_parts = _total_path_split(hostpath)

        return self.mountpoint_parts == \
               hostpath_parts[:len(self.mountpoint_parts)]

    def hostpath_to_neurospath(self, hostpath, neuros_type = None):
        if not self.is_valid_hostpath(hostpath):
            raise Error("Host path not under Neuros mountpoint")

        # Get the path parts, but prune off the mountpoint
        hostpath_parts = _total_path_split(hostpath)[len(self.mountpoint_parts):]

        if neuros_type == None:
            neuros_type = self.neuros_type

        if neuros_type == Neuros.TYPE_HD:
            drive_letter = "C:"
        elif neuros_type == Neuros.TYPE_FLASH:
            drive_letter = "D:"
        else:
            # Assume HD
            drive_letter = "C:"
        
        hostpath_parts.insert(0, drive_letter)
        # Now join it all with forward slashes to get the Neuros path
        return "/".join(hostpath_parts)

    def neurospath_to_hostpath(self, neurospath):
        neurospath_parts = neurospath.split("/")

        if len(neurospath_parts) < 1 \
               or neurospath_parts[0].lower() not in ("c:", "d:"):
            raise Error("Neuros path does not start with C: or D:")

        hostpath_parts = self.mountpoint_parts + neurospath_parts[1:]

        return path.join(*hostpath_parts)

    def mangle_hostpath(self, hostpath):
        if not self.is_valid_hostpath(hostpath):
            raise Error("Host path not under Neuros mountpoint")

        # Only mangle the parts after the mountpoint
        hostpath_parts = _total_path_split(hostpath)[len(self.mountpoint_parts):]
        hostpath_parts = self.mountpoint_parts + \
                         map(FAT_mangle_filename, hostpath_parts)

        return path.join(*hostpath_parts)
