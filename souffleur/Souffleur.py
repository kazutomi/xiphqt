#!/usr/bin/env python

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


#import oggStreams
#from gstfile import GstFile
from GPlayer import VideoWidget
from GPlayer import GstPlayer
from Subtitles import Subtitles
#from datetime import time
import sys

from streams import Media
from streams import Stream
from MediaInfo import MediaInfo
from SouffleurXML import ProjectXML

try:
    import pygtk
    #tell pyGTK, if possible, that we want GTKv2
    pygtk.require("2.0")
except:
    #Some distributions come with GTK2, but not pyGTK
    pass
try:
    import gtk
    import gobject
    import gtk.glade
except:
    print "You need to install pyGTK or GTKv2 ",
    print "or set your PYTHONPATH correctly."
    print "try: export PYTHONPATH=",
    print "/usr/local/lib/python2.2/site-packages/"
    sys.exit(1)
#now we have both gtk and gtk.glade imported
#Also, we know we are running GTK v2
import gst

class Souffleur:
#    gladefile=""
    def __init__(self):
        """
        In this init we are going to display the main
        Souffleur window
        """
        gladefile="souffleur.glade"
        windowname="MAIN_WINDOW"
        
        self.update_id = -1
        self.p_position = gst.CLOCK_TIME_NONE
        self.p_duration = gst.CLOCK_TIME_NONE
        self.UPDATE_INTERVAL=100
        
        self.Subtitle = None
        self.Subtitles = []
        self.curSub = -1
        self.scroll = 0
        self.videoWidgetGst = None
        self.player = None
        self.t_duration = 0
        
        self.media = []
        self.lastID=0
        #self.videoWidget=VideoWidget();
        #gtk.glade.set_custom_handler(self.videoWidget, VideoWidget())

        #gtk.glade.set_custom_handler(self.custom_handler)
        self.wTree=gtk.glade.XML (gladefile,windowname)
        self.gladefile = gladefile
        # we only have two callbacks to register, but
        # you could register any number, or use a
        # special class that automatically
        # registers all callbacks. If you wanted to pass
        # an argument, you would use a tuple like this:
        # dic = { "on button1_clicked" : (self.button1_clicked, arg1,arg2) , ...
        #dic = { "on_button1_clicked" : self.button1_clicked, \
        #	"gtk_main_quit" : (gtk.mainquit) }
        dic = { "gtk_main_quit" : (gtk.main_quit),\
            "on_main_file_quit_activate": (gtk.main_quit), \
            "on_main_file_open_activate": self.mainFileOpen, \
            "on_TOOL_PLAY_clicked": self.playerPlay,\
            "on_TOOL_STOP_clicked": self.playerStop,\
            "on_MEDIA_ADJUSTMENT_button_press_event": self.buttonPressAdjustment,\
            "on_MEDIA_ADJUSTMENT_button_release_event": self.buttonReleaseAdjustment,\
            "on_MEDIA_ADJUSTMENT_change_value": self.changeValueAdjustment,\
            "on_VIDEO_OUT_PUT_expose_event": self.exposeEventVideoOut,\
            "on_TOOL_START_clicked": self.cb_setSubStartTime,\
            "on_TOOL_END_clicked": self.cb_setSubEndTime,\
            "on_TOOL_SAVE_clicked": self.cb_subChangeSave,\
            "on_TOOL_DELETE_clicked": self.cb_subDel,\
            "on_main_file_save_activate": self.cb_onSaveMenu,\
            "on_main_file_save_as_activate": self.cb_onSaveAsMenu,\
            "on_main_file_new_activate": self.cb_onNewMenu,\
            "on_LIST_SUBS_cursor_changed": self.cb_onSubsListSelect}
        self.wTree.signal_autoconnect (dic)
        
        self.windowProjectOpen=None
        self.windowProjectSO=None
        self.PFileName=None
        self.windowMediaOpen=None
        self.windowStreams=gtk.glade.XML (self.gladefile,"STREAM_WINDOW")
        dic = {"on_TOOL_DEL_STREAM_clicked": self.cb_delStream,\
                "on_TOOL_MOD_STREAM_clicked": self.cb_modStream,\
                "on_TOOL_SAVE_STREAM_clicked": self.cb_saveStream,\
                "on_TOOL_ADD_STREAM_clicked": self.cb_addNewStream}
        self.windowStreams.signal_autoconnect (dic)
        ### Setup LIST_STREAMS
        LIST = self.windowStreams.get_widget("LIST_STREAMS")
        if LIST:
            self.streamsTreeStore = gtk.TreeStore(gobject.TYPE_STRING, gobject.TYPE_UINT)
            LIST.set_model(self.streamsTreeStore)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('Streams', cell, text = 0)
            LIST.append_column(tvcolumn)
        
        self.windowSubsList=gtk.glade.XML (self.gladefile,"SUBS_LIST")
        dic = {"on_LIST_SUBS_cursor_changed": self.cb_onSubsListSelect}
        self.windowSubsList.signal_autoconnect (dic)
        SUBLIST = self.windowSubsList.get_widget("LIST_SUBS")
        if SUBLIST:
            self.subsListStore = gtk.ListStore(gobject.TYPE_UINT,
                                                gobject.TYPE_UINT,
                                                gobject.TYPE_STRING)
            SUBLIST.set_model(self.subsListStore)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('Start', cell, text = 0)
            SUBLIST.append_column(tvcolumn)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('End', cell, text = 1)
            SUBLIST.append_column(tvcolumn)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('Text', cell, text = 2)
            SUBLIST.append_column(tvcolumn)
        WND=self.windowStreams.get_widget("STREAM_WINDOW")
        WND.hide()
        WND=self.windowSubsList.get_widget("SUBS_LIST")
        WND.hide()
        ### Main window setup
        self.videoWidget = self.wTree.get_widget("VIDEO_OUT_PUT")
        self.adjustment = self.wTree.get_widget("MEDIA_ADJUSTMENT")
        self.SubEdit = self.wTree.get_widget("VIEW_SUB")
        self.labelHour = self.wTree.get_widget("LABEL_HOUR")
        self.labelMin = self.wTree.get_widget("LABEL_MIN")
        self.labelSec = self.wTree.get_widget("LABEL_SEC")
        self.labelMSec = self.wTree.get_widget("LABEL_MSEC")
        self.subStartTime = self.wTree.get_widget("SUB_START_TIME")
        self.subEndTime = self.wTree.get_widget("SUB_END_TIME")
        self.playButton = self.wTree.get_widget("TOOL_PLAY")
        return
