"""positron add:\tAdds files to the Neuros database, copying as necessary

  positron add <sourcefile> <targetfile>

     Copies sourcefile to targetfile (a path on the Neuros that does not exist)

  positron add <sourcefile1> <sourcefile2> ... <targetdir>

     Copies sourcefiles to targetdir (a directory on the Neuros that already
     exists)

  positron add <sourcefile1> <sourcefile2> ...

     Copies sourcefiles to the default music directory.

Files that are already on the Neuros will not be overwritten, but just
added to the database if not already present.  Non-music files are
ignored.
"""

import os
from os import path
from audiofile import MP3File
from neuros import Neuros
import util

mp3file = MP3File()

def gen_filelist(neuros, prefix, suffix, target_prefix):
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
                print "Skipping %s.  Not a recognized audio format." \
                      % (fullname,)
            elif neuros.is_valid_hostpath(fullname):
                # Don't need to copy files already on the Neuros
                filelist.append((None, fullname))
            else:
                targetname = path.join(target_prefix, name)

                if path.exists(targetname):
                    print "Skipping %s because %s already exists." \
                          % (fullname, targetname)
                else:
                    filelist.append((fullname, targetname))
                    
        elif path.isdir(fullname):
            filelist.extend(gen_filelist(neuros, prefix, name, target_prefix))
        else:
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

            
def run(config, neuros, args):
    audio_db = neuros.open_db("audio")

    # There are a couple different cases we could be dealing with:
    # 1) Last argument is a filename on the Neuros that does not exist.
    #    => First argument is the source name, last is the target name
    # 2) Last argument is an existing directory on the Neuros
    #    => Previous arguments are all source names, target names are
    #       last plus basename of the source
    # 3) Last argument is not one of the previous two cases
    #    => Use case 2, but create a new argument that is the default music
    #       directory
    # *) Special case: Any source names that resolve to Neuros paths
    #    automatically have the same target name as source name (and then
    #    the source name is nulled to signal that no copying is required)

    filelist = []

    if len(args) == 0:
        print __module__.__doc__
        return
    
    # Deal with case 3 first
    if not neuros.is_valid_hostpath(args[-1]) or path.isfile(args[-1]):
        musicdir = path.join(neuros.mountpoint, config.musicdir)

        if not path.exists(musicdir):
            os.makedirs(musicdir)  # Does not work on Win32 UNC paths

        args.append(musicdir)

    # Now we are only in case 1 or 2
    if neuros.is_valid_hostpath(args[-1]) and not path.exists(args[-1]):
        if len(args) != 2 or path.isdir(args[0]):
            print "Error: Cannot copy multiple files to %s if it is not a directory." % (args[-1],)
            return

        filelist.append((args[0], args[1]))
    elif neuros.is_valid_hostpath(args[-1]) and path.isdir(args[-1]):
        print "Generating file list..."
        for arg in args[:-1]:
            (prefix, suffix) = path.split(arg)
            filelist.extend(gen_filelist(neuros, prefix, suffix, args[-1]))
            

    if len(filelist) == 0:
        print "No files left to add!"
    elif len(filelist) == 1:
        (sourcename, targetname) = filelist[0]
        print "Adding %s to the Neuros..." % (sourcename,),
        add_track(neuros, sourcename, targetname)
        print "  Done!"
    else:
        print "Adding %d tracks to the Neuros..." % (len(filelist),)
        for (sourcename, targetname) in filelist:
            print "  %s..." % (sourcename,)
            add_track(neuros, sourcename, targetname)

        print "\nDone!"
    neuros.close_db("audio")
    
