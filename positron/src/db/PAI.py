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
        pointers = []

        pointer = FILE_HEADER_LEN

        f.seek(to_offset(pointer))
        length_str = f.read(2)
        while length_str != "":
            pointers.append(pointer)
            
            length = struct.unpack(">H", length_str)
            pointer += length

            f.seek(to_offset(pointer))
            length_str = f.read(2)

        return pointers

    def _read_module_header(self, pointer):
        f = self.file

        f.seek(to_offset(pointer))

        pattern = ">HHH6x"
        size_required = struct.calcsize(pattern)
        return struct.unpack(pattern, f.read(size_required))

    def read_module_at(self, pointer):
        f = self.file

        (length, flag, num_entries) = _read_module_header(pointer)

        if flag & 0x0001 == 0x0001:
            return ([], pointer+length)

        entries = []
        for i in range(num_entries):
            entry = struct.unpack(">I", f.read(4))
            entries.append(entry)

        return (entries, pointer+length)

    def add_entry_to_module_at(self, pointer, entry):
        f = self.file

        # Calculate empty space
        (length, flag, num_entries) = self._read_module_header(pointer)

        # Total length of module is 6 word header, 2 word ending zeros,
        # num_entries * 32 bit word pointers, and empty space
        if (length - PAI.MODULE_HEADER_LEN - PAI.MODULE_FOOTER_LEN
            - num_entries * 2) == 0:
            extended = True
            self.extend_module_at(pointer, 1)

            # Reread header
            (length, flag, num_entries) = self._read_module_header(pointer)
        else:
            extended = False


        # Update part of header
        flag &= 0xFFFE
        num_entries += 1
        header = struct.pack(">HH", flag, num_entries)
        f.seek(to_offset(pointer+1)) # Skipping length value
        f.write(header)

        # Add entry   
        entry_pointer = pointer + PAI.MODULE_HEADER_LEN + num_entries * 2
        packed_entry = struct.pack(">I", entry)
        
        f.seek(to_offset(entry_pointer))
        f.write(packed_entry)

        if extended:
            return self.get_module_pointers()
        else:
            return None
        
    def delete_entry_in_module_at(self, pointer, entry_num):
        f = self.file

        (length, flag, num_entries) = _read_module_header(pointer)

        # Sanity check
        if entry_num >= num_entries:
            raise Error("entry_num is greater than the number of entries")

        # Find the word pointer to word *after* entry_num.  If entry_num is last
        # then we will be pointing at the terminating null longword
        next_entry = pointer + PAI.MODULE_HEADER_LEN + 2 * (entry_num + 1)

        # Read everything from here to the end of the module (including footer)
        read_len = length - (pointer - next_entry)

        f.seek(to_offset(next_entry))
        module_remainder = f.read(to_offset(read_len))

        # Now go to the entry to delete and write over it.  The terminating null
        # footer will ensure that the extra space is null padded correctly.
        f.seek(to_offset(next_entry - 2))
        f.write(module_remainder)
        f.flush()

    def append_module(self, entries):
        f = self.file

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

        return position

    def extend_module_at(self, pointer, chunks=1):
        f = self.file

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
        (length_str,) = struct.pack(">H", length)
        f.write(length_str)
        f.flush()


    def set_empty_module_at(self, pointer, value=True):
        f = self.file

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
        f.seek(to_pointer(PAI.FILE_HEADER_LEN))
        f.truncate()
        f.flush()
    
    def close(self):
        self.file.close()
        self.__init__()

