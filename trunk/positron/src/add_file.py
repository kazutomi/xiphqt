"Utility module for adding files to the Neuros.  Used by cmd_sync and cmd_add."

import os
from os import path
from audiofile import MP3File
from neuros import Neuros
import util

mp3file = MP3File()

def gen_filelist(neuros, prefix, suffix, target_prefix, silent=False):
    filelist = []
    fullname = path.join(prefix, suffix)

    if path.isdir(fullname):
        files = [path.join(suffix,name) for name in os.listdir(fullname)]
    else:
        files = [suffix]

    for name in files:
        fullname = path.join(prefix, name)
        
        if path.isfile(fullname):
            if not mp3file.detect(fullname):
                if not silent:
                    print "Skipping %s.  Not a recognized audio format." \
                          % (fullname,)
            elif neuros.is_valid_hostpath(fullname):
                # Don't need to copy files already on the Neuros
                filelist.append((None, fullname))
            else:
                targetname = path.join(target_prefix, name)

                if path.exists(targetname):
                    if not silent:
                        print "Skipping %s because %s already exists." \
                              % (fullname, targetname)
                else:
                    filelist.append((fullname, targetname))
                    
        elif path.isdir(fullname):
            filelist.extend(gen_filelist(neuros, prefix, name, target_prefix,
                                         silent))
        else:
            if not silent:
                print "Ignoring %s.  Not a file or directory." % (fullname)

    return filelist


def add_track(neuros, sourcename, targetname):

    if sourcename == None:
        # File already on Neuros
        info = mp3file.get_info(targetname)
    else:
        # File needs to be copied to Neuros
        info = mp3file.get_info(sourcename)
        util.copy_file(sourcename, targetname)

    # Create DB entry
    record = (info["title"], None, info["artist"], info["album"],
              info["genre"], None, info["length"], info["size"] // 1024,
              neuros.hostpath_to_neurospath(targetname))
    # Add entry to database
    neuros.db["audio"].add_record(record)
    