#==============================================================================
    def getSubtitle(self, source):
        for i in self.Subtitles:
            if i.subSource==source:
                return i
        return None
#==============================================================================
    def cb_saveStream(self, widget):
        if not self.windowStreams:
            return
        if not self.streamsTreeStore:
            return
        TView = self.windowStreams.get_widget("LIST_STREAMS")
        TSelec = TView.get_selection()
        TModel, TIter = TSelec.get_selected()
        if not TIter:
            return
        N=TModel.get_value(TIter, 1)
        mInfo = self.media[N]
        if "subtitle" in mInfo.MIME:
            tSubtitle = self.getSubtitle(mInfo.Streams[0].ID)
            tSubtitle.subSave(mInfo.source, 1)
#==============================================================================
    def cb_modStream(self, widget):
        if not self.windowStreams:
            return
        if not self.streamsTreeStore:
            return
        TView = self.windowStreams.get_widget("LIST_STREAMS")
        TSelec = TView.get_selection()
        TModel, TIter = TSelec.get_selected()
        if not TIter:
            return
        N=TModel.get_value(TIter, 1)
        mInfo = self.media[N]
        if "subtitle" in mInfo.MIME:
            self.setSubtitle(mInfo.Streams[0].ID)
#==============================================================================
    def setSubtitle(self, source):
        for i in self.Subtitles:
            if i.subSource==source:
                self.Subtitle=i
                break
        if self.Subtitle:
            if (self.windowStreams):
                WND=self.windowSubsList.get_widget("SUBS_LIST")
                WND.show()
            self.subsWindowUpdate()
#==============================================================================
    def updateStreamWindow(self):
        if not self.streamsTreeStore:
            return
        self.streamsTreeStore.clear()
        for mInfo in self.media:
            iter = self.streamsTreeStore.append(None)
            self.streamsTreeStore.set(iter, 0, mInfo.MIME + " ("+mInfo.source+")", 1, self.media.index(mInfo))
            for i in mInfo.Streams:
                child = self.streamsTreeStore.append(iter)
                self.streamsTreeStore.set(child, 0, i.MIME + " ("+i.Name+")", 1, self.media.index(mInfo))
#==============================================================================
    def cb_delStream(self, widget):
        if not self.windowStreams:
            return
        if not self.streamsTreeStore:
            return
        TView = self.windowStreams.get_widget("LIST_STREAMS")
        TSelec = TView.get_selection()
        TModel, TIter = TSelec.get_selected()
        if not TIter:
            return
        N=TModel.get_value(TIter, 1)
        del self.media[N]
        self.updateStreamWindow()
