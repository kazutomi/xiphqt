# -*- Mode: python -*-
#
# db/PAI.py - PAI functions
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
from util import *

class PAI:

    SIGNATURE = 0x01162002
    
    # These are in WORDS!
    MIN_PAI_MODULE_LEN = 32
    FILE_HEADER_LEN = 8
    MODULE_HEADER_LEN = 6
    MODULE_FOOTER_LEN = 2
    
    def __init__(self):
        self.filename = None
        self.file = None
        
    def open(self, filename):
        "Open the PAI file and parse the header"
        self.file = file(filename, "r+b")
        self.filename = filename

        pattern = ">I12x"
        size_required = struct.calcsize(pattern)
        header = struct.unpack(pattern, self.file.read(size_required))

        if header[0] != PAI.SIGNATURE:
            raise Error("Invalid PAI signature")

    def get_module_pointers(self):
        f = self.file
        pointers = []

        pointer = PAI.FILE_HEADER_LEN

        f.seek(to_offset(pointer))
        length_str = f.read(2)
        while length_str != "":
            pointers.append(pointer + PAI.MODULE_HEADER_LEN)
            
            (length,) = struct.unpack(">H", length_str)
            pointer += length

            f.seek(to_offset(pointer), 1)
            length_str = f.read(2)

        return pointers

    def read_module_header_at(self, pointer):
        """Returns (length, flags, num_entries) for the module referenced by
        pointer."""
        f = self.file
        pointer -= PAI.MODULE_HEADER_LEN

        return self._read_module_header(pointer)

    def _read_module_header(self, pointer):
        f = self.file

        f.seek(to_offset(pointer))

        pattern = ">HHH6x"
        size_required = struct.calcsize(pattern)
        return struct.unpack(pattern, f.read(size_required))

    def read_module_at(self, pointer):
        f = self.file
        pointer -= PAI.MODULE_HEADER_LEN

        (length, flag, num_entries) = self._read_module_header(pointer)

        if flag & 0x0001 == 0x0001:
            return ([], pointer+length)

        entries = []
        for i in range(num_entries):
            entry = struct.unpack(">I", f.read(4))
            entries.append(entry)

        return (entries, pointer+length)

    def add_entry_to_module_at(self, pointer, entry):
        f = self.file
        pointer -= PAI.MODULE_HEADER_LEN

        # Calculate empty space
        (length, flag, num_entries) = self._read_module_header(pointer)

        # Total length of module is 6 word header, 2 word ending zeros,
        # num_entries * 32 bit word pointers, and empty space
        if (length - PAI.MODULE_HEADER_LEN - PAI.MODULE_FOOTER_LEN
            - num_entries * 2) == 0:
            # Argh, stupid difference between SAI pointer (which we
            # use in the calling convention in this module) and
            # the pointer to the start of the module
            extension_offset = self.extend_module_at(pointer
                                                     +PAI.MODULE_HEADER_LEN, 1)

            # Reread header
            (length, flag, num_entries) = self._read_module_header(pointer)
        else:
            extension_offset = 0


        # Update part of header
        flag &= 0xFFFE
        num_entries += 1
        header = struct.pack(">HH", flag, num_entries)
        f.seek(to_offset(pointer+1)) # Skipping length value
        f.write(header)

        # Add entry   
        entry_pointer = pointer + PAI.MODULE_HEADER_LEN + (num_entries - 1) * 2
        packed_entry = struct.pack(">I", entry)
        
        f.seek(to_offset(entry_pointer))
        f.write(packed_entry)
        f.flush()
        
        return extension_offset
        
    def delete_entry_in_module_at(self, pointer, entry):
        """Searches through module and erases the give entry.

        Note that entry is the value, not the index number or some
        other identifier.  A linear search for entry is performed and
        the first match encountered is erased, if any.

        Returns True if entry located and deleted, False if entry not
        present"""
        
        f = self.file
        pointer -= PAI.MODULE_HEADER_LEN

        (length, flag, num_entries) = self._read_module_header(pointer)

        # Find the word pointer to word *after* entry.  If entry
        # is last then we will be pointing at the terminating null
        # longword
        f.seek(to_offset(pointer + PAI.MODULE_HEADER_LEN))
        (curr_entry,) = struct.unpack(">I", f.read(4))
        entry_num = 0
        while entry_num < num_entries:
            if curr_entry == entry:
                break
            (curr_entry,) = struct.unpack(">I", f.read(4))
            entry_num += 1
        else:
            # Entry not found.
            return False

        # Read everything from here to the end of the module (including footer)
        next_entry_pointer = to_pointer(f.tell())
        read_len = length - (next_entry_pointer - pointer)
        module_remainder = f.read(to_offset(read_len))

        # Now go to the entry to delete and write over it.  The
        # terminating null footer will ensure that the extra space is
        # null padded correctly.
        f.seek(to_offset(next_entry_pointer - 2))
        f.write(module_remainder)

        # And fix the number of entries
        num_entries -= 1
        if num_entries == 0:
            flag &= 0x0001
        
        f.seek(to_offset(pointer+1))
        f.write(struct.pack(">HH", flag, num_entries))
        f.flush()

        return True

    def append_module(self, entries):
        f = self.file

        # "Empty" PAI files always have one empty module in them.
        # This module is not associated with the mandatory null entry
        # in the MDB, so we must "overwrite it" when adding a module
        # for to an empty PAI.  In actuality, we just find it and
        # return a pointer to it.
