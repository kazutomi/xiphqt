#!/usr/bin/env python
#
#       discoverer.py
#       
#       Copyright 2008 Joao Mesquita <jmesquita@gmail.com>
#       
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 3 of the License, or
#       (at your option) any later version.
#       
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#       
#       You should have received a copy of the GNU General Public License
#       along with this program; if not, write to the Free Software
#       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#       MA 02110-1301, USA.

import os, glob
from . import Formats

discover_funcs = []

format_path = os.path.dirname(Formats.__file__)
modules = glob.glob(os.path.join(format_path, '*.py'))
for module in modules:
    module = __import__('Formats.' + os.path.basename(module)[:-3], 
        globals(), locals(), ['discover'], 1)
    if hasattr(module, 'discover'):
        discover_funcs.append(module.discover)

def discoverer(file):
    """
        This procedure will negotiate and return the proper subtitle class to
        handle the specific format. If it returns None, format is not yet
        supported.
    """

    for func in discover_funcs:
        print "Testing this func: %s" % func
        handle = func(file)
        if handle is not None:
            print "I have found a handle: %s" % handle
            return handle(file)
    else:
        print '----'            


