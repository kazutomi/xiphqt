#!/usr/bin/env python
#
#       Subtitles.py
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

import os
import string

# Subtle imports
from Sub import *

class Subtitles:
    """
        This class defines all the interface for the application to handle
        subtitles and to ease the implementation of new formats
    """
    
    def __init__(self,FN):
        """
            Initialize all the attributes needed to handle
            all types of subtitle formats as well as their manipulation
        """
        self.subs={}
        self.subKeys=[]
        self.filename = FN
        # TODO: Support more subtitles types
        self.subType = None
        self.encoding = None
        self.framerate = None
        return

    ## Delete subtitle.
    # Delete subtitle from subtitles array.
    # \param time - key of subtitle in "subs" list.
    def subDel(self, time):
        del self.subs[time]
        self.updateKeys()
        
    ## Add subtitle.
    # Add subtitle to the "subs" list.
    # \param STime - start time of the subtitle.
    # \param ETime - end time of the subtitle.
    # \param Attrs - attributes of the subtitle.
    # \param isUpdate - to update (or not) keys array of "subs" list.
    def subAdd(self, STime, ETime, Text, Attrs, isUpdate=0):
        TS=Sub()
        TS.text=Text
        TS.start_time=STime
        TS.end_time=ETime
        TS.Attributes=Attrs
        self.subs[int(STime)]=TS
        if isUpdate==1:
            self.updateKeys()

    ## Update keys array.
    # Update array of "subs" keys.
    def updateKeys(self):
        self.subKeys=self.subs.keys()
        self.subKeys.sort()

    ## Update sub text.
    # Update text for sub.
    def updateText(self, key, text):
        if key in self.subs.keys():
            self.subs[key].text = text
        else:
            print "Subkey %s not found" % key

    ## Update subtitle.
    # Update subtitle key.
    # \param upSubKey - subtitle to update.
    def subUpdate(self, upSubKey):
        Sub = self.subs[upSubKey]
        self.subDel(upSubKey)
        self.subAdd(Sub.start_time, Sub.end_time, Sub.text, Sub.Attributes, 1)

    ## Get subtitle.
    # Get subtitle with given time of visibility.
    # \param time - time of requested subtitle.
    # \return subtitle or "None".
    def getSub(self, time):
        i=0
        for i in self.subKeys:
            if(time>=i):
                if(self.subs[i].isInTime(time)==1):
                    return self.subs[i]
            else:
                return None
        return None


    ## Get subtitle supported types.
    # Get subtitle supported types
    # \return supported subtitle types 
    def getSupportedTypes(self):
        return [".srt"]
