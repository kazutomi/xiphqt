"""positron del: Removes files from the Neuros database and disk

  positron del [sources]

     [sources] - A list of files or directories on the Neuros to remove
     Note that only files referenced in the database will be removed, others
     will not be affected.
"""

import os
from os import path
from neuros import Neuros
import db
import util

def usage():
    print __doc__
    
def gen_filelist(neuros, pathname):
    filelist = []
    fullname = path.abspath(pathname)
    empty = True

    if not neuros.is_valid_hostpath(fullname):
        print "Warning: Ignoring %s because it is not a path on the Neuros." \
              % (pathname, )
    elif path.isfile(pathname):

        neuros_path = neuros.hostpath_to_neurospath(fullname)

        # Search for this pathname in the last field
        sai_index = neuros.db["audio"].find(neuros_path, 8)
        if sai_index == None:
            empty = False
        else:
            filelist.append((fullname, sai_index))
                    
    elif path.isdir(pathname):
        subs = os.listdir(pathname)

        for sub in subs:
            (sub_list, sub_empty) = gen_filelist(neuros,
                                                 path.join(pathname, sub))
            filelist.extend(sub_list)
            empty = empty and sub_empty

        # If all the contents of this directory are (or are going to
        # be empty), then this directory can be deleted
        if empty:
            filelist.append((fullname, None))
    else:
        print "Ignoring %s.  Not a file or directory." % (pathname)

    return (filelist, empty)


def del_track(neuros, sourcename, sai_index):
    try:
        os.remove(sourcename)
        # Remove entry from database
        neuros.db["audio"].delete_record(sai_index)
    except os.error, e:
        print "Error:", e
    except db.util.Error, e:
        print "Error:", e

def cmd_del(config, neuros, args):
    audio_db = neuros.open_db("audio")

    filelist = []

    if len(args) == 0:
        usage()
        return

    for arg in args:
        (new_list, empty) = gen_filelist(neuros, arg)
        filelist.extend(new_list)
            
    if len(filelist) == 0:
        print "No files to remove are present in the database!"
    elif len(filelist) == 1:
        (sourcename, sai_index) = filelist[0]
        basename = path.basename(sourcename)
        print "Removing %s from the Neuros..." % (basename,),
        del_track(neuros, sourcename, sai_index)
        print "  Done!"
    else:
        tracks = len([item for item in filelist if item[1] != None])
        print "Removing %d tracks from the Neuros..." % (tracks,)
        for (sourcename, sai_index) in filelist:
            basename = path.basename(sourcename)
            if sai_index != None:
                print "  %s..." % (basename, )
                del_track(neuros, sourcename, sai_index)
            else:
                print "  Removing empty directory %s..." % (sourcename, )
                try:
                    os.rmdir(sourcename)
                except os.error, e:
                    print "Error:", e

        print "\nDone!"
    neuros.close_db("audio")
    
