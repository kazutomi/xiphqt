import os

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