#==============================================================================
    def cb_openMediaCancel(self, widget):
        if self.windowMediaOpen:
            WND=self.windowMediaOpen.get_widget("OPEN_MEDIA")
            WND.hide()
#==============================================================================
    def cb_openMediaOpen(self, widget):
        WND=self.windowMediaOpen.get_widget("OPEN_MEDIA")
        FN=WND.get_filename()
        URI=WND.get_uri()
        WND.hide()
        MI = MediaInfo(URI, FN, self.lastID)
        MI.run()
        tMedia = MI.getMedia()
        MI=None
        self.addMedia(tMedia)
#==============================================================================
    def cb_addNewStream(self, widget):
        if(self.windowMediaOpen==None):
            self.windowMediaOpen=gtk.glade.XML (self.gladefile,"OPEN_MEDIA")
            dic={"on_OPEN_BUTTON_CANCEL_clicked": self.cb_openMediaCancel,\
                "on_OPEN_BUTTON_OPEN_clicked": self.cb_openMediaOpen }
            self.windowMediaOpen.signal_autoconnect(dic)
        else:
            WND=self.windowMediaOpen.get_widget("OPEN_MEDIA")
            if not WND:
                self.windowMediaOpen=None
            else:
                WND.show()
        return
#==============================================================================
    def cb_onNewMenu(self, menu):
        if self.windowStreams:
            WND=self.windowStreams.get_widget("STREAM_WINDOW")
            WND.show()
#==============================================================================
    def setEditSubtitle(self, Sub):
        if not self.Subtitle:
            return
        if Sub == None:
            if (self.curSub!=-1):
                BUF=gtk.TextBuffer()
                BUF.set_text("")
                self.SubEdit.set_buffer(BUF)
                self.curSub=-1
                self.setSubStartTime(0)
                self.setSubEndTime(0)
        else:
            if (Sub.start_time!=self.curSub):
                BUF=gtk.TextBuffer()
                BUF.set_text(Sub.text)
                self.SubEdit.set_buffer(BUF)
                self.curSub=int(Sub.start_time)
                self.setSubStartTime(Sub.start_time)
                self.setSubEndTime(Sub.end_time)
#==============================================================================
    def cb_onSubsListSelect(self, widget):
        Row=None
        Selection = widget.get_selection()
        if Selection==None:
            return
        Model, Rows = Selection.get_selected_rows()
        if Rows != None:
            Row = Model[Rows[0][0]]
            if self.Subtitle:
                Sub = self.Subtitle.subs[Row[0]]
                self.setEditSubtitle(Sub)
                if self.player:
                    B=0;
                    if self.player.is_playing():
                        B=1
                        self.play_toggled()
                    real = long(Row[0]) # in ns
                    self.player.seek(real*1000000)
                    # allow for a preroll
                    self.player.get_state(timeout=50*gst.MSECOND) # 50 ms
                    if B==1:
                        self.play_toggled()
#==============================================================================
    def subsWindowUpdate(self):
        if not self.Subtitle:
            return
        if self.windowSubsList:
            self.subsListStore.clear()
            for i in self.Subtitle.subKeys:
                S=self.Subtitle.subs[i]
                iter = self.subsListStore.append(None)
                self.subsListStore.set(iter, 0, int(S.start_time),
                                            1, int(S.end_time),
                                            2, str(S.text))
#==============================================================================
    def saveProject(self):
        if not self.PFileName:
            return
        if self.PFileName[-4:]!=".spf":
            self.PFileName=self.PFileName+".spf"
        PXML=ProjectXML()
        PXML.addHeadInfo("title", "Soufleur development version")
        PXML.addHeadInfo("desc", "This is version current at development stage.")
        PXML.addHeadInfo("author", "DarakuTenshi")
        PXML.addHeadInfo("email", "otaky@ukr.net")
        PXML.addHeadInfo("info", "Sample of save function")
        for i in self.media:
            PXML.addMedia(i)
        for i in self.Subtitles:
            PXML.addSubtitle(i)
        PXML.write(self.PFileName)
#==============================================================================
    def cb_projectSaveOpen(self, widget):
        WND=self.windowProjectSO.get_widget("SAVE_OPEN_PFILE")
        self.PFileName=WND.get_filename()
        self.saveProject()
        WND.hide()
#==============================================================================
    def cb_projectSaveCancel(self, widget):
        if(self.windowProjectSO==None): return
        WND=self.windowProjectSO.get_widget("SAVE_OPEN_PFILE")
        WND.hide()
