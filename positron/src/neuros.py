import db
import os
from os import path

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

def _total_path_split(pathname):
    path_parts = []
    (dirname, basename) = path.split(pathname)
    while dirname != "" and dirname != "/":
        if basename != "":
            path_parts.insert(0, basename)
        (dirname, basename) = path.split(dirname)

    if basename != "":
        path_parts.insert(0, basename)
    if dirname != "":
        path_parts.insert(0, dirname)

    return path_parts

class Neuros:

    DB_DIR = path.normcase("WOID_DB")

    db_formats = {
        "audio"    : { "extra_format" : (">I",">I","z"),
                       "extra_names"  : ("Time", "Size", "Path") },
        
        "pcaudio"  : { "extra_format" : (">I",">I","z"),
                       "extra_names"  : ("Time", "Size", "Path") },
        
        "idedhisi" : { "extra_format" : (">I",">I","z"),
                       "extra_names"  : ("Time", "Size", "Path") }
        }


    def __init__(self, mountpoint):
        self.mountpoint = mountpoint

        # Check and see if the mountpoint looks legit
        dbpath = path.join(self.mountpoint, Neuros.DB_DIR)
        try:
            os.listdir(dbpath)
        except OSError:
            raise Error("%s does not look like a Neuros mountpoint"
                        % (mountpoint,))

        # Handy to keep around
        self.mountpoint_parts = _total_path_split(self.mountpoint)

        self.db = {}
        for db_name in self.db_formats.keys():
            self.db[db_name] = None

    def open_db(self, name):

        if name not in self.db.keys():
            raise Error("Database %s is not a valid Neuros database"
                        % (name, ))

        rootpath = path.join(self.mountpoint, Neuros.DB_DIR, name, name)

        self.db[name] = db.WOID()
        self.db[name].open(rootpath, self.db_formats[name]["extra_format"])
        return self.db[name]

    def close_db(self, name):
        if name not in self.db.keys():
            raise Error("Database %s is not a valid Neuros database"
                        % (name, ))

        if self.db[name] == None:
            raise Error("Database %s is not open" % (name, ))

        self.db[name].close()
        self.db[name] = None

    def get_serial_number(self):

        serial_filename = path.join(self.mountpoint, "sn.txt")
        
        f = file(serial_filename, "r")
        serial = f.readline()

        return serial

    def is_valid_hostpath(self, hostpath):
        """Checks if a given path on the host is a subdirectory (or the root)
        of the Neuros mountpoint."""
        
        hostpath_parts = _total_path_split(hostpath)

        return self.mountpoint_parts == \
               hostpath_parts[:len(self.mountpoint_parts)]

    def hostpath_to_neurospath(self, hostpath):
        if not self.is_valid_hostpath(hostpath):
            raise Error("Host path not under Neuros mountpoint")

        # Get the path parts, but prune off the mountpoint
        hostpath_parts = _total_path_split(hostpath)[len(self.mountpoint_parts):]

        hostpath_parts.insert(0,"C:")

        # Now join it all with forward slashes to get the Neuros path
        return "/".join(hostpath_parts)

    def neurospath_to_hostpath(self, neurospath):
        neurospath_parts = neurospath.split("/")

        if len(neurospath_parts) < 1 or neurospath_parts[0] != "C:":
            raise Error("Neuros path does not start with C:")

        hostpath_parts = self.mountpoint_parts + neurospath_parts[1:]

        return path.join(*hostpath_parts)
