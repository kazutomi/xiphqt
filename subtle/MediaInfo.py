#    This file is part of Subtle
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import pygtk
pygtk.require('2.0')
import gobject
gobject.threads_init()
import pygst
pygst.require('0.10')
import gst
from gst.extend import discoverer

class Media:
    has_audio = None
    has_video = None 
    framerate = None
    sourceURI = None
    MIME = None
    videoLengthNS = None
    videoLengthS = None
    videoCaps = None
    videoWidth = None
    videoHeight = None

class MediaInfo:

    def __init__(self, file, uri):
        self.media = Media()
        self.media.source = file
        self.media.sourceURI = uri
        self.discover(file)
        self.notDone = True

    def discover(self,path):
        d = discoverer.Discoverer(path)
        d.connect('discovered',self.cb_discover)
        d.discover()

    def cb_discover(self, d, ismedia):
        if ismedia:
            self.media.MIME = d.mimetype
            if d.is_video:
                self.media.has_video = True
                self.media.framerate = float(d.videorate.num) \
                                       / float(d.videorate.denom)
                self.media.videoLengthNS = d.videolength
                self.media.videoLengthS = float(d.videolength) \
                                          / float(gst.MSECOND)/1000.0
                self.media.videoCaps = d.videocaps
                self.media.videoHeight = d.videoheight
                self.media.videoWidth = d.videowidth
            if d.is_audio:
                self.media.has_audio = True
        self.notDone = False

    def poll(self):
        return self.notDone

    def getMedia(self):
        return self.media
