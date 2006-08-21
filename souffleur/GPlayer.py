# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import pygtk
pygtk.require('2.0')

import gobject
gobject.threads_init()

import pygst
pygst.require('0.10')
import gst
import gst.interfaces
import gtk

## \file GPlayer.py
# Documentation for GPlayer module of Souffleur project.
# \todo Add better seeking.


## GstPlayer class.
# Class for playing media in GStreamer.
class GstPlayer:
    ## Construstor
    # \param videowidget - VideoWidget class.
    def __init__(self, videowidget):
        self.playing = False
        self.player = gst.element_factory_make("playbin", "player")
        self.videowidget = videowidget

        bus = self.player.get_bus()
        bus.enable_sync_message_emission()
        bus.add_signal_watch()
        bus.connect('sync-message::element', self.on_sync_message)
        bus.connect('message', self.on_message)

    ## \var playing
    # Bool variable, TRUE - if media is playing.
    
    ## \var player
    # GStreamer playerbin element.
    
    ## \var videowidget
    # GTK+ widget for video render.
    
#==============================================================================
    def on_sync_message(self, bus, message):
        if message.structure is None:
            return
        if message.structure.get_name() == 'prepare-xwindow-id':
            self.videowidget.set_sink(message.src)
            message.src.set_property('force-aspect-ratio', True)

#==============================================================================
    def on_message(self, bus, message):
        t = message.type
        if t == gst.MESSAGE_ERROR:
            err, debug = message.parse_error()
            print "Error: %s" % err, debug
            self.playing = False
        elif t == gst.MESSAGE_EOS:
            self.playing = False

#==============================================================================
    ## Set location.
    # Set location of the source.
    # \param location - URI of the source.
    def set_location(self, location):
        self.player.set_state(gst.STATE_NULL)
        self.player.set_property('uri', location)

#==============================================================================
    ## Get location.
    # Get location of the source.
    def get_location(self):
        return self.player.get_property('uri')

#==============================================================================
    def query_position(self):
        "Returns a (position, duration) tuple"
        try:
            position, format = self.player.query_position(gst.FORMAT_TIME)
        except:
            position = gst.CLOCK_TIME_NONE

        try:
            duration, format = self.player.query_duration(gst.FORMAT_TIME)
        except:
            duration = gst.CLOCK_TIME_NONE

        return (position, duration)

#==============================================================================
    ## Seek.
    # Seek media.
    # \param location - location to the seek.
    def seek(self, location):
        gst.debug("seeking to %r" % location)
        event = gst.event_new_seek(1.0, gst.FORMAT_TIME,
            gst.SEEK_FLAG_FLUSH,
            gst.SEEK_TYPE_SET, location,
            gst.SEEK_TYPE_NONE, 0)

        res = self.player.send_event(event)
        if res:
            gst.info("setting new stream time to 0")
            self.player.set_new_stream_time(0L)
        else:
            gst.error("seek to %r failed" % location)

#==============================================================================
    ## Pause.
    # Media pause.
    def pause(self):
        gst.info("pausing player")
        self.player.set_state(gst.STATE_PAUSED)
        self.playing = False

#==============================================================================
    ## Play.
    # Media play.
    def play(self):
        gst.info("playing player")
        self.player.set_state(gst.STATE_PLAYING)
        self.playing = True

#==============================================================================
    ## Stop
    # Media stop.
    def stop(self):
        self.player.set_state(gst.STATE_NULL)
        self.playing = False
        gst.info("stopped player")

#==============================================================================
    ## Get state.
    # Get current state of the media.
    # \param timeout - time out of the operation.
    # \raturn state of the media.
    def get_state(self, timeout=1):
        return self.player.get_state(timeout=timeout)

#==============================================================================
    ## Is playing
    # \return TRUE if media is playing.
    def is_playing(self):
        return self.playing

#==============================================================================
## VideoWidget class.
# VideoWidget class for render video stream on GTK+ widget.
class VideoWidget:
    ## Constructor.
    # \param TArea - GTK+ drowing area widget.
    def __init__(self, TArea):
        self.Area=TArea
        self.imagesink = None
        self.Area.unset_flags(gtk.DOUBLE_BUFFERED)

    ## \var Area
    # GTK+ drowing area widget.
    
    ## \var imagesink
    # Sink element for 

    def do_expose_event(self, event):
        if self.imagesink:
            self.imagesink.expose()
            return False
        else:
            return True

    def set_sink(self, sink):
        assert self.Area.window.xid
        self.imagesink = sink
        self.imagesink.set_xwindow_id(self.Area.window.xid)
