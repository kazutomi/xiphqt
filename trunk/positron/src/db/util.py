# -*- Mode: python -*-
#
# db/util.py - db utility functions
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

import struct
from types import *

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

class Special:
    # Special characters

    # '#' - Marks the end of a file within a record
    FIELD_DELIM = "\x00\x23"

    # '%' - Marks the end of the current record
    END_OF_RECORD = "\x00\x25"

    # '$' - Delimiter between multiple values in a single field
    BAG_DELIM = "\x00\x24"

    # '/' Used to represent special words within strings, much as in C:
    #     # -> /#, % -> /%, $ -> /$, and / -> //
    ESCAPER = "\x00\x2F"


def to_offset(pointer):
    "Converts a \"pointer\" as used in the WOID DB formats to a byte offset."
    return pointer*2

def to_pointer(offset):
    """Converts a byte offset to a WOID DB pointer.

    If the byte offset is not an even number, Error is raised since pointers must
    point at 16-bit aligned data."""
    if offset % 1 == 1:
        raise Error("offset not 16-bit aligned")
    else:
        return offset//2

def read_display_data(str):
    "Reads a string of display data prefixed by the string size in words."
    (length,) = struct.unpack(">H", str[:2])
    length = to_offset(length)
    
    return struct.unpack(">%ds" % (length,), str[2:2+length])[0]

def read_stringz(str):
    """Reads a null terminated string from str.

    If the string is not null terminated, Error is raised."""
    term = str.find("\0")
    if term == -1:
        raise Error("Conversion error: String not null terminated.")

    return str[:term]

def fread_word(f):
    "Reads a 16 bit big endian word from file object f."
    return struct.unpack(">H", f.read(2))[0]

def fwrite_word(f, word):
    "Writes an integer as a 16 bit big endian word to file object f."
    f.write(struct.pack(">H", word))

def trim_string(str):
    "Returns copy of str with trailing null bytes removed"

    substr_len = len(str)
    while substr_len > 0 and str[substr_len-1] == "\x00":
        substr_len -= 1

    return str[:substr_len]

def term_string(str):
    """Returns str terminated with null word and padded to word boundary"""
    # Need to word align things and add a null-termination word
    if len(str) % 2 == 0:
        return str + "\x00\x00"
    else:
        return str + "\x00\x00\x00"

def collapse_null_list(obj):
    """Converts [None] to []"""

    if type(obj) is ListType or type(obj) is TupleType:
        if len(obj) == 1 and obj[0] == None:
            return []
        else:
            return obj
    else:
        return obj

def uncollapse_null_list(obj):
    """Converts [] to [None], leaves other objects unchanged"""
    if type(obj) is ListType or type(obj) is TupleType:
        if len(obj) == 0:
            return [None]
        else:
            return obj
    else:
        return obj
    

def flatten_singlet(obj):
    """Extracts element from single object lists, otherwise returns obj unchanged

    This is the inverse operation to unflatten_singlet()."""

    if type(obj) is ListType or type(obj) is TupleType:
        if len(obj) == 1:
            return obj[0]
        else:
            return obj
    else:
        return obj

def unflatten_singlet(obj):
    """If obj is a list, returned unchanged.  Otherwise obj put in single element list.

    This is the inverse operation to flatten_singlet()."""

    if type(obj) is ListType or type(obj) is TupleType:
        return obj
    else:
        return [obj]
