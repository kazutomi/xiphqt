# -*- Mode: python -*-
#
# cmd_clear.py - clear command
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

"""positron clear:\tClears all entries from databases

  positron clear

     Clears all databases.

  positron clear <database1> <database2> ...

     Clears all entries from a particular database.  Valid database names
     are: audio, pcaudio, unidedhisi, idedhisi, failedhisi

Note that the clear command never removes files.
"""

from neuros import Neuros
import neuros as neuros_module
import util

def run(config, neuros, args):
    if len(args) == 0:
        args = ["audio", "unidedhisi", "idedhisi", "failedhisi"]

    for arg in args:
        try:
            database = neuros.open_db(arg)
            database.clear()
            neuros.close_db(arg)
            print "Database \"%s\" cleared." % (arg,)
        except neuros_module.Error, e:
            print "Error:", e
    
