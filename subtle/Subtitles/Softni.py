#!/usr/bin/env python
#
#       Softni.py
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


# This implementation is on real BAD alpha stage and should not be
# considered by any chance ready for production.
# A lot of study is still needed to transform the frames into timestamp
# since this format is a bit funky.

import os
import string
import re
import codecs

# This is not the best option since we rely on Linux-only
# Make use of file command to check on the file type
try:
    import magic
except:
    print "We need python-magic, otherwise, this format will not be \
    supported"
    sys.exit(1)

from random import randint

from Subtitles import Subtitles
from Sub import *

FRAMERATE=25.00

def discover(file):
    """
        Every subtitle should have a discover function
        and return true if it should handle the requested
        file.
    """

    m = magic.open(magic.MAGIC_COMPRESS | magic.MAGIC_MIME)
    status = m.load()

    if m.file(file).split('/')[0] == "text":
        # Open file and read it
        fd = open(file, "r")
        data = fd.read()
        fd.close()
    else:
        return

    # Test for SubRip by matching the header
    rawstr = r"""^(?P<sub>.*\r?\n)*?
            ^(?P<ts_from>\d{2}:\d{2}:\d{2}.\d{2})\\(?P<ts_to>\d{2}:\d{2}:\d{2}.\d{2})"""

    regex = re.compile(rawstr,  re.MULTILINE| re.VERBOSE)

    if regex.search(data):
        return True
    return
    
class Softni(Subtitles):
    """
        This class handles the Softni file format
    """
    def __init__(self, filename):
        Subtitles.__init__(self,filename)
        
        # Set the file encoding
        m = magic.open(magic.MAGIC_COMPRESS | magic.MAGIC_MIME)
        status = m.load()
        self.encoding = m.file(filename).split('/')[1].split('=')[1]
        
        self.subType="Softni"

        self._loadFromFile(filename)
        return

    def _loadFromFile(self, file):
        """
            Parse and load the subtitle using a string
            as input
        """
        regex = re.compile(r"""^(?P<ts_from>\d{2}:\d{2}:\d{2}.\d{2})\\(?P<ts_to>\d{2}:\d{2}:\d{2}.\d{2})""", re.MULTILINE)
  
        # We reopen the file here so we can
        # iterate over the lines
        fd = codecs.open(file, "r", self.encoding)
        str = fd.readlines()
        fd.close()
        
        # Lets set the data structure like we need it
        info = []
        buffer = ""
        for line in str:
            if regex.search(line):
                info.append(tuple([buffer] + line.split('\\')))
                buffer=""
            else:
                buffer+=line
        
        # Iterate all the subs and create the
        # sub objects
        sub_count = 0
        for sub in info:
            text = sub[0]
            stime = sub[1]
            etime = sub[2]
            TS = Sub(text)
            TS.start_time = self._softniFormat2Timestamp(stime)
            TS.end_time = self._softniFormat2Timestamp(etime)
            TS.start_frame = self._softniFormat2Frame(stime)
            TS.end_frame = self._softniFormat2Frame(etime)
            TS.number = sub_count
            sub_count += 1
            self.subs[int(self._softniFormat2Timestamp(stime))]=TS
        self.updateKeys()
        return
    
    def _softniFormat2Frame(self, softniFormat):
        """
            Convert Softni frame format to cumulative frame counting
        """
        frames = ((float(softniFormat[0:2])*60*60) + \
                (float(softniFormat[3:5])*60) + \
                float(softniFormat[6:8]) * FRAMERATE) + \
                float(softniFormat[9:11])
        return frames
        
    def _softniFormat2Timestamp(self, softniFormat):
        """
            Convert Softni frame format to cumulative frame counting
        """ 
        timestamp = (float(softniFormat[0:2])*60*60) + \
                (float(softniFormat[3:5])*60) + \
                float(softniFormat[6:8]) + \
                (float(softniFormat[9:11])/FRAMERATE)
        return timestamp*1000
