# -*- Mode: python -*-
#
# config.py - config file handling
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

import ports
import shlex
import os
from os import path
from util import trim_newline
import ConfigParser

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

def parse_boolean(s):
    s = s.lower()

    if s == "yes" or s == "y" or s == "true":
        return True
    elif s == "no" or s == "n" or s == "false":
        return False
    else:
        raise Error()

# Generate the standard list of word characters
s = shlex.shlex()
wordchars = s.wordchars + "/.:~"

def quote_string(s):
    safe = True
    for ch in s:
        if ch not in wordchars:
            safe = False
            break

    if not safe:
        if '"' in s:
            return "'"+s+"'"
        else:
            return '"'+s+'"'
    else:
        return s

def strip_quotes(s):
    if len(s) >= 2:
        if    (s[0] == '"' and s[-1] == '"') \
           or (s[0] == "'" and s[-1] == "'"):
            return s[1:-1]
        else:
            return s
    else:
        return s


class Config:
    def __init__(self):
        self.set_config_dir(ports.user_config_dir())
        self.mountpoint = None
        self.recordingdir = None
        self.neuros_musicdir = "MUSIC"
        self.sort_database = True
        self.oggvorbis_support = False
        self.syncdirs = []

    def set_config_dir(self, config_dir):
        """Sets the positron configuration directory.

        Use this instead of directly changing the config_dir attribute.""" 
        self.config_dir = config_dir
        self._generate_filenames()

    def _generate_filenames(self):
        self.config_filenames = {
            "config"     : path.join(self.config_dir, "config"),
            "deleted"    : path.join(self.config_dir, "deleted"),
            "recordings" : path.join(self.config_dir, "recordings") }

    def create_new_config(self):
        if not path.exists(self.config_dir):
            os.makedirs(self.config_dir)
        
        self.write_config_file()
        if not path.exists(self.config_filenames["deleted"]):
            self.clear_deleted()
        if not path.exists(self.config_filenames["recordings"]):
            self.clear_recordings()

    def supported_music_types(self):
        """Convenience method for getting the list of music types enabled
        in the config file"""

        types = ["mp3"]

        if self.oggvorbis_support:
            types.append("oggvorbis")

        return types

    # --------- Config file methods ---------

    def _read_key_value_pair(self, tokenizer):
        key = tokenizer.get_token()
        equals = tokenizer.get_token()
        value = tokenizer.get_token()

        if key != "" and equals == "=" and value != "":
            return (key, strip_quotes(value))
        else:
            raise Error(tokenizer.error_leader()
                        +"Incorrect option syntax: %s %s %s"
                        % (key, equals, value))
        
    def _read_sync_section(self, tokenizer):
        startline = tokenizer.lineno
        token1 = tokenizer.get_token()
        token2 = tokenizer.get_token()

        if token1 != "begin":
            raise Error(tokenizer.error_leader()
                        +"Sync block did not start with 'begin'")
        if token2 != "sync":
            raise Error(tokenizer.error_leader()
                        +"Sync block did not start with 'begin sync'")

        token = tokenizer.get_token()
        src = None
        dest = None
        while token != "":
            if token == "end":
                token2 = tokenizer.get_token()
                if token2 == "sync":
                    if src == None:
                        raise Error(tokenizer.error_leader()
                                    +"No src specified in sync block")
                    else:
                        self.syncdirs.append([src, dest])
                        break
                else:
                    raise Error(tokenizer.error_leader()
                                +"Must close sync block with 'end sync'")
            else:
                # Assume key/value pair
                tokenizer.push_token(token)
                (key, value) = self._read_key_value_pair(tokenizer)

                if key == "src":
                    src = path.expanduser(value)
                elif key == "dest":
                    dest = path.expanduser(value)
                else:
                    print tokenizer.error_leader() \
                          + "Ignoring unknown option %s" % (key,)

            token = tokenizer.get_token()
        else:
            raise Error(tokenizer.error_leader(line=startline)
                        +"sync block is never closed")


    def read_config_file(self):
        f = file(self.config_filenames["config"], "r")

        tokenizer = shlex.shlex(f)
        tokenizer.wordchars = wordchars 

        token = tokenizer.get_token()
        while token != "":
            if token == "begin":
                token2 = tokenizer.get_token()
                if token2 == "sync":
                    tokenizer.push_token(token2)
                    tokenizer.push_token(token)
                    self._read_sync_section(tokenizer)
                else:
                    raise Error(tokenizer.error_leader()
                                +"Unknown block '%s'" % (token2,))
            else:
                # Assume key/value pair
                tokenizer.push_token(token)
                (key, value) = self._read_key_value_pair(tokenizer)

                if key == "mountpoint":
                    self.mountpoint = path.expanduser(value)
                elif key == "recordingdir":
                    self.recordingdir = path.expanduser(value)
                elif key == "neuros_musicdir":
                    self.neuros_musicdir = value
                elif key == "sort_database":
                    try:
                        self.sort_database = parse_boolean(value)
                    except Error:
                        raise Error(tokenizer.error_leader()
                                    +"Non boolean value '%s' given for %s",
                                    (value, key))
                elif key == "oggvorbis_support":
                    try:
                        self.oggvorbis_support = parse_boolean(value)
                    except Error:
                        raise Error(tokenizer.error_leader()
                                    +"Non boolean value '%s' given for %s",
                                    (value, key))
                else:
                    print tokenizer.error_leader() \
                          + "Ignoring unknown option %s" % (key,)

            token = tokenizer.get_token()

        # Cleanup unspecified dest directories now that we've read everything
        # and know what the neuros_musicdir is
        for i in range(len(self.syncdirs)):
            if self.syncdirs[i][1] == None:
                self.syncdirs[i][1] = self.neuros_musicdir

    def write_config_file(self):
        f = file(self.config_filenames["config"], "w")

        if self.mountpoint != None and self.mountpoint != "":
            f.write("mountpoint=%s\n" % (quote_string(self.mountpoint),))
        if self.recordingdir != None and self.recordingdir != "":
            f.write("recordingdir=%s\n" % (quote_string(self.recordingdir),))
        if self.neuros_musicdir != None:
            f.write("neuros_musicdir=%s\n"
                    % (quote_string(self.neuros_musicdir),))
        if self.sort_database:
            sort_database_value = "true"
        else:
            sort_database_value = "false"            
        f.write("sort_database=%s\n" % (sort_database_value,))

        if self.oggvorbis_support:
            oggvorbis_support_value = "true"
        else:
            oggvorbis_support_value = "false"
        f.write("oggvorbis_support=%s\n" % (oggvorbis_support_value,))

        for (src,dest) in self.syncdirs:
            f.write("\nbegin sync\n")
            f.write("  src=%s\n" % (quote_string(src),))
            if dest != None:
                f.write("  dest=%s\n" % (quote_string(dest),))
            f.write("end sync\n")
        
        f.close()

    # ------- Deleted File Methods -----------

    def clear_deleted(self):
        self._clear_file(self.config_filenames["deleted"])

    def get_deleted(self):
        return self._get_list(self.config_filenames["deleted"])

    def add_deleted(self, item):
        self._add_item(self.config_filenames["deleted"], item)

    # ------- Recordings File Methods -----------

    def clear_recordings(self):
        self._clear_file(self.config_filenames["recordings"])

    def get_recordings(self):
        return self._get_list(self.config_filenames["recordings"])

    def add_recording(self, item):
        self._add_item(self.config_filenames["recordings"], item)

    # ------- Shared Deleted/Recordings Methods --------

    def _clear_file(self, filename):
        f = file(filename, "w")
        f.close()
        
    def _get_list(self, filename):
        f = file(filename, "r")
        l = f.readlines()
        f.close()

        # Eliminate null entries and trim trailing newlines
        return [trim_newline(item) for item in l if item != "" or item == "\n"]

    def _add_item(self, filename, item):
        f = file(filename, "a")
        f.write(item+"\n")
        f.close()
