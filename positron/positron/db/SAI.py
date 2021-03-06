# -*- Mode: python -*-
#
# db/SAI.py - SAI functions
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
import types
from util import *

def cmp_sai_record(mdb, sai_record_a, sai_record_b):
    (a,dummy) = mdb.read_record_at(sai_record_a[0])
    (b,dummy) = mdb.read_record_at(sai_record_b[0])
    
    if a == None and b == None:
        return 0
    elif a == None:
        return -1
    elif b == None:
        return 1
    else:
        return cmp(a["data"][0].lower(), b["data"][0].lower())

class SAI:
    """Read and write a SAI file as if it were a list.

    This class implements all of the list methods, so it can be used
    just the same as a list.  However, it is backed by a file on disk
    which is synchronously updated, so operations will be slower."""
    
    SIGNATURE = 0x5181971
    DATA_START = 24
    ENTRY_FORMAT = ">II"
    ENTRY_SIZE = 8
    
    def __init__(self):
        self.file = None
        self.filename = None
        self.num_entries = None
         
    def open(self, filename):
        "Open the SAI file and parse the header"
        self.file = file(filename, "r+b")
        self.filename = filename

        pattern = ">I4xH6xQ"
        size_required = struct.calcsize(pattern)
        header = struct.unpack(pattern, self.file.read(size_required))

        if header[0] != SAI.SIGNATURE:
            raise Error("Invalid SAI signature")
        self.num_entries = header[1]

        if header[2] != 0:
            raise Error("Initial zero entry missing")

    def __len__(self):
        return self.num_entries

    def __getitem__(self, key):
        if key is types.SliceType:
            return None
        else:
            if key < 0:
                # Interpret negative indices in Python-like manner
                key = key + self.num_entries
                
            if key < 0 or key >= self.num_entries:
                raise IndexError("Key out of bounds")
            
            self.file.seek(SAI.DATA_START + 8*key)
            return list(struct.unpack(SAI.ENTRY_FORMAT,
                                      self.file.read(SAI.ENTRY_SIZE)))
            
    def __setitem__(self, key, value):
        if key is types.SliceType:
            return None
        else:
            if key < 0:
                # Interpret negative indices in Python-like manner
                key = key + self.num_entries
                
            if key < 0 or key >= self.num_entries:
                raise IndexError("Key out of bounds")

            entry = self._pack_entry(value)
            
            self.file.seek(SAI.DATA_START + 8*key)
            self.file.write(entry)


    def _pack_entry(self, value):
        if len(value) > 2:
            raise ValueError("Incorrect SAI entry format")
        else:
            try:
                entry = struct.pack(SAI.ENTRY_FORMAT, value[0], value[1])
            except struct.error, e:
                raise ValueError("Incorrect entry format: "+e.value)

            return entry

    def find(self, key, field=0):
        for i in range(self.num_entries):
            if self[i][field] == key:
                return i
        else:
            return None
        
    def append(self, value):
        entry = self._pack_entry(value) + "\x00"*8

        self.file.seek(SAI.DATA_START + 8*self.num_entries)
        self.file.write(entry)

        # Update header
        self.num_entries += 1
        self.file.seek(8)
        self.file.write(struct.pack(">H", self.num_entries))

    def delete(self, index):
        self.file.seek(SAI.DATA_START + 8*(index+1))
        rest = self.file.read()
        self.file.seek(SAI.DATA_START + 8*index)        
        self.file.write(rest)

        # Update header
        self.num_entries -= 1
        self.file.seek(8)
        self.file.write(struct.pack(">H", self.num_entries))

    def shift_pai(self, threshold, offset):
        """Locates all entries with PAI pointer greater than threshold and shifts them by offset.

        Use this function if a PAI module is extended, thus invalidating all
        of the PAI module pointers in a database (though in a predictable way).
        Let threshold be the pointer to the module that was extended, and
        offset be the length of the extension in words."""

        for i in range(len(self)):
            # Gotta do things this way because we can't assign to individual
            # elements of each SAI record.  Gotta write the whole things as
            # a block
            record = self[i]
            if self[i][1] > threshold:
                record[1] += offset
                self[i] = record

    def sort(self, cmpfunc):
        """Sort the SAI entries based upon cmpfunc (as used in list.sort())."""

        # Note we gotta do this the slighly long way because we're not
        # exactly a python list.
        mylist = [self[i] for i in range(len(self))]
        
        mylist.sort(cmpfunc)

        for i in range(len(self)):
            self[i] = mylist[i]

    def clear(self):
        self.file.truncate(SAI.DATA_START+8)
        self.file.seek(8)
        self.file.write("\x00"*24)
        self.num_entries = 0
    

    def close(self):
        self.file.close()
        self.__init__()
