#!/usr/bin/env python
# -*- Mode: python -*-
#
# positron - main frontend
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

import positron.MP3Info as MP3Info
import sys
from os import path
import time

def check_dir(arg, dirname, names):

    for name in names:
        fullname = path.join(dirname, name)
        if path.isfile(fullname):
            try:
                # Figure out if this is an MP3
                f = file(fullname)
                try:
                    mp3info = MP3Info.MP3Info(f)
                    arg.append((fullname, "mp3"))
                except MP3Info.Error:
                    arg.append((fullname, None))
                f.close()
            except Exception, e:
                # This means MP3Info has an uncaught exception and needs
                # to be fixed
                print "ERROR: %s crashed MP3Info:" % (fullname,), e

if __name__ == "__main__":

    if len(sys.argv) == 1:
        print """Tests MP3Info on a test set of files.  Assumes test set has .mp3 for true MP3
files and some other extension, or no extension, for other files.

Usage: detect_test.py dir1 dir2 dir3 ..."""

        sys.exit(0)

    starttime = time.time()
    results = []
    for dir in sys.argv[1:]:
        path.walk(dir, check_dir, results)
    endtime = time.time()
    
    # Now let's see how MP3Info did.  We assume all MP3s have the .mp3
    # extension and all non-MP3s do not.  So only use this on test sets
    # with those extensions.

    correct_mp3 = []
    incorrect_mp3 = []
    correct_other = []
    incorrect_other = []
    
    for filename, detected_type in results:
        (dummy, ext) = path.splitext(filename.lower())
        
        if ext == ".mp3" and detected_type == "mp3":
            correct_mp3.append(filename)
        elif ext == ".mp3" and detected_type != "mp3":
            incorrect_mp3.append(filename)
        elif ext != ".mp3" and detected_type != "mp3":
            correct_other.append(filename)
        elif ext != ".mp3" and detected_type == "mp3":
            incorrect_other.append(filename)


    print
    print "Results"
    print "-------"

    correct_mp3_num = len(correct_mp3)
    incorrect_mp3_num = len(incorrect_mp3)
    total_mp3_num = correct_mp3_num + incorrect_mp3_num

    if total_mp3_num == 0:
        print "No MP3s found!"
    else:
        print "MP3s correctly identified: %d/%d (%3.1f%%)" % \
              (correct_mp3_num, total_mp3_num,
               100.0 * correct_mp3_num / total_mp3_num)
        print "MP3s not identified: %d/%d" % (incorrect_mp3_num, total_mp3_num)
        if len(incorrect_mp3) > 0:
            print "Filenames:"
            for filename in incorrect_mp3:
                print " ", filename
        print

    correct_other_num = len(correct_other)
    incorrect_other_num = len(incorrect_other)
    total_other_num = correct_other_num + incorrect_other_num
    if total_other_num == 0:
        print "No other files found!"
    else:
        print "Other files correctly identified: %d/%d (%3.1f%%)" % \
              (correct_other_num, total_other_num,
               100.0 * correct_other_num / total_other_num)
        print "Other files incorrectly identified as MP3s: %d/%d" \
              % (incorrect_other_num, total_other_num)
        if len(incorrect_other) > 0:
            print "Filenames:"
            for filename in incorrect_other:
                print " ", filename

    if len(results) > 0:
        totaltime = endtime - starttime
        print "Total time spent checking files: %1.1f seconds" % (totaltime,)
        print "Rate: %1.1f files/sec" % (len(results) / totaltime)
