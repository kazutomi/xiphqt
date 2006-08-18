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

import pygst
pygst.require('0.10')

import gst
import gobject
import os.path
from gst.extend.pygobject import gsignal
import gst.interfaces

from string import replace
from streams import Media
from streams import Stream


UNKNOWN = 0
SUCCESS = 1
FAILURE = 2
CANCELLED = 3


class DiscoverMIMEBin(gst.Pipeline):
    __gsignals__ = {
        'mime_found' : (gobject.SIGNAL_RUN_FIRST,
                        None,
                        (gobject.TYPE_BOOLEAN, ))
        }
    
    mimetype = None
    finished = False


    def __init__(self, filename):
        gobject.GObject.__init__(self)


        self.filename = filename
        self.mimetype = None

        self.finished = False
        self._success = False

        self._timeoutid = 0
        
        if not os.path.isfile(filename):
            self.finished = True
            return
        
        # the initial elements of the pipeline
        self.src = gst.element_factory_make("filesrc")
        self.src.set_property("location", filename)
        self.src.set_property("blocksize", 1000000)
        self.dbin = gst.element_factory_make("decodebin")
            
        self.add(self.src, self.dbin)
        self.src.link(self.dbin)
        self.typefind = self.dbin.get_by_name("typefind")

        # callbacks
        self.typefind.connect("have-type", self._have_type_cb)

    def _finished(self, success=False):
        self._success = success
        self.bus.remove_signal_watch()
        if self._timeoutid:
            gobject.source_remove(self._timeoutid)
            self._timeoutid = 0
        gobject.idle_add(self._stop)
        return False

    def _stop(self):
        self.finished = True
        self.set_state(gst.STATE_READY)
        self.emit('mime_found', self._success)
        

    def _bus_message_cb(self, bus, message):
        if message.type == gst.MESSAGE_EOS:
            self._finished()
        elif message.type == gst.MESSAGE_ERROR:
            self._finished()

    def discover(self):
        if self.finished:
            self.emit('mime_found', False)
            return

        self.bus = self.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect("message", self._bus_message_cb)

        # 3s timeout
        self._timeoutid = gobject.timeout_add(3000, self._finished)
        
        if not self.set_state(gst.STATE_PLAYING):
            self._finished()


    def _have_type_cb(self, typefind, prob, caps):
        self.mimetype = caps.to_string()
        self._finished()

    def getMIME(self):
        return self.mimetype

    def getSource(self):
        return self.filename

class DiscoverMIME:
    def __init__(self, file):
        self.file = file
        self.mainloop = gobject.MainLoop()
        self.current = None
    def __destroy__(self):
        if self.file:
            del self.file
        if self.mainloop:
            del self.mainloop
        if self.current:
            del self.current
    
    def run(self):
        gobject.idle_add(self._discover_one)
        self.mainloop.run()

    def _discovered(self, discoverer, ismedia):
        self.current = None

        self.SOURCE = discoverer.getSource()
        self.MIME = discoverer.getMIME()

        self.mainloop.quit()
        
    def _discover_one(self):
        if not self.file:
            gobject.idle_add(self.mainloop.quit)
            return False
        filename = self.file
        if not os.path.isfile(filename):
            gobject.idle_add(self._discover_one)
            return False
        # create a discoverer for that file
        self.current = DiscoverMIMEBin(filename)
        # connect a callback on the 'discovered' signal
        self.current.connect('mime_found', self._discovered)
        # start the discovery
        self.current.discover()
        return False

    def getMIME(self):
        return self.MIME

