#!/usr/bin/env python

"""Setup script for the Ao module distribution."""

import os
from distutils.core import setup
from distutils.extension import Extension

AO_INCLUDE_DIR = '/usr/local/include'
AO_LIB_DIR = '/usr/local/lib'

setup (# Distribution meta-data
        name = "pyao",
        version = "0.0.2",
        description = "A wrapper for the ao library",
        author = "Andrew Chatham",
        author_email = "andrew.chatham@duke.edu",
        url = "http://dulug.duke.edu/~andrew/pyvorbis.html",

        # Description of the modules and packages in the distribution

        ext_modules = [Extension(
                name='aomodule',
                sources=['src/aomodule.c'],
                include_dirs=[AO_INCLUDE_DIR],
		library_dirs=[AO_LIB_DIR],
                libraries=['ao'])]
)

