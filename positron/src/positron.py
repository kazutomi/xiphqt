#!/usr/bin/env python
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

import ports

version = "Xiph.org Positron version 0.1"

# Hash table of commands.  The first value in the tuple is the module
# where the command is stored.  The second value is the order to
# display commands in from usage()
commands = { "add"  : (cmd_add,   1),
             "del"  : (cmd_del,   2),
             "list" : (cmd_list,  3),
             "clear": (cmd_clear, 4),
             "pack" : (cmd_pack,  5) }

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
            config.config_filename = a
        elif o in ("-m", "--mount-point"):
            config.mountpoint = a

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
