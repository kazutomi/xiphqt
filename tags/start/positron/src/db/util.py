import struct


class Error(Exception):
    """Base class for exceptions in this module."""
    pass

class Special:
    # Special characters

    # '#' - Marks the end of a file within a record
    FIELD_DELIM = "\x00\x23"

    # '%' - Marks the end of the current record
    END_OF_RECORD = "\x00\x25"

    # '$' - NO IDEA!!!
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

def escape_string(str):
    """Returns str with special characters escaped and null-word terminated"""
    # I'm unclear on how escaping is used in practice (I don't see it in any of the
    # sample files), so for now this is almost a no-op.  I just null-terminate it.

    new = str

    # Need to word align things and add a null-termination word
    if len(new) % 2 == 0:
        return new + "\x00\x00"
    else:
        return new + "\x00\x00\x00"

def fread_word(f):
    "Reads a 16 bit big endian word from file object f."
    return struct.unpack(">H", f.read(2))[0]

def fwrite_word(f, word):
    "Writes an integer as a 16 bit big endian word to file object f."
    f.write(struct.pack(">H", word))

def fread_escaped_string(f):
    """Reads string with special characters escaped from f.

    Also consumes extra nulls."""

    str = ""
    word = f.read(2)
    escape_mode = False
    while word != "":
        if escape_mode:
            str += word
            escape_mode = False
        else:
            if word == Special.FIELD_DELIM or word == Special.END_OF_RECORD:
                # In order to consume extra nulls, we actually terminate on the
                # delimiter and not on the null, like one would expect
                break
            elif word == Special.ESCAPER:
                escape_mode = True
            elif word == "\x00\x00":
                pass
            elif word[1] == "\x00":
                str += word[0]
            else:
                str += word

        word = f.read(2)
    else:
        raise Error("Premature EOF while reading string.")
    
    return (str, word)
