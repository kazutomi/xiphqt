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

import os
from os import path

# Someday we'll stick OS specific information into this file

def user_config_dir():
        return path.join(path.expanduser("~"),".positron")