#==============================================================================
    def cb_onSaveAsMenu(self, widget):
        self.PFileName=None
        self.cb_onSaveMenu(widget)
#==============================================================================
    def cb_onSaveMenu(self, widget):
        if self.PFileName:
            self.saveProject()
            return
        if(self.windowProjectSO==None):
            self.windowProjectSO=gtk.glade.XML (self.gladefile,"SAVE_OPEN_PFILE")
            dic={"on_PROJECT_BUTTON_CANCEL_clicked": self.cb_projectSaveCancel,\
                "on_PROJECT_BUTTON_OK_clicked": self.cb_projectSaveOpen }
            self.windowProjectSO.signal_autoconnect(dic)
            WND=self.windowProjectSO.get_widget("SAVE_OPEN_PFILE")
            WND.set_action(gtk.FILE_CHOOSER_ACTION_SAVE)
            OKB = self.windowProjectSO.get_widget("PROJECT_BUTTON_OK")
            OKB.set_label("gtk-save")
            OKB.set_use_stock(True)
            Filter=gtk.FileFilter()
            Filter.set_name("Souffleur project file")
            Filter.add_pattern("*.spf")
            WND.add_filter(Filter)
        else:
            WND=self.windowProjectSO.get_widget("SAVE_OPEN_PFILE")
            if(WND==None):
                self.windowProjectSO=None
                self.cb_onSaveMenu(widget)
            else:
                WND.show()
#==============================================================================
    def cb_subDel(self, widget):
        if (self.Subtitle != None) and (self.curSub != -1):
            self.Subtitle.subDel(self.curSub)
#==============================================================================
    def cb_subChangeSave(self, widget):
        if (self.Subtitle != None):
            if (self.curSub != -1):
                BUF = self.SubEdit.get_buffer()
                TEXT = BUF.get_text(BUF.get_start_iter(), BUF.get_end_iter())
                self.Subtitle.subs[int(self.curSub)].text = str(TEXT)
                self.Subtitle.subs[int(self.curSub)].end_time=self.subEndTime.get_value_as_int()
                if self.Subtitle.subs[int(self.curSub)].start_time!=self.subStartTime.get_value_as_int():
                    newTime=self.subStartTime.get_value_as_int()
                    self.Subtitle.subs[int(self.curSub)].start_time=newTime
                    self.Subtitle.subUpdate(int(self.curSub))
                    self.curSub = newTime
                #for i in self.Subtitles:
                #    if i.subSource == self.Subtitle.subSource:
                #        self.Subtitles[self.Subtitles.index(i)]=self.Subtitle
                self.subsWindowUpdate()
            else:
                self.subAdd()
#==============================================================================
    def subAdd(self):
        ST = self.subStartTime.get_value()
        ET = self.subEndTime.get_value()
        BUF = self.SubEdit.get_buffer()
        Text = BUF.get_text(BUF.get_start_iter(), BUF.get_end_iter())
        if (( ST > 0 ) and ( ET > ST ) and ( Text != "" )):
            self.Subtitle.subAdd(ST, ET, Text, None, 1)
            self.curSub = ST
#==============================================================================
    def cb_setSubStartTime(self, widget):
        self.subStartTime.set_value(self.p_position/1000000)
#==============================================================================
    def cb_setSubEndTime(self, widget):
        self.subEndTime.set_value(self.p_position/1000000)
#==============================================================================
    def setSubStartTime(self, time):
        self.subStartTime.set_value(time)
#==============================================================================
    def setSubEndTime(self, time):
        self.subEndTime.set_value(time)
#==============================================================================
    def exposeEventVideoOut(self, widget, event):
        if self.videoWidgetGst:
            self.videoWidgetGst.do_expose_event(event)
#==============================================================================
    def changeValueAdjustment(self, widget, t1, t2):
        #if (not self.scroll):
            real = long(self.adjustment.get_value()) # in ns
            self.player.seek(real)
            # allow for a preroll
            self.player.get_state(timeout=50*gst.MSECOND) # 50 ms

#==============================================================================
    def buttonReleaseAdjustment(self, widget, event):
        self.scroll=0
#==============================================================================
    def buttonPressAdjustment(self, widget, event):
        self.scroll=1
#==============================================================================
    def playerStop(self, widget):
        if self.player:
            if self.player.is_playing():
                self.play_toggled()
            self.player.stop()
#==============================================================================
    def playerPlay(self, widget):
        if self.player:
            self.play_toggled()
