# -*- Mode: python -*-
#
# util.py - utility functions
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

import os
from os import path

def trim_newline(s):
    "Returns s without a trailing newline if present."
    if len(s) > 1:
        if s[-1] == "\n":
            return s[:-1]
        else:
            return s
    else:
        return s

def copy_file (src_filename, dest_filename):
    """Copy a file from src_filename to dest_filename

    Directories are created as needed to accomodate dest_filename"""
    blocksize = 1048576
    src = file(src_filename, "rb")

    (dirname, basename) = os.path.split(dest_filename)
    if not os.path.exists(dirname):
        os.makedirs(dirname)  # Does not work with UNC paths on Win32
        
    dest = file(dest_filename, "wb")

    block = src.read(blocksize)
    while block != "":
        dest.write(block)
        block = src.read(blocksize)

    src.close()
    dest.close()

def recursive_delete(pathname):
    if path.isfile(pathname):
        os.remove(pathname)
    elif path.isdir(pathname):
        for item in os.listdir(pathname):
            recursive_delete(path.join(pathname, item))

        os.rmdir(pathname)
    else:
        raise OSError("Non-file or directory encountered.")
    
def cmp_records(a,b):
    if a == None and b == None:
        return 0
    elif a == None:
        return -1
    elif b == None:
        return 1
    else:
        return cmp(a[0].lower(), b[0].lower())
