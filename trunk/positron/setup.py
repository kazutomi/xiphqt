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

import os
from glob import glob
from os.path import isfile

from distutils.core import setup
from setupext import install_data_ext

docdirbase  = 'share/doc/positron'
manpagebase = 'share/man/man1'
docfiles    = filter(isfile, glob('doc/*.html')) + \
              filter(isfile, glob('doc/*.png')) + ['README', 'COPYING']
manpages    = ['doc/positron.1.gz']
examfiles   = filter(isfile, glob('doc/examples/*'))

setup(name="positron",
      version="1.1",
      description="A synchronization manager for the Neuros Audio Computer",
      long_description="positron is the synchronization manager for the Neuros Audio Computer.\nIt supports adding, removing, and syncing files to/from the Neuros\ndevice.",
      license="BSD",
      author="Stan Seibert",
      author_email="volsung@xiph.org",
      url="http://www.neurosaudio.com/",
      packages=['positron', 'positron.db', 'positron.db.new'],
      scripts=['scripts/positron'],
      cmdclass = {'install_data': install_data_ext},
      data_files = [('data', docdirbase, docfiles),
                    ('data', os.path.join(docdirbase, 'examples'), examfiles),
                    ('data', manpagebase, manpages)]
      )


