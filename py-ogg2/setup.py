#!/usr/bin/env python

"""Setup script for the Ogg module distribution."""

import os
import re
import sys
import string

from distutils.core import setup
from distutils.extension import Extension

VERSION_MAJOR = 2
VERSION_MINOR = 0
pyogg_version = str(VERSION_MAJOR) + "." + str(VERSION_MINOR)

def get_setup():
    data = {}
    r = re.compile(r'(\S+)\s*?=\s*(.+)')
    
    if not os.path.isfile('Setup'):
        print "No 'Setup' file. Perhaps you need to run the configure script."
        sys.exit(1)

    f = open('Setup', 'r')
    
    for line in f.readlines():
        m = r.search(line)
        if not m:
            print "Error in setup file:", line
            sys.exit(1)
        key = m.group(1)
        val = m.group(2)
        data[key] = val
        
    return data

data = get_setup()
ogg_include_dir = data['ogg_include_dir'] 
ogg_lib_dir = data['ogg_lib_dir']
ogg_libs = string.split(data['ogg_libs'])

_ogg2module = Extension(name='ogg2',
                       sources=['src/module.c',
                                'src/packet.c',
                                'src/streamstate.c',
                                'src/page.c',
                                'src/packbuff.c',
                                'src/syncstate.c',
                                'src/general.c'],
                       define_macros = [('VERSION_MAJOR', VERSION_MAJOR),
                                        ('VERSION_MINOR', VERSION_MINOR),
                                        ('VERSION', '"%s"' % pyogg_version)],
                       
                       include_dirs=[ogg_include_dir, 'include'],
                       library_dirs=[ogg_lib_dir],
                       libraries=ogg_libs)

setup ( name = "pyogg",
        version = pyogg_version,
        description = "A wrapper for the Ogg libraries.",
        author = "Arc Riley",
        author_email = "arc@xiph.org",
        url = "NONEYET",

        headers = [],
        packages = ['ogg2'],
        package_dir = {'ogg2' : 'pysrc'},
        ext_package = 'ogg2',
        ext_modules = [_ogg2module] )
