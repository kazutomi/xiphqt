#!/usr/bin/env python

"""Setup script for the Ao module distribution.
Configuration in particular could use some work."""

import os, sys
from distutils.core import setup
from distutils.extension import Extension
from distutils.command.config import config
from distutils.command.build import build

def load_config():
    '''This is still a bit creaky. Hopefully distutils will
    offer better configuration support in the future.'''

    if not os.path.isfile('config.ao'):
        print "File config.ao not found"
        return {}
    f = open('config.ao', 'r')
    dict = eval(f.read())
    return dict


class config_ao (config):

    added_variables = ('ao_include_dirs', 'ao_library_dirs', 'ao_libraries')

    user_options = config.user_options + [
        ('ao-prefix=', None,
         'prefix in which to find ao headers and libraries'),
        ('ao-include-dirs=', None,
         "directories to search for ao header files"),
        ('ao-library-dirs=', None,
         "directories to search for ao library files"),
        ]

    def _save(self):
        '''Save the variables I want as the representation of a dictionary'''
        
        dict = {}
        for v in self.added_variables:
            dict[v] = getattr(self, v)
        f = open('config.ao', 'w')
        f.write(repr(dict))
        f.write('\n')

    def initialize_options (self):
        config.initialize_options(self)
        self.ao_prefix = '/usr/local'
        self.ao_include_dirs = []
        self.ao_library_dirs = []
        self.ao_libraries = ['ao', 'dl']


    def finalize_options (self):
        if not self.ao_include_dirs:
            self.ao_include_dirs = [os.path.join(self.ao_prefix, 'include')]
        if not self.ao_library_dirs:
            self.ao_library_dirs = [os.path.join(self.ao_prefix, 'lib')]
                
    def run (self):
        self.have_ao = self.check_lib("ao", self.ao_library_dirs,
                                 ['ao/ao.h'], self.ao_include_dirs, ['dl'])

        if not self.have_ao:
            print "*** ao check failed ***"
            print "You may need to install the ao library"
            print "or pass the paths where it can be found"
            print "(setup.py --help)"
            sys.exit(1)

        self._save()


class nullBuilder (build):
    '''Prevents building. This is used for when they try to build
    without having run configure first.'''

    def run(self):
        print
        print "*** You must first run 'setup.py config' ***"
        print
        sys.exit(1)

cmdclass = {'config' : config_ao}

config_data = load_config()
if not config_data:
    cmdclass['build'] = nullBuilder
    ao_include_dirs = ao_library_dirs = ao_libraries = []
else:
    ao_include_dirs = config_data['ao_include_dirs']
    ao_library_dirs = config_data['ao_library_dirs']
    ao_libraries = config_data['ao_libraries']

setup (# Distribution meta-data
        name = "pyao",
        version = "0.0.2",
        description = "A wrapper for the ao library",
        author = "Andrew Chatham",
        author_email = "andrew.chatham@duke.edu",
        url = "http://dulug.duke.edu/~andrew/pyvorbis.html",

        cmdclass = cmdclass,

        # Description of the modules and packages in the distribution

        ext_modules = [Extension(
                name = 'aomodule',
                sources = ['src/aomodule.c'],
                include_dirs = ao_include_dirs,
		library_dirs = ao_library_dirs,
                libraries = ao_libraries)]
)


