# -*- Mode: python -*-
#
# cmd_sync.py - sync command
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
                                             config.supported_music_types(),
                                             silent=True))

        # Cut explicitly deleted files
        deletedlist = [neuros.neurospath_to_hostpath(f)
                       for f in config.get_deleted()]
        filelist = [item for item in filelist
                    if item[1].lower() not in deletedlist]

        # Cut recordings (so we don't resync them back onto the Neuros in
        # the event the recordings dir is under the music tree)
        if config.recordingdir != None:
            recordinglist = config.get_recordings()
            targets_to_avoid = []
            for neuros_trackname in recordinglist:
                sourcename = neuros.neurospath_to_hostpath(neuros_trackname)
                basename = path.basename(sourcename)
                targetname = path.join(config.recordingdir, basename)
                # We lowercase everything here because config.get_recordings
                # only contains lowercased names, so we're going to lower
                # case everything before comparison
                targets_to_avoid.append(targetname.lower())

            filelist = [item for item in filelist
                        if item[0].lower() not in targets_to_avoid]

        if len(filelist) == 0:
            print "None."
        else:
            if len(filelist) == 1:
                print "Copying 1 new track."
            else:
                print "Copying %d new tracks."% (len(filelist),)

            i = 1
            for sourcename, targetname, metadata in filelist:
                basename = path.basename(sourcename)
                print "    %d. %s..." % (i, basename)
                i += 1
                add_track(neuros, sourcename, targetname, metadata)


    if config.recordingdir == None:
        print "  No directory given in config for recordings.  Skipping."
    else:
        print "  Checking for new recordings...  ",
        recordinglist = config.get_recordings()
        new_recordings = [track[8]
                          for track in audio_db.get_records()
                          if track != None and track[5] != None \
                          and track[8].lower() not in recordinglist]

        if len(new_recordings) == 0:
            print "None."
        else:
            if len(filelist) == 1:
                print "Copying 1 new recording to host."
            else:
                print "Copying %d new recordings to host." \
                      % (len(new_recordings),)

            i = 1
            for neuros_trackname in new_recordings:
                sourcename = neuros.neurospath_to_hostpath(neuros_trackname)
                basename = path.basename(sourcename)
                targetname = path.join(config.recordingdir, basename)

                print "    %d. %s..." % (i, basename)
                i += 1
                util.copy_file(sourcename, targetname)
                config.add_recording(neuros_trackname.lower())

    # Only pack when necessary
    if audio_db.count_deleted() > 0:
        print "  Packing audio database...",
        audio_db.pack()
        print " Done."

    if config.sort_database:
        audio_db.sort()
        
    neuros.close_db("audio")
    
