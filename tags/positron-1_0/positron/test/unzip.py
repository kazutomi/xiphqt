import sys
import os
from os import path
import zipfile

def unzip(source_file, target_dir):
    """Unzips source_file and writes the contents to target_dir"""

    zip = zipfile.ZipFile(source_file)

    for item in zip.namelist():
        fullpath = path.join(target_dir, item)

        if fullpath.endswith("/"):
            os.mkdir(fullpath)
        else:
            target_file = file(fullpath, "wb")
            data = zip.read(item)
            target_file.write(data)
            target_file.close()

def unzip_usage():
    print "Usage: unzip zipfile [target_dir]"
    
if __name__ == "__main__":
    if len(sys.argv) == 1:
        unzip_usage()
    elif len(sys.argv) == 2:
        unzip(sys.argv[1], path.abspath("."))
    elif len(sys.argv) == 3:
        unzip(sys.argv[1], path.abspath(sys.argv[2]))
    else:
        unzip_usage()
