#!/usr/bin/env python
# -*- Mode: python -*-
#
# detect_test.py - tests the accuracy of positron.audiofile.detect()
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

import positron.audiofile
import sys
from os import path
import time

# Mappings from audio types to extensions
type_mapping = { "mp3" : ".mp3",
                  "oggvorbis" : ".ogg"}
ext_mapping = { ".mp3" : "mp3",
                 ".ogg" : "oggvorbis"}
type_names = {"mp3" : "MP3",
              "oggvorbis" : "Ogg Vorbis"}


def check_dir(arg, dirname, names):

    for name in names:
        fullname = path.join(dirname, name)
        if path.isfile(fullname):
            try:
                metadata = positron.audiofile.detect(fullname)
                if metadata != None:
                    arg.append((fullname, metadata["type"]))
                else:
                    arg.append((fullname, None))
            except Exception, e:
                # This means detect() has an uncaught exception and needs
                # to be fixed
                print "ERROR: %s crashed detect():" % (fullname,), e

def print_results(name, correct, incorrect):

    correct_num = len(correct)
    incorrect_num = len(incorrect)
    total_num = correct_num + incorrect_num
    
    if total_num == 0:
        print "No %s files found!" % (name,)
    else:
        print "%s files correctly identified: %d/%d (%3.1f%%)" % \
              (name, correct_num, total_num,
               100.0 * correct_num / total_num)
        print "%s files not identified: %d/%d" % (name, incorrect_num,
                                                  total_num)
        if len(incorrect) > 0:
            print "Filenames:"
            for filename,detected_type in incorrect:
                print "  (%s) %s" % (detected_type, filename)
        print


if __name__ == "__main__":

    if len(sys.argv) == 1:
        print """Tests positron.audiofile.detect() on a test set of files.

Assumes test set has .mp3 for true MP3, .ogg for Ogg Vorbis files and some
other extension, or no extension, for other files.

Usage: detect_test.py dir1 dir2 dir3 ..."""

        sys.exit(0)

    starttime = time.time()
    files = []
    for dir in sys.argv[1:]:
        path.walk(dir, check_dir, files)
    endtime = time.time()


    # None is for the results of non-music files
    results = {}
    for key in type_mapping.keys()+[None]:
        results[key] = [[], []]  # Correctly IDed, Incorrectly IDed

    for filename, detected_type in files:
        (dummy, ext) = path.splitext(filename.lower())

        if detected_type != None:            
            if ext == type_mapping[detected_type]:
                results[detected_type][0].append(filename)
            elif ext in ext_mapping.keys():
                results[ext_mapping[ext]][1].append((filename, detected_type))
            else:
                results[None][1].append((filename, detected_type))
        else:
            if ext not in ext_mapping.keys():
                results[None][0].append(filename)
            else:
                results[ext_mapping[ext]][1].append((filename, detected_type))

    # Print results
    print
    print "Results"
    print "-------"


    keys = results.keys()
    keys.sort()
    keys.reverse()

    for filetype in keys:

        if filetype == None:
            name = "Other"
        else:
            name = type_names[filetype]
            
        print_results(name, results[filetype][0], results[filetype][1])
        

    if len(files) > 0:
        totaltime = endtime - starttime
        print
        print "Total time spent checking files: %1.1f seconds" % (totaltime,)
        print "Rate: %1.1f files/sec" % (len(files) / totaltime)
