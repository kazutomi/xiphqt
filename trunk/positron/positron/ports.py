# -*- Mode: python -*-
#
# ports.py - os specific stuff
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
import os
from os import path
from util import trim_newline

# Someday we'll stick OS specific information into this file

def user_config_dir():
    return path.join(path.expanduser("~"),".positron")

def filesystem_size(pathname):
    """Returns the size of the filesystem where pathname is stored in bytes.

    Returns None, if the size cannot be found for whatever reason."""
    
    if sys.platform[:3] == "win":
        size = None
    else:
        try:
            stat = os.statvfs(pathname)
            size = stat.f_bsize * stat.f_blocks
        except Exception:
            pass
    
    return size