##         modules = self.get_module_pointers()
##         if len(modules) == 1 and len(self.read_module_at(modules[0])[0]) == 0:

##             for entry in entries:
##                 self.add_entry_to_module_at(modules[0], entry)

##             return modules[0]

        #Compute header values
        num_entries = len(entries)
        if num_entries == 0:
            flags = 0x0001
        else:
            flags = 0x0000
        length = PAI.MODULE_HEADER_LEN + num_entries*2 + PAI.MODULE_FOOTER_LEN
        # Round up to multiple of MIN_PAI_MODULE_LEN
        length += PAI.MIN_PAI_MODULE_LEN - (length % PAI.MIN_PAI_MODULE_LEN)
        
        module = struct.pack(">HHH", length, flags, num_entries)
        module += "\x00\x00" * 3

        for entry in entries:
            module += struct.pack(">I", entry)

        # Null pad the rest
        module += "\x00\x00" * (length - PAI.MODULE_HEADER_LEN - 2*num_entries)

        f.seek(0,2)
        position = to_pointer(f.tell())
        f.write(module)
        f.flush()

        return position + PAI.MODULE_HEADER_LEN


    def extend_module_at(self, pointer, chunks=1):
        f = self.file
        pointer -= PAI.MODULE_HEADER_LEN
        
        (length, flags, num_entries) = self._read_module_header(pointer)

        # Read everything after this module
        f.seek(to_offset(pointer+length))
        rest = f.read()

        # Extend this module with nulls
        f.seek(to_offset(pointer+length))
        f.write("\x00\x00"*chunks*PAI.MIN_PAI_MODULE_LEN)
        f.write(rest)

        # Fix up header
        f.seek(to_offset(pointer))
        length = length + chunks * PAI.MIN_PAI_MODULE_LEN
        length_str = struct.pack(">H", length)
        f.write(length_str)
        f.flush()

        return chunks * PAI.MIN_PAI_MODULE_LEN

    def clear_module_at(self, pointer):
        f = self.file
        module_start = pointer - PAI.MODULE_HEADER_LEN

        (length, flag, num_entries) = self._read_module_header(module_start)

        flag |= 0x0001
        num_entries = 0

        new_module = struct.pack(">HHH", length, flag, num_entries)

        new_module += "\x00\x00" * (length - 3)

        f.seek(to_offset(module_start),0)
        f.write(new_module)
        f.flush()

    def set_empty_module_at(self, pointer, value=True):
        f = self.file
        pointer -= PAI.MODULE_HEADER_LEN

        f.seek(to_offset(pointer+1))
        flags = struct.unpack(">H", f.read(2))

        if value:
            flags |= 0x0001
        else:
            flags &= 0xFFFE

        new_flags = struct.pack(">H", flags)
        f.seek(to_offset(pointer+1))
        f.write(new_flags)
        f.flush()        

    def clear(self):
        f = self.file
        f.seek(to_offset(PAI.FILE_HEADER_LEN))
        f.truncate()

        # Always have to have one dummy module in it
        position = self.append_module([])
        
        f.flush()
        return position
    
    def close(self):
        self.file.close()
        self.__init__()

