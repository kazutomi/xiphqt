# -*- Mode: python -*-
#
# cmd_config.py - config command
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

"""positron config:\tCreate or modify the current configuration

Asks several questions, and creates a new configuration set for
positron.  Also allows the current configuration to be edited.
"""

import neuros
from util import trim_newline

def bool_input(prompt, default):
    if default:
        prompt += " [Y/n] "
    else:
        prompt += " [y/N] "

    while True:
        value = trim_newline(raw_input(prompt))
        if value.lower() in ("y", "yes"):
            return True
        elif value.lower() in ("n", "no"):
            return False
        elif value == "":
            return default
        else:
            print "Invalid option.  Please enter \"y\" or \"n\"."

def menu_input(prompt, values, default, option_hints = True):
    if option_hints:
        prompt += " ["+default+"] "
    else:
        prompt += " "

    while True:
        value = trim_newline(raw_input(prompt).lower())
        if value in values:
            return value
        elif value == "":
            return default
        else:
            print "Invalid option.  Please enter: "+(",".join(values))

def display_syncdirs(config):

    syncdirs = config.syncdirs
    
    print "Synchronized directories"
    print "------------------------"
    print "(Format: [source directory] => [subdirectory on neuros]"
    print
    
    if len(syncdirs) == 0:
        print "No synchronized directories specified!"
    else:
        for i in range(len(syncdirs)):
            src = syncdirs[i][0]
            dest = syncdirs[i][1]
            if dest == None:
                dest = config.neuros_musicdir
                
            print "%d) %s => %s" % (i+1, src, dest)

def syncdir_input():
    src_prompt = "Enter the directory on your computer to synchronize with.\n=> "
    dest_prompt = "Enter the subdirectory on the Neuros to copy files to.  Do not include the path\nto the Neuros mountpoint. [Leave blank to use default]\n=> "
    
    src = trim_newline(raw_input(src_prompt))
    print
    dest = trim_newline(raw_input(dest_prompt))
    if dest == "":
        dest = None

    return (src, dest)


def run(config, bogus_neuros, args):

    # Because of early dispatch of the config command,
    # config.read_config_file() has not been called and bogus_neuros == None

    # Try to load old configuration
    try:
        config.read_config_file()
        prior_config = True
    except Exception, e:
        prior_config = False
    
    print "Positron configuration"
    print
    print "Please connect your Neuros to your computer and mount it."
    raw_input("Press return when ready.")
    print

    # Read mountpoint
    while True:
        if config.mountpoint == None:
            old_mountpoint = ""
        else:
            old_mountpoint = config.mountpoint
            
        mountpoint = trim_newline(
            raw_input("Where is your Neuros mounted? [%s] " % old_mountpoint))
        if mountpoint == "":
            mountpoint = old_mountpoint
        try:
            n = neuros.Neuros(mountpoint)
            print "Neuros found at that mountpoint."
            done = True
        except neuros.Error:
            print "Warning: A Neuros does not appear to be mounted at %s." \
                  % (mountpoint,)
            if bool_input("Would you like to try a different mountpoint?",
                          True):
                continue

        config.mountpoint = mountpoint
        break

    print
    print \
"""Positron can copy new FM and microphone recordings from your Neuros to your
computer when you synchronize with \"positron sync\"."""
    print
    
    if bool_input("Would you like to enable this feature?",
                  not prior_config or config.recordingdir != None):
        # Read recordingdir
        while True:
            if config.recordingdir == None:
                old_recordingdir = ""
            else:
                old_recordingdir = config.recordingdir
            
            recordingdir = trim_newline(
                raw_input("Where should recordings be copied to? [%s]\n=>  " % old_recordingdir))
            if recordingdir == "":
                recordingdir = old_recordingdir

            # What error checking should go here?
            config.recordingdir = recordingdir
            break
    else:
        config.recordingdir = None

    print

    # Configure sync dirs
    if len(config.syncdirs) == 0:
        print \
"""Positron can automatically find new music files in the directories you
specify and copy them to the Neuros during synchronization."""
        print
        config_syncdirs = bool_input("Would you like to configure this?", True)
    else:
        config_syncdirs = True

    if prior_config:
        default_option = "d"
    else:
        default_option = "a"

    while config_syncdirs:

        display_syncdirs(config)

        print
        print "[a]dd another synchronized directory"
        print "[r]emove a synchronized directory"
        print "[d]one configuring"
        option = menu_input("Command?", ["a","r","d"], default_option,
                            option_hints=True)

        if option == "a":
            print
            config.syncdirs.append(syncdir_input())
            default_option = "d"
        elif option == "r":
            if len(config.syncdirs) == 0:
                print "No directories to remove from list."
                continue
            prompt = "Number of directory to remove from list? (\"c\" to cancel)"
            allowed_values = [str(i+1) for i in range(len(config.syncdirs))] \
                             + ["c"]
            value = menu_input(prompt, allowed_values, "c", option_hints=False)
            print
            if value == "c":
                continue
            else:
                del config.syncdirs[int(value) - 1]
        elif option == "d":
            break


    print
    print
    print "New Configuration"
    print "-----------------"
    print
    print "Neuros Mountpoint = %s" % (config.mountpoint,)
    print "Recording Directory = %s" % (config.recordingdir,)
    print
    display_syncdirs(config)

    print
    if bool_input("Write this configuration to disk?", True):
        config.create_new_config()
        print "Configuration written to disk."
    else:
        print "Configuration discarded."
