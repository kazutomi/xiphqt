# -*- Mode: python -*-
#
# db/MDB.py - MDB database functions
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

def _unpack_fields(f):
    """Parses a record in file object f into fields.

    16-bit words are read starting at the current position of f and
    escape codes are handled, as well as bag and field delimiters.

    Returns a list with one element for each field.  An element will be
    a list of strings of bytes (since fields can have multiple values)."""

    fields = []
    current_bag = []
    current_item = ""
    escape_mode = False

    word = f.read(2)
    while word != "":
        if escape_mode:
            current_item += word
            escape_mode = False
        else:
            if word == Special.ESCAPER:
                escape_mode = True
            elif word == Special.FIELD_DELIM or word == Special.END_OF_RECORD:
                current_bag.append(current_item)
                fields.append(current_bag)
                current_bag = []
                current_item = ""

                if word == Special.END_OF_RECORD:
                    break
            else:
                current_item += word

        word = f.read(2)
    else:
        raise Error("Premature EOF while reading record.")

    # Special handled the empty record for easy ID with len(record) == 0:
    if len(fields) == 1 and fields[0] == [""]:
        fields = []

    return fields

def _escape_string(str):
    """Scans str for special characters and escapes them"""

    escape_mode = False
    new = ""

    for i in range(0, len(str), 2):
        word = str[i:i+2]
        if word in (Special.FIELD_DELIM, Special.END_OF_RECORD,
                   Special.BAG_DELIM, Special.ESCAPER):
            new += Special.ESCAPER
        new += word

    return new

def _pack_field(field):
    """Packs a field (list of values) with bag delimiters as needed"""
    str = _escape_string(field[0])
    for item in field[1:]:
        str +=  Special.BAG_DELIM + _escape_string(item)

    return str

def _pack_fields(fields):
    """Packs a list of fields using the MDB format.

    fields should have the same structure as returned by _unpack_fields().

    Returns a string with the packed data."""

    str = _pack_field(fields[0])

    for field in fields[1:]:
        str += Special.FIELD_DELIM + _pack_field(field)

    str += Special.END_OF_RECORD
    return str

