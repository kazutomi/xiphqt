#!/usr/bin/env python
# -*- Mode: python -*-
#
# positron.py - main frontend
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

import sys
from os import path
import getopt
from config import Config
from neuros import Neuros
import neuros
import cmd_add
import cmd_del
import cmd_list
import cmd_clear
import cmd_pack
import cmd_sync
import cmd_config
import cmd_rebuild

import ports

version = "Xiph.org Positron version 0.1"

# Hash table of commands.  The first value in the tuple is the module
# where the command is stored.  The second value is the order to
# display commands in from usage()
# Note: "config" is not run using the normal command dispatcher
commands = { "add"     : (cmd_add,     1),
             "sync"    : (cmd_sync,    2),
             "del"     : (cmd_del,     3),
             "config"  : (cmd_config,  4),
             "rebuild" : (cmd_rebuild, 5),
             "list"    : (cmd_list,    6),
             "clear"   : (cmd_clear,   7),
             "pack"    : (cmd_pack,    8) }

# For sorting according to the display order element in the tuple
def cmp_func(a, b):
    return a[1] - b[1]

def usage():
    print version, "- Neuros portable music player sync tool"
    print
    
    cmds = commands.values()
    cmds.sort(cmp_func)

    for (item, order) in cmds:
        first_line = item.__doc__.split("\n")[0]
        print "  ",first_line

    print
    print "For more help on a specific command, type: positron help <command>"


def main(argv):
    options = "c:hm:v"
    long_options = ("config=", "help", "mount-point=", "version")

    # parse global options
    try:
        opts, remaining = getopt.getopt(argv[1:], options, long_options)
    except getopt.GetoptError, e:
        print "Error:", e
        usage()
        sys.exit()

    config = Config()
    
    for o,a in opts:
        if o in ("-v", "--version"):
            print version
            sys.exit()
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-c", "--config"):
            config.set_config_dir(a)
        elif o in ("-m", "--mount-point"):
            mountpoint = a

    if len(remaining) == 0:
        usage()
        sys.exit(0)
    elif remaining[0] == "help":
        if len(remaining) > 1:
            if commands.has_key(remaining[1]):
                (cmd, display_order) = commands[remaining[1]]
                print cmd.__doc__
            else:
                    print remaining[1], "is not a valid command."
            sys.exit(0)
        else:
            usage()
            sys.exit(0)
    elif remaining[0] == "config":
        # Config gets special treatment because everything might be screwed
        # up until we run it
        (cmd, display_order) = commands["config"]
        cmd.run(config, None, remaining[1:])
        sys.exit(0)
        

    # Read configuration
    try:
        config.read_config_file()
    except IOError,e:
        print "Error opening config file:\n  %s" \
              % (e,)
        print "You can create or modify the configuration file with \"positron config\""
        sys.exit(1)

    # Sanity check
    if not config.mountpoint:
        print "Error: Neuros mountpoint not set with -m and not present in config file."
        print "Please use \"positron config\" to create or modify the configuration."
        sys.exit(1)

    try:
        myNeuros = Neuros(config.mountpoint)

        if commands.has_key(remaining[0]):
            (cmd, display_order) = commands[remaining[0]]
            cmd.run(config, myNeuros, remaining[1:])
        else:
            print remaining[0], "is not a valid command."
            print
            usage()

        exit_value = 0
    except neuros.Error, e:
        print "Error:", e
        exit_value = 1

    sys.exit(exit_value)

    
if __name__ == "__main__":
    main(sys.argv)
