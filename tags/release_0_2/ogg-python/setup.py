#!/usr/bin/env python

"""Setup script for the Ogg module distribution."""

import os
import re
import sys
import string

from distutils.core import setup
from distutils.extension import Extension

VERSION_MAJOR = 0
VERSION_MINOR = 2
pyogg_version = str(VERSION_MAJOR) + "." + str(VERSION_MINOR)

def get_setup():
    data = {}
    r = re.compile(r'(\S+)\s*?=\s*?(.+)')
    
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

_oggmodule = Extension(name='_oggmodule',
                       sources=['src/_oggmodule.c',
                                'src/pyoggpacket.c',
                                'src/pyoggstreamstate.c',
                                'src/pyoggpage.c',
                                'src/pyoggpackbuff.c',
                                'src/pyoggsyncstate.c',
                                'src/general.c'],
                       define_macros = [('VERSION_MAJOR', VERSION_MAJOR),
                                        ('VERSION_MINOR', VERSION_MINOR),
                                        ('VERSION', '"%s"' % pyogg_version)],
                       
                       include_dirs=[ogg_include_dir, 'include'],
                       library_dirs=[ogg_lib_dir],
                       libraries=['ogg'])

setup ( name = "pyogg",
        version = pyogg_version,
        description = "A wrapper for the Ogg libraries.",
        author = "Andrew Chatham",
        author_email = "andrew.chatham@duke.edu",
        url = "http://dulug.duke.edu/~andrew/pyogg",

        headers = ['include/pyogg/pyogg.h'],
        packages = ['ogg'],
        package_dir = {'ogg' : 'pysrc'},
        ext_package = 'ogg',
        ext_modules = [_oggmodule])
