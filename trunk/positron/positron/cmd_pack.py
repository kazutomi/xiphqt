# -*- Mode: python -*-
#
# cmd_pack.py - pack command
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

"""positron pack:\tRemoves unused space from databases

  positron pack

     Packs all databases.

  positron pack <database1> <database2> ...

     Packs all entries from a particular database.  Valid database names
     are: audio, pcaudio, unidedhisi, idedhisi, failedhisi
"""

from neuros import Neuros
import neuros as neuros_module
import util
from os import path
    
def run(config, neuros, args):
    if len(args) == 0:
        args = ["audio", "pcaudio", "unidedhisi", "idedhisi", "failedhisi"]

    for arg in args:
        try:
            database = neuros.open_db(arg)
            print "  Packing database \"%s\"..." % (arg,)
            database.pack()

            if config.sort_database:
                database.sort(path.join(*neuros.mountpoint_parts +
                                        [neuros.DB_DIR, 'tracks.txt']))
                
            neuros.close_db(arg)
        except neuros_module.Error, e:
            print "Error:", e

    print
    print "Done!"