class MDB:

    def __init__(self):
        self.header = None
        self.ximdata = None
        self.filename = None
        self.file = None
        self.extra_format = ()

        
    def open(self, filename):
        "Open the MDB file and parse the header"
        self.file = file(filename, "r+b")
        self.filename = filename

        self.header = self._read_header(self.file)
        self.file.seek(to_offset(self.header["pXIM"]))
        self.ximdata = self._read_ximdata(self.file)

    def set_extra_format(self, extra_format):
        """Defines the format of the extra info fields.

        extra_format is a tuple of strings that define the format of
        the binary data found in each extra info field in the
        database.  Each element should be a format string as defined in
        the struct module, or the format string \"z\", which means the
        field is a null-terminated string with possible escape
        characters.  Note that len(extra_format) ==
        header[\"NumOfFieldsPerRecord\"] - header[\"NumOfKeys\"]."""

        # Sanity check
        if  len(extra_format) != (self.header["NumOfFieldsPerRecord"]
                                  - self.header["NumOfKeys"]):
            raise Error("Number of format strings does not equal number of extra info fields.")

        self.extra_format = extra_format

    def find_next_item(self, pointer):
        """Finds the word pointer to the record in the database after the given pointer.

        Returns -1 if no additional record can be found, otherwise returns the word
        pointer (NOT the byte offset), which can then be passed to read_item_at().
        This is obviously a slow search, so if possible, the SAI object should be
        used to find where records start."""
        f = self.file

        f.seek(to_offset(pointer))

        # Look for record delimeter '%' (0x0025) not immediately following escape
        # word '/' (x002f)
        escape_mode = false
        word = f.read(2)
        while word != "":
            pointer += 1
            
            if escape_mode:
                escape_mode = false
            else:
                if word == self.ESCAPER:
                    escape_mode = true
                elif word == self.END_OF_RECORD:
                    break

        else:
            # Hit eof before END_OF_RECORD
            return -1

        return pointer


    def read_record_at(self, pointer):
        """Returns a tuple with the record at pointer and a pointer to the next record.

        The first element in the tuple is a dict with the record
        contents.  The second element is the pointer to the next
        record (so you can efficiently iterate through the file).
        Note that pointer is a word pointer (as stored in the DB) and
        NOT a byte offset.  If the record is the special empty record,
        None is returned for the record contents.  If there are no
        more records, the pointer to the next record will be None."""
        
        f = self.file

        f.seek(to_offset(pointer))

        record = {}

        record["pointer"] = pointer
        record["flags"] = fread_word(f)

        # Check ID bit
        if record["flags"] & 0x8000 == 0:
            raise Error("Invalid record: ID bit not set")

        if record["flags"] & 0x01:
            record["isDeleted"] = True
        else:
            record["isDeleted"] = False

        fields = _unpack_fields(f)

        # Special empty record
        if len(fields) == 0:
            return (None, to_pointer(f.tell()))

        # Read primary field
        record["data"] = map(trim_string, fields[0])

        # Read access keys
        record["keys"] = []
        for i in range(1, self.header["NumOfKeys"]):
            values = [struct.unpack(">I", item)[0]
                      for item in fields[i]]
            record["keys"].append(values)

        # Read extra fields
        record["extra"] = []
        i = self.header["NumOfKeys"]
        for fmt in self.extra_format:
            values = []
            for item in fields[i]:
                if fmt == "z":
                    value = trim_string(item)
                else:
                    (value, ) = struct.unpack(fmt, item)

                values.append(value)

            record["extra"].append(values)

            i += 1

        if i != self.header["NumOfFieldsPerRecord"]:
            raise Error("Incorrect number of fields in record.")

        # Check if this is the last record
        current_offset = f.tell()
        f.seek(0,2)
        if current_offset != f.tell():
            return (record, to_pointer(current_offset))
        else:
            return (record, None)

    def is_record_deleted_at(self, pointer):
        f = self.file

        f.seek(to_offset(pointer))

        flags = fread_word(f)

        # Check ID bit
        if flags & 0x8000 == 0:
            raise Error("Invalid record: ID bit not set")

        return flags & 0x01

    def undelete_record_at(self, pointer):
        self._set_delete_flag(pointer, 0)

    def delete_record_at(self, pointer):
        self._set_delete_flag(pointer, 1)

    def _set_delete_flag(self, pointer,delete_flag):
        f = self.file
        
        f.seek(to_offset(pointer))

        flags = fread_word(f)

        # Check ID bit
        if flags & 0x8000 == 0:
            raise Error("Invalid record: ID bit not set")
        # Modify delete flag
        flags = (0xFFFE & flags) | (0x0001 & delete_flag)

        f.seek(-2, 1)
        fwrite_word(f, flags)
        f.flush()

    def count_deleted(self):
        "Returns the number of deleted records in this database."

        f = self.file
        f.seek(0,2)
        size = f.tell()

        if to_pointer(size) > self.header["RecordStart"]:
            curr_ptr = self.header["RecordStart"]
        else:
            curr_ptr = None

        count = 0
        while curr_ptr != None:
            (curr_record, curr_ptr) = self.read_record_at(curr_ptr)
            if curr_record != None and curr_record["isDeleted"]:
                count += 1

        return count

    def append_record(self, record):
        """Writes a new record to the end of the file.

        The record should be formatted in the same way as the return value from
        read_record_at()."""

        # Deal with degenerate null record case
        if record == None:
            new = "\x80\x00\x00\x25"
        else:
            # Record flag
            flags = 0x8000
            if record["isDeleted"]:
                flags |= 0x0001

            new = struct.pack(">H", flags)

            fields = []
            # Primary Record Data
            fields.append(map(term_string, record["data"]))

            # Access Keys
            if len(record["keys"]) != self.header["NumOfKeys"]-1:
                raise Error("Incorrect number of access keys in record")

            for key in record["keys"]:
                values = [struct.pack(">I", item) for item in key]
                fields.append(values)

            # Extra Info Fields
            if len(record["extra"]) != (self.header["NumOfFieldsPerRecord"]
                                        - self.header["NumOfKeys"]):
                raise Error("Incorrect number of extra info fields in record")

            for i in range(len(record["extra"])):
                values = []
                for item in record["extra"][i]:
                    if self.extra_format[i] == "z":
                        values.append(term_string(item))
                    else:
                        values.append(struct.pack(self.extra_format[i], item))

                fields.append(values)


            # Pack record
            new += _pack_fields(fields)

        # Write to disk
        self.file.seek(0,2)
        position = to_pointer(self.file.tell())
        self.file.write(new)
        self.file.flush()

        return position

    def _read_header(self, f):
        "Parse the MDB header data and return a dict with the relevant info."

        # Figure out how much to read
        (length,) = struct.unpack(">H", f.read(2))
        length = to_offset(length)

        f.seek(-2, 1)
        header_data = f.read(length)

        # Verify signature
        if header_data[-4:] != "WOID":
            raise Error("Corrupted header")

        index = 0
        header = {}
        
        pattern = ">HHHHHII12sH"
        size_required = struct.calcsize(pattern)
        (header["LengthOfHeader"], header["Attributes"], header["Status"],
         header["NumOfKeys"], header["NumOfFieldsPerRecord"],
         header["RecordStart"], header["pXIM"], header["Reserved"],
         header["DB_ID"]) \
         = struct.unpack(pattern, header_data[index:index+size_required])
        index += size_required
        
        header["isRoot"] = (header["Attributes"] & 0x01) == 0x01
        header["isRemovableChildDB"] = (header["Attributes"] & 0x02) == 0x02
        header["isModified"] = (header["Status"] & 0x01) == 0x01

        # Read pointers
        pattern = ">II" + "II"*header["NumOfKeys"]
        size_required = struct.calcsize(pattern)
        pointers = list(struct.unpack(pattern,
                                      header_data[index:index+size_required]))

        pDB_Name = pointers.pop(0)
        header["DB_Name"] = read_display_data(header_data[to_offset(pDB_Name):])
        pParentDBFileName = pointers.pop(0)
        if pParentDBFileName != 0:
            header["ParentDBFileName"] = read_stringz(header_data[to_offset(pParentDBFileName):])
        else:
            header["ParentDBFileName"] = None
            
        Rules = []
        while len(pointers) > 0:
            pRuleName = pointers.pop(0)
            RuleName = read_display_data(header_data[to_offset(pRuleName):])
            pRuleFileName = pointers.pop(0)
            if pRuleFileName != 0:
                RuleFileName = read_stringz(header_data[to_offset(pRuleFileName):])
            else:
                RuleFileName = None
            Rules.append((RuleName,RuleFileName))
        header["Rules"] = Rules

        return header

    def _read_ximdata(self, f):
        "Reads the XIM data from the MDB header, but does not parse it"

        # Figure out how much to read
        (length, ) = struct.unpack(">H", f.read(2))
        length = to_offset(length)
        
        f.seek(-2, 1)

        return f.read(length)

    def clear(self):
        """Remove all entries from database.

        Returns the file pointer to the required null record."""
        f = self.file

        # Truncate to just after header
        record_start = self.header["RecordStart"]
        f.truncate(to_offset(record_start))

        # Add required null record
        null_rec_pointer = self.append_record(None)

        f.flush()

        return null_rec_pointer

    def close(self):
        self.file.close()
        self.__init__()


    pass