class MediaInfo(gst.Pipeline):

    def __init__(self, URI, FN,lastID):
        # HACK: should do Pipeline.__init__, but that doesn't do what we
        # want; there's a bug open aboooot that
        self.__gobject_init__()

        self.fromuri = URI
        self.filename = FN
        self.lastID = lastID
        self.src = self.remuxbin = self.sink = None
        self.resolution = UNKNOWN

        self.window = None

        self.media=Media()

        self._query_id = -1

    def do_setup_pipeline(self):
        self.src = gst.element_make_from_uri(gst.URI_SRC, self.fromuri)
        dMIME = DiscoverMIME(self.filename)
        dMIME.run()
        self.MIME = dMIME.getMIME()
        self.media.MIME=self.MIME
        self.media.source=self.filename
        self.media.sourceURI=self.fromuri
        if self.MIME == "application/ogg":
            self.remuxbin = OggBin(self.lastID, self.media)
        else:
            self.remuxbin = DefaultBin(self.lastID, self.media)
        self.remuxbin.connect('done', self._finished)
        self.sink = gst.element_factory_make("fakesink")
        self.resolution = UNKNOWN

        if gobject.signal_lookup('allow-overwrite', self.sink.__class__):
            self.sink.connect('allow-overwrite', lambda *x: True)

        self.add(self.src, self.remuxbin, self.sink)

        self.src.link(self.remuxbin)
        self.remuxbin.link(self.sink)

    def _start_queries(self):
        def do_query():
            try:
                # HACK: self.remuxbin.query() should do the same
                # (requires implementing a vmethod, dunno how to do that
                # although i think it's possible)
                # HACK: why does self.query_position(..) not give useful
                # answers? 
                pad = self.remuxbin.get_pad('src')
                pos, format = pad.query_position(gst.FORMAT_TIME)
                duration, format = pad.query_duration(gst.FORMAT_TIME)
                #print (pos*100.0)/duration
            except:
                pass
            return True
        if self._query_id == -1:
            self._query_id = gobject.timeout_add(100, # 10 Hz
                                                 do_query)

    def _stop_queries(self):
        if self._query_id != -1:
            gobject.source_remove(self._query_id)
            self._query_id = -1

    def _bus_watch(self, bus, message):
        if message.type == gst.MESSAGE_ERROR:
            print 'error', message
            self._stop_queries()
            m = gtk.MessageDialog(self.window,
                                  gtk.DIALOG_MODAL|gtk.DIALOG_DESTROY_WITH_PARENT,
                                  gtk.MESSAGE_ERROR,
                                  gtk.BUTTONS_CLOSE,
                                  "Error processing file")
            gerror, debug = message.parse_error()
            txt = ('There was an error processing your file: %s\n\n'
                   'Debug information:\n%s' % (gerror, debug))
            m.format_secondary_text(txt)
            m.run()
            m.destroy()
            self.response(FAILURE)
        elif message.type == gst.MESSAGE_WARNING:
            print 'warning', message
        elif message.type == gst.MESSAGE_STATE_CHANGED:
            if message.src == self:
                old, new, pending = message.parse_state_changed()
                if ((old, new, pending) ==
                    (gst.STATE_READY, gst.STATE_PAUSED,
                     gst.STATE_VOID_PENDING)):
                    self._start_queries()
                    self.set_state(gst.STATE_PLAYING)
        elif message.type == gst.MESSAGE_EOS:
            self._finished(True)

    def _finished(self, success=False):
        self.bus.remove_signal_watch()
        gobject.idle_add(self._stop)
        return False

    def _stop(self):
        self.set_state(gst.STATE_READY)
        self.response(SUCCESS)

    def response(self, response):
        assert self.resolution == UNKNOWN
        self.resolution = response
        self.set_state(gst.STATE_NULL)
        self.loop.quit()

    def start(self):
        self.do_setup_pipeline()
        self.bus = self.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message', self._bus_watch)

        self.set_state(gst.STATE_PAUSED)
        return True
        
    def run(self):
        if self.start():
            self.loop = gobject.MainLoop()
            self.loop.run()
        else:
            self.resolution = CANCELLED
        return self.resolution

    def getMedia(self):
        return self.media

class OggBin(gst.Bin):
    __gsignals__ = {
        'done' : (gobject.SIGNAL_RUN_FIRST,
                        None, ())
                        #(gobject.TYPE_BOOLEAN, ))
        }

    def __init__(self, ID, MEDIA):
        self.__gobject_init__()

        self.startID=ID
        self.media=MEDIA


        self.parsefactories = self._find_parsers()
        self.parsers = []

        self.demux = gst.element_factory_make('oggdemux')
        #self.videoparse = gst.element_factory_make("ogmvideoparse")
        self.mux = gst.element_factory_make('oggmux')

        self.add(self.demux, self.mux)#, self.videoparse)

        self.add_pad(gst.GhostPad('sink', self.demux.get_pad('sink')))
        #self.add_pad(gst.GhostPad('vsink', self.videoparse.get_pad('sink')))
        self.add_pad(gst.GhostPad('src', self.mux.get_pad('src')))

        self.demux.connect('pad-added', self._new_demuxed_pad)
        #self.videoparse.connect('pad-added', self._new_demuxed_pad)
        self.demux.connect('no-more-pads', self._no_more_pads)

    def _no_more_pads(self, elem):
        self.post_message(gst.message_new_eos(elem))
        self.emit('done')
        return False

    def _find_parsers(self):
        registry = gst.registry_get_default()
        ret = {}
        for f in registry.get_feature_list(gst.ElementFactory):
            #print "Parser: ", f.get_name(), f.get_klass()
            #if f.get_name().find('parse') >= 0:
            if f.get_klass().find('Parser') >= 0:
                for t in f.get_static_pad_templates():
                    if t.direction == gst.PAD_SINK:
                        for s in t.get_caps():
                            ret[s.get_name()] = f.get_name()
                            #print f.get_name(), s.get_name(), f.get_klass()
                        #break
        return ret

    def _pad_have_data(self, pad, buffer, udata):
        #Name = pad.get_caps().to_string()
        #LEN = len(buffer)
        #if len(buffer)>3:
        #if udata == "serial_00000004":
        #if buffer.flag_is_set(gst.BUFFER_FLAG_LAST):
        #    print "last:"
        #if (LEN>4):
            #if ("text" not in buffer[1:5]) and ("vorbis" not in buffer[1:7]):
        #        print buffer.duration, buffer.offset, buffer.timestamp, buffer.offset_end, buffer.size, udata , "\n\t", str(buffer[3:-1])
        #else:
        #        print buffer.duration, buffer.offset_end, buffer.size, udata, "\n\tstop" 
        #print buffer.timestamp, buffer.src
        return True

    def _new_demuxed_pad(self, element, pad):
        format = pad.get_caps()[0].get_name()
        #print pad.get_name(), pad.get_caps().to_string()
        if "text" in format:
            pad.add_buffer_probe(self._pad_have_data, pad.get_name())

        #print "format: ", format
        nStream=Stream()
        nStream.MIME=format
        nStream.Name=pad.get_name()
        self.startID=self.startID+1
        nStream.ID=self.startID
        self.media.addStream(nStream)
        print format
        #if format in "application/x-ogm-video":
        #    pad.link(self.videoparse.get_pad('sink'))
        if format not in self.parsefactories:
            #self.async_error("Unsupported media type: %s", format)
            return

        queue = gst.element_factory_make('queue', 'queue_' + pad.get_name())
        parser = gst.element_factory_make(self.parsefactories[format])
        self.add(queue)
        self.add(parser)
        queue.set_state(gst.STATE_PAUSED)
        parser.set_state(gst.STATE_PAUSED)
        pad.link(queue.get_compatible_pad(pad))
        queue.link(parser)
        if "text" not in format:
            parser.link(self.mux)
        self.parsers.append(parser)

