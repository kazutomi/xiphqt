#!/usr/bin/env python
#
#       Sub.py
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
import string

from Line import *

class Sub:
    """
        The Sub class, is the class that handles each subtitle 
        individually
    """

    def __init__(self,text):
        """
            Init all the variables
        """
        self.lines = []
        # Start with 1 cos we are only called
        # when there is at least one line
        self.nLines = 1
        self.start_time=None
        self.start_frame=0
        self.end_time=None
        self.end_frame=0
        self.number=None
        self._processText(text)

    def isInTime(self, time):
        """
            Is it time to display a subtitle?
        """
        if( (time>=self.start_time) and (time<=self.end_time) ):
            return 1
        else:
            return 0

    def _processText(self,text):
        """
            We should parse the full text of a subtitle and divide it
            line by line.
            Another getSub method exists to retrieve the full text
        """
        lines = text.splitlines(True)
        self.nLines = len(lines)
        for i in xrange(0, len(lines)):
            self.lines.append( Line(lines[i]) )
        return
    
    def getSubText(self):
        """
            Retrieve the full subtitle text.
            The data model is yet to be defined.
        """
        fullText = ''
        for i in range(0, self.nLines):
            fullText += self.lines[i].text
        return fullText
        
    def setSubText(self, text):
        """
            Set the subtitle text and this method will rearrange the
            structure of lines as well as all other attributes.
        """
        pass
