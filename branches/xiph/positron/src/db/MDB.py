import struct
from util import *

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

        try:
            new_extra_format = []
            for fmt in extra_format:
                if fmt == "z":
                    new_extra_format.append((fmt, None))
                else:
                    new_extra_format.append((fmt, struct.calcsize(fmt)))
        except struct.error, e:
            # Catch improper format strings
            raise Error(e.value)

        self.extra_format = new_extra_format

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
        """Returns a tuple with the record at pointer and a pointer to the.next record.

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

        # Read primary field
        (record["data"], delim) = fread_escaped_string(f)

        if delim == Special.END_OF_RECORD:
            if record["data"] == "":
                # Special empty record
                return (None, to_pointer(f.tell()))
            else:
                # Degenerate case for child databases
                record["keys"] = ()
                record["extra"] = ()
                return (record, to_pointer(f.tell()))

        # Read access keys (field delimiter followed by word pointer)
        # Need to account for having already read first delimiter
        pattern = ">" + "2sI" * (self.header["NumOfKeys"]-1)
        size_required = struct.calcsize(pattern)
        data = struct.unpack(pattern, delim + f.read(size_required-2))

        # Split field delimeters and record keys apart.
        # In Python 2.3, I can just do:
        # delims = data[::2]
        # record["keys"] = list(data[1::2])
        delims = []
        record["keys"] = []
        for i in range(len(data)):
            if i % 2 == 0:
                delims.append(data[i])
            else:
                record["keys"].append(data[i])

        # Check field delimeters
        for i in range(len(delims)):
            if delims[i] != Special.FIELD_DELIM:
                raise Error("Field delimeter not found after field %d" %(i,))

        # Read extra fields
        record["extra"] = []
        i = self.header["NumOfKeys"]-1
        need_delim = True
        for fmt, size in self.extra_format:
            if need_delim:
                word = f.read(2)
            if word != Special.FIELD_DELIM:
                raise Error("Field delimiter not found after field %d" %(i,))

            if fmt == "z":
                (value, word) = fread_escaped_string(f)
                need_delim = False  # Already got the next field delimiter
            else:
                (value, ) = struct.unpack(fmt, f.read(size))
                need_delim = True

            record["extra"].append(value)

            i += 1

        # Finally, make sure we are at the end of the record
        if need_delim:
            word = f.read(2)
        if word != Special.END_OF_RECORD:
            raise Error("End of record delimiter not found.")

        return (record, to_pointer(f.tell()))

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


    def append_record(self, record):
        """Writes a new record to the end of the file.

        The record should be formatted in the same way as the return value from
        read_record_at()."""

        new = ""

        # Record flag
        flags = 0x8000
        if record["isDeleted"]:
            flags |= 0x0001

        new += struct.pack(">H", flags)

        # Primary Record Data
        new += escape_string(record["data"])

        # Access Keys
        if len(record["keys"]) != self.header["NumOfKeys"]-1:
            raise Error("Incorrect number of access keys in record")

        for key in record["keys"]:
            new += Special.FIELD_DELIM
            new += struct.pack(">I", key)

        # Extra Info Fields
        if len(record["extra"]) != (self.header["NumOfFieldsPerRecord"]
                                    - self.header["NumOfKeys"]):
            raise Error("Incorrect number of extra info fields in record")

        for i in range(len(record["extra"])):
            new += Special.FIELD_DELIM
            if self.extra_format[i][0] == "z":
                new += escape_string(record["extra"][i])
            else:
                new += struct.pack(self.extra_format[i][0], record["extra"][i])

        # End record
        new += Special.END_OF_RECORD

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

    def close(self):
        self.file.close()
        self.__init__()


    pass

