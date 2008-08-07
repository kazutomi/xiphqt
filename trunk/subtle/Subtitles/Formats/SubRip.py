#!/usr/bin/env python
#
#       SubRip.py
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

from .. Subtitles import Subtitles
from Subtitles.Sub import *

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
    rawstr = r"""^(?P<counter>\d+)\s*
                ^(?P<ts_from>\d{2}:\d{2}:\d{2},\d{3})\s*-->\s*(?P<ts_to>\d{2}:\d{2}:\d{2},\d{3})\r?"""
    regex = re.compile(rawstr,re.MULTILINE| re.VERBOSE)
    
    print "BEGIN: Test for SubRip"
    if regex.search(data):
        print "END: Test for SubRip"
        return SubRip
    return

class SubRip(Subtitles):
    """
        This class handles the SubRip subtitle format
    """
    
    ## Load subtitles.
    # Load subtitles from file.
    def __init__(self, filename):
        Subtitles.__init__(self,filename)
        
        # Set the file encoding
        m = magic.open(magic.MAGIC_COMPRESS | magic.MAGIC_MIME)
        status = m.load()
        self.encoding = m.file(filename).split('/')[1].split('=')[1]
        
        FILE = codecs.open(filename, 'r', self.encoding)
        DATA = FILE.read()
        FILE.close()
        
        self.subType="SubRip"

        self._subSRTLoadFromString(DATA)
        return

    ## Save subtitles.
    # Save subtitles to the file.
    # \param FN - file name.
    # \param format - the store format of subtitles. (NOT USED YET)
    def subSave(self, FN, format):
        FUN=os.open(FN,os.O_WRONLY|os.O_CREAT|os.O_TRUNC)
        N=1
        for i in self.subKeys:
            SUB = self.subs[int(i)]
            Text=str(N)+"\r\n"
            Hour, Min, Sec, MSec = self._subTime2SRTtime(SUB.start_time)
            Text+="%02d:%02d:%02d,%03d"%(Hour, Min, Sec, MSec)
            Text+=" --> "
            Hour, Min, Sec, MSec = self._subTime2SRTtime(SUB.end_time)
            Text+="%02d:%02d:%02d,%03d"%(Hour, Min, Sec, MSec)+"\r\n"
            Text+=SUB.text+"\r\n"
            if (SUB.text[-2]!="\r\n"):
                Text+="\r\n"
            os.write(FUN, Text)
            N+=1
        os.close(FUN)
        return
        
    ## Convert subtitle time to SRT format.
    # Convert subtitle time for saving in SRT subtitles file.
    # \param time - subtitle time.
    # \return list of: hour, minute, second and milisecond
    def _subTime2SRTtime(self, time):
        tTime = time
        MSec = tTime%1000
        tTime /=1000
        Sec = tTime%60
        tTime /= 60
        Min = tTime%60
        Hour = tTime/60
        return Hour, Min, Sec, MSec

    ## Load SRT formated subtitles.
    # Load SRT formated subtitles from given string.
    # \param DATA - string of SRT subtitles.
    def _subSRTLoadFromString(self, DATA):
        num_sub = 0
        if (string.find(DATA, "\r\n")==-1):
            DATA=string.split(DATA,"\n")
        else:
            DATA=string.split(DATA,"\r\n")
        i=0
        while(i<len(DATA)):
            if(i>=len(DATA)):
                break
            N = DATA[i]
            i+=1
            if(i>=len(DATA)):
                break
            Timing = DATA[i]
            Text="";
            i+=1
            if(i>=len(DATA)):
                break
            while(DATA[i]!=""):
                Text=Text+DATA[i]+"\n"
                i+=1
            i+=1
            Text=Text[0:-1]
            ST=int(Timing[0:2])*3600000+int(Timing[3:5])*60000+int(Timing[6:8])*1000+int(Timing[9:12])
            ET=int(Timing[17:19])*3600000+int(Timing[20:22])*60000+int(Timing[23:25])*1000+int(Timing[26:29])
            
            TS=Sub(Text)
            num_sub += 1
            #TS.text=Text
            TS.start_time=ST
            TS.end_time=ET
            TS.start_frame=ST*FRAMERATE/1000
            TS.end_frame=ET*FRAMERATE/1000
            TS.number = num_sub
            self.subs[int(ST)]=TS
        self.updateKeys()
