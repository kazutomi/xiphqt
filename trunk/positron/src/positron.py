import sys
from os import path
import getopt
from ConfigParser import ConfigParser

import progress
import ports

def usage():
    pass

def cmd_sync(econtrol, args):
    pass

def cmd_add(econtrol

def main(argv):
    options = "c:i:hv"
    long_options = ("config=", "help", "mount-point=", "version")

    # parse global options
    try:
        opts, remaining = getopt.getopt(argv[1:], options, long_options)
    except getopt.GetoptError:
        print "Invalid option"
        usage()
        sys.exit()

    config_file = ports.config_file_path()
    config_defaults = { "mount-point" => None }

    for o,a in opts:
        if o in ("-v", "--version"):
            print "Xiph.org positron version 0.1"
            sys.exit()
        elif o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-c", "--config"):
            config_dir = a
        elif o in ("-m", "--mount-point"):
            mount_point = a

    config = ConfigParser()
    

    # now select major command
    if len(remaining) == 0:
        usage()
    elif remaining[0] == "set":
        cmd_set(econtrol, remaining[1:])
    elif remaining[0] == "list":
        if len(remaining) == 1:
            list_usage()
        elif remaining[1].startswith("dir"):
            cmd_list_directories(econtrol, remaining[2:])
        elif remaining[1].startswith("chan"):
            cmd_list_channels(econtrol, remaining[2:])
        elif remaining[1] == "new":
            cmd_list_new(econtrol, remaining[2:])
    elif remaining[0] == "add":
        cmd_add(econtrol, remaining[1:])
    elif remaining[0] == "subscribe":
        cmd_subscribe(econtrol, remaining[1:])
    elif remaining[0] == "unsubscribe":
        cmd_unsubscribe(econtrol, remaining[1:])
    elif remaining[0] == "download":
        cmd_download(econtrol, remaining[1:])

    sys.exit()

    
if __name__ == "__main__":
    main(sys.argv)