class DefaultBin(gst.Bin):
    __gsignals__ = {
        'done' : (gobject.SIGNAL_RUN_FIRST,
                        None, ())
                        #(gobject.TYPE_BOOLEAN, ))
        }

    def __init__(self, ID, MEDIA):
        self.__gobject_init__()

        self.startID=ID
        self.media=MEDIA


        self.parsefactories = self._find_parsers()
        self.parsers = []

        self.demux = gst.element_factory_make('decodebin')
        self.mux = gst.element_factory_make('oggmux')

        self.add(self.demux) #, self.mux)

        self.add_pad(gst.GhostPad('sink', self.demux.get_pad('sink')))
        self.add_pad(gst.GhostPad('src', self.mux.get_pad('src')))

        self.demux.connect('pad-added', self._new_demuxed_pad)
        self.demux.connect('no-more-pads', self._no_more_pads)

    def _no_more_pads(self, elem):
        self.post_message(gst.message_new_eos(elem))
        self.emit('done')
        return False

    def _find_parsers(self):
        registry = gst.registry_get_default()
        ret = {}
        for f in registry.get_feature_list(gst.ElementFactory):
            #print "Parser: ", f.get_name(), f.get_klass()
            #if f.get_name().find('parse') >= 0:
            if f.get_klass().find('Parser') >= 0:
                for t in f.get_static_pad_templates():
                    if t.direction == gst.PAD_SINK:
                        for s in t.get_caps():
                            ret[s.get_name()] = f.get_name()
                            #print f.get_name(), s.get_name(), f.get_klass()
                        #break
        return ret

    def _pad_have_data(self, pad, buffer, udata):
        #Name = pad.get_caps().to_string()
        #LEN = len(buffer)
        #if len(buffer)>3:
        #if udata == "serial_00000004":
        #if buffer.flag_is_set(gst.BUFFER_FLAG_LAST):
        #    print "last:"
        #if (LEN>4):
            #if ("text" not in buffer[1:5]) and ("vorbis" not in buffer[1:7]):
        #        print buffer.duration, buffer.offset, buffer.timestamp, buffer.offset_end, buffer.size, udata , "\n\t", str(buffer[3:-1])
        #else:
        #        print buffer.duration, buffer.offset_end, buffer.size, udata, "\n\tstop" 
        #print buffer.timestamp, buffer.src
        return True

    def _new_demuxed_pad(self, element, pad):
        format = pad.get_caps()[0].get_name()
        #print pad.get_name(), pad.get_caps().to_string()
        #if "text" in format:
        #    pad.add_buffer_probe(self._pad_have_data, pad.get_name())

        #print "format: ", format
        nStream=Stream()
        nStream.MIME=format
        nStream.Name=pad.get_name()
        self.startID=self.startID+1
        nStream.ID=self.startID
        self.media.addStream(nStream)
        if format not in self.parsefactories:
            #self.async_error("Unsupported media type: %s", format)
            return

        #queue = gst.element_factory_make('queue', 'queue_' + pad.get_name())
        #parser = gst.element_factory_make(self.parsefactories[format])
        #self.add(queue)
        #self.add(parser)
        #queue.set_state(gst.STATE_PAUSED)
        #parser.set_state(gst.STATE_PAUSED)
        #pad.link(queue.get_compatible_pad(pad))
        #queue.link(parser)
        #if "text" not in format:
        #    parser.link(self.mux)
        #self.parsers.append(parser)