#==============================================================================
    def mainFileOpen(self, widget):
        if(self.windowProjectOpen==None):
            self.windowProjectOpen=gtk.glade.XML (self.gladefile,"SAVE_OPEN_PFILE")
            dic={"on_PROJECT_BUTTON_CANCEL_clicked": self.openFileCancel,\
                "on_PROJECT_BUTTON_OK_clicked": self.openFileOpen }
            self.windowProjectOpen.signal_autoconnect(dic)
            WND=self.windowProjectOpen.get_widget("SAVE_OPEN_PFILE")
            WND.set_action(gtk.FILE_CHOOSER_ACTION_OPEN)
            OKB = self.windowProjectOpen.get_widget("PROJECT_BUTTON_OK")
            OKB.set_label("gtk-open")
            OKB.set_use_stock(True)
            Filter=gtk.FileFilter()
            Filter.set_name("Souffleur project file")
            Filter.add_pattern("*.spf")
            WND.add_filter(Filter)
        else:
            WND=self.windowProjectOpen.get_widget("SAVE_OPEN_PFILE")
            if(WND==None):
                self.windowProjectOpen=None
                self.mainFileOpen(widget)
            else:
                WND.show()
        return
#==============================================================================
    def openFileCancel(self, widget):
        if(self.windowProjectOpen==None): return
        WND=self.windowProjectOpen.get_widget("SAVE_OPEN_PFILE")
        WND.hide()
        return
#==============================================================================
    def openFileOpen(self, widget):
        WND=self.windowProjectOpen.get_widget("SAVE_OPEN_PFILE")
        self.PFileName=WND.get_filename()
        WND.hide()
        PXML=ProjectXML()
        PXML.load(self.PFileName)
        for i in PXML.getMedia():
            self.addMedia(i)
        self.Subtitles=[]
        for i in PXML.getSubtitle():
            self.Subtitles.append(i)
        if len(self.media)>0:
            WND=self.windowStreams.get_widget("STREAM_WINDOW")
            WND.show()
        return
#==============================================================================
    def addMedia(self, mInfo):
        if not mInfo:
            return
        N=len(self.media)
        self.media.append(mInfo)
        self.lastID = mInfo.lastID
        self.updateStreamWindow()
        if "subtitle" in mInfo.MIME:
            tSubtitle = Subtitles()
            tSubtitle.subLoad(mInfo.source, mInfo.Streams[0].ID)
            self.Subtitles.append(tSubtitle)
        else:
            self.videoWidgetGst=VideoWidget(self.videoWidget)
            self.player=GstPlayer(self.videoWidgetGst)
            self.player.set_location("file://"+mInfo.source)
            if self.videoWidget.flags() & gtk.REALIZED:
                self.play_toggled()
            else:
                self.videoWidget.connect_after('realize',
                                           lambda *x: self.play_toggled())
        return
#==============================================================================
    def play_toggled(self):
        if self.player.is_playing():
            self.player.pause()
            self.playButton.set_stock_id(gtk.STOCK_MEDIA_PLAY)
            #self.playButton.set_icon_name(gtk.STOCK_MEDIA_PLAY)
        else:
            self.player.play()
            if self.update_id == -1:
                self.update_id = gobject.timeout_add(self.UPDATE_INTERVAL,
                                                     self.update_scale_cb)
            self.playButton.set_stock_id(gtk.STOCK_MEDIA_PAUSE)
#==============================================================================
    def update_scale_cb(self):
        had_duration = self.p_duration != gst.CLOCK_TIME_NONE
        self.p_position, self.p_duration = self.player.query_position()
        if self.p_duration != self.t_duration:
            self.t_duration = self.p_duration
            self.adjustment.set_range(0, self.t_duration)
        tmSec= self.p_position/1000000
        MSec = tmSec
        tmSec = tmSec/1000
        Sec = tmSec%60
        tmSec = tmSec/60
        Min = tmSec%60
        Hour = tmSec/60
        if self.Subtitle:
            TText = self.Subtitle.getSub(MSec)
            if TText:
                self.setEditSubtitle(TText)
            else:
                self.setEditSubtitle(None)
        if (self.p_position != gst.CLOCK_TIME_NONE):# and (not self.scroll):
            value = self.p_position
            self.adjustment.set_value(value)
        self.labelHour.set_text("%02d"%Hour)
        self.labelMin.set_text("%02d"%Min)
        self.labelSec.set_text("%02d"%Sec)
        self.labelMSec.set_text("%09d"%MSec)
        return True
#==============================================================================
#	MAIN:
#==============================================================================
souffleur=Souffleur()
gtk.main()
