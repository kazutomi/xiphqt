"""positron sync:\tSynchronizes your Neuros with your music library

  positron sync
  
     Copies any new music tracks from the directories specified in your
     config file to the Neuros.  Also copies new FM or microphone
     recordings from the Neuros to your computer.

     Positron remembers which tracks you have removed using
     \"positron del\" and will not add them again when you sync.
     Similarly, if you delete a recording from the host computer after
     it has been copied, it will not be copied again.

  positron sync --reset-deleted
  positron sync --reset-recordings

     Clears the list of deleted files or already-transferred recordings
     (respectively), which are ignored during sync.

"""

import sys
import os
import getopt
from os import path
from add_file import *
from neuros import Neuros
import util

def run(config, neuros, args):
    audio_db = neuros.open_db("audio")

    try:
        opts, remaining = getopt.getopt(args, "", ("reset-deleted",
                                                   "reset-recordings"))
    except getopt.GetoptError, e:
        print "Error:", e
        print
        print __doc__
        sys.exit()

    reset = False
    for o,a in opts:
        if o == "--reset-deleted":
            reset = True
            config.clear_deleted()
            print "Clearing deleted file list."            
        elif o == "--reset-recordings":
            reset = True
            config.clear_recordings()
            print "Clearing recording file list."

    if reset:
        return

    print "Synchronizing Neuros music database.\n"


    if len(config.syncdirs) == 0:
        print "  No music directories given in config file for synchronization.  Skipping."
    else:
    
        print "  Checking for new music...       ",

        # Generate list of candidates
        filelist = []
        for source, dest in config.syncdirs:
            targetdir = path.join(neuros.mountpoint, dest)

            if not path.isdir(source):
                print "Error: %s is not a directory.  Cannot synchronize with it." % (source,)
                sys.exit(1)
            
            for item in os.listdir(source):
                filelist.extend(gen_filelist(neuros, source, item, targetdir,
                                             silent=True))

        # Cut explicitly deleted files
        deletedlist = [neuros.neurospath_to_hostpath(f)
                       for f in config.get_deleted()]
        filelist = [item for item in filelist
                    if item[1].lower() not in deletedlist]

        if len(filelist) == 0:
            print "None."
        else:
            if len(filelist) == 1:
                print "Copying 1 new track."
            else:
                print "Copying %d new tracks."% (len(filelist),)
                
            for (sourcename, targetname) in filelist:
                basename = path.basename(sourcename)
                print "    %s..." % (basename,)
                add_track(neuros, sourcename, targetname)


    if config.recordingdir == None:
        print "  No directory given in config for recordings.  Skipping."
    else:
        print "  Checking for new recordings...  ",
        recordinglist = config.get_recordings()
        new_recordings = [track
                          for track in audio_db.get_records()
                          if track != None and track[5] != None \
                          and track[8].lower not in recordinglist]

        if len(new_recordings) == 0:
            print "None."
        else:
            if len(filelist) == 1:
                print "Copying 1 new recording to host."
            else:
                print "Copying %d new recordings to host." \
                      % (len(new_recordings),)

            for neuros_trackname in new_recordings:
                sourcename = neuros.neurospath_to_hostpath(neuros_trackname)
                basename = path.basename(sourcename)
                targetname = path.join(config.recording_dir, basename)

                print "    %s..." % (basename,)
                util.copy_file(sourcename, targetname)
                config.add_recording(neuros_trackname.lower())
    
    neuros.close_db("audio")
    
