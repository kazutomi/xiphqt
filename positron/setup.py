#!/usr/bin/env python
# -*- Mode: python -*-
#
# setup.py - distutils setup script
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

from distutils.core import setup

setup(name="positron",
      version="1.0b1",
      #summary="Positron synchronization software for the Neuros Audio Computer",
      #platform=["POSIX"],
      #license="BSD",
      author="Stan Seibert",
      author_email="volsung@xiph.org",
      url="http://www.neurosaudio.com/",
      packages=['positron', 'positron.db', 'positron.db.new'],
      scripts=['scripts/positron']
      )


