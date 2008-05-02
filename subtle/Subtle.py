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


from GPlayer import VideoWidget
from GPlayer import GstPlayer
from Subtitles import Subtitles
import sys
import os

from MediaInfo import MediaInfo
from SubtleXML import ProjectXML

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

ONLINE_MODE = 1
EDITING_MODE = 0

class Subtle:
    def __init__(self):
        """
        In this init we are going to display the main
        Subtle window
        """
        gladefile="subtle.glade"
        windowname="MAIN_WINDOW"
        
        self.update_id = -1
        self.p_position = gst.CLOCK_TIME_NONE
        self.p_duration = gst.CLOCK_TIME_NONE
        self.UPDATE_INTERVAL=100
        
        self.Subtitle = None
        self.Subtitles = []
	# Current subtitle being edited
	self.cur_edit_sub_iter = None
	#Current editing mode
	## Refer to globals for values
	self.mode = ONLINE_MODE

        #self.scroll = 0
        self.videoWidgetGst = None
        self.player = None
        self.t_duration = 0
        self.media = []

        self.wTree=gtk.glade.XML(gladefile,windowname)
        self.gladefile = gladefile
        dic = { "gtk_main_quit" : (gtk.main_quit),\
            "on_main_file_quit_activate": (gtk.main_quit), \
            "on_main_file_open_activate": self.mainFileOpen, \
            "on_TOOL_PLAY_clicked": self.playerPlay,\
            "on_TOOL_STOP_clicked": self.playerStop,\
            "on_TOOL_SEEK_FORWARD_clicked": self.playerSeekForward,\
            "on_TOOL_SEEK_REWIND_clicked": self.playerSeekRewind,\
            "on_TOOL_HIDE_STREAMS_clicked": self.cb_hideStreamsPane,\
            "on_TOOL_HIDE_SUBLIST_clicked": self.cb_hideSubPane,\
            "on_MEDIA_ADJUSTMENT_button_press_event": self.buttonPressAdjustment,\
            "on_MEDIA_ADJUSTMENT_button_release_event": self.buttonReleaseAdjustment,\
            "on_MEDIA_ADJUSTMENT_change_value": self.changeValueAdjustment,\
            "on_VIDEO_OUT_PUT_expose_event": self.exposeEventVideoOut,\
            "on_main_file_save_activate": self.cb_onSaveMenu,\
            "on_main_file_save_as_activate": self.cb_onSaveAsMenu,\
            "on_main_file_new_activate": self.cb_onNewMenu,\
            "on_MAIN_VIEW_STREAMS_PANE_activate": self.cb_showStreamsPane,\
            "on_MAIN_VIEW_SUBTITLES_activate": self.cb_showSubtitlePane,\
            "on_TOOL_DEL_STREAM_clicked": self.cb_delStream,\
	    "on_TOOL_MOD_STREAM_clicked": self.cb_modStream,\
	    "on_TOOL_NEW_STREAM_clicked": self.cb_newStream,\
	    "on_TOOL_ADD_STREAM_clicked": self.cb_addNewStream,\
	    "on_LIST_SUBS_button_release_event": self.cb_onSubsListSelect,\
	    "on_LIST_SUBS_button_press_event": self.cb_onSubsListSelect,\
	    "on_txt_subedit_key_release_event": self.cb_onSubtitleEdit,\
	    "on_tgl_mode_toggled": self.cb_onModeChanged,\
	    "on_TOOL_SAVE_STREAM_clicked": self.cb_saveStream,\
	    "on_TOOL_DEL_SUBS_clicked": self.cb_subDel,\
	    "on_TOOL_OUT_SUB_clicked": self.cb_subOut,\
	    "on_TOOL_IN_SUB_clicked": self.cb_subIn,\
	    "on_TOOL_INS_B4_SUB_clicked": self.cb_onSubInsB4,\
	    "on_TOOL_INS_AFTER_SUB_clicked": self.cb_onSubInsAfter,\
	    "on_TOOL_IN_SUB_clicked": self.cb_subIn}
        self.wTree.signal_autoconnect (dic)
        
	self.windowMainWindow=self.wTree.get_widget("MAIN_WINDOW")
        self.windowProjectOpen=None
        self.windowProjectSO=None
	self.windowNewSubFile=None
        self.PFileName=None
        self.windowMediaOpen=None
        #self.windowStreams=gtk.glade.XML (self.gladefile,"STREAM_WINDOW")
        #dic = {"on_TOOL_DEL_STREAM_clicked": self.cb_delStream,\
        #        "on_TOOL_MOD_STREAM_clicked": self.cb_modStream,\
        #        "on_TOOL_NEW_STREAM_clicked": self.cb_newStream,\
        #        "on_TOOL_ADD_STREAM_clicked": self.cb_addNewStream,\
        #        "on_STREAM_WINDOW_delete_event": self.cb_StreamWindowDelete}
        #self.windowStreams.signal_autoconnect (dic)
        ### Setup LIST_STREAMS
        #LIST = self.windowStreams.get_widget("LIST_STREAMS")
        LIST = self.wTree.get_widget("LIST_STREAMS")
        if LIST:
            self.streamsTreeStore = gtk.TreeStore(gobject.TYPE_STRING, gobject.TYPE_UINT)
            LIST.set_model(self.streamsTreeStore)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('Streams', cell, text = 0)
            LIST.append_column(tvcolumn)
        
        #self.windowSubsList=gtk.glade.XML (self.gladefile,"SUBS_LIST")
        #dic = {"on_LIST_SUBS_cursor_changed": self.cb_onSubsListSelect,\
        #        "on_TOOL_SAVE_STREAM_clicked": self.cb_saveStream,\
        #        "on_TOOL_DEL_SUBS_clicked": self.cb_subDel,\
        #        "on_TOOL_OUT_SUB_clicked": self.cb_subOut,\
        #        "on_TOOL_IN_SUB_clicked": self.cb_subIn,\
        #        "on_TOOL_INS_B4_SUB_clicked": self.cb_onSubInsB4,\
        #        "on_TOOL_INS_AFTER_SUB_clicked": self.cb_onSubInsAfter,\
        #        "on_TOOL_IN_SUB_clicked": self.cb_subIn,\
        #        "on_SUBS_LIST_delete_event": self.cb_onSubsWindowDelete}
        #self.windowSubsList.signal_autoconnect (dic)
        #SUBLIST = self.windowSubsList.get_widget("LIST_SUBS")
        SUBLIST = self.wTree.get_widget("LIST_SUBS")
	SUBLIST.add_events(gtk.gdk._2BUTTON_PRESS)
        if SUBLIST:
            self.subsListStore = gtk.ListStore(gobject.TYPE_UINT,
                                                gobject.TYPE_UINT,
						gobject.TYPE_UINT,
                                                gobject.TYPE_STRING)
            SUBLIST.set_model(self.subsListStore)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('#', cell, text = 0)
            SUBLIST.append_column(tvcolumn)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('Start', cell, text = 1)
            SUBLIST.append_column(tvcolumn)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('End', cell, text = 2)
            SUBLIST.append_column(tvcolumn)
            cell = gtk.CellRendererText()
            tvcolumn = gtk.TreeViewColumn('Text', cell, text = 3)
	    tvcolumn.set_resizable(True)
            SUBLIST.append_column(tvcolumn)
        #WND=self.windowStreams.get_widget("STREAM_WINDOW")
        #WND.hide()
        #WND=self.windowSubsList.get_widget("SUBS_LIST")
        #WND.hide()
        ### Main window setup
        self.videoWidget = self.wTree.get_widget("VIDEO_OUT_PUT")
        self.adjustment = self.wTree.get_widget("MEDIA_ADJUSTMENT")
        self.labelHour = self.wTree.get_widget("LABEL_HOUR")
        self.labelMin = self.wTree.get_widget("LABEL_MIN")
        self.labelSec = self.wTree.get_widget("LABEL_SEC")
        self.labelMSec = self.wTree.get_widget("LABEL_MSEC")
        self.playButton = self.wTree.get_widget("TOOL_PLAY")
	self.lbl_cur_fps = self.wTree.get_widget("lbl_cur_fps")
	self.streams_pane = self.wTree.get_widget("streams_pane")
	self.subtitle_pane = self.wTree.get_widget("subtitle_pane")
	self.txt_subedit = self.wTree.get_widget("txt_subedit")
	self.tgl_mode = self.wTree.get_widget("tgl_mode")
	self.subList = SUBLIST
	#self.windowMainWindow.maximize()
        return
#==============================================================================
    def cb_onModeChanged(self, widget):
	"""
	    Change from online mode to editing mode
	    and vice versa
	"""
	# Online mode
	if self.tgl_mode.get_active():
	    self.mode = ONLINE_MODE
	    self.txt_subedit.set_sensitive(False)
	    return
	# Editing mode
	else:
	    self.mode = EDITING_MODE
	    self.txt_subedit.set_sensitive(True)
	    return
#==============================================================================
    def cb_hideSubPane(self, widget):
	"""
	    Hide the subtitles pane
	"""
	self.subtitle_pane.hide()
	return
#==============================================================================
    def cb_hideStreamsPane(self, widget):
	"""
	    Hide the streams pane
	"""
	self.streams_pane.hide()
	return
#==============================================================================
    def cb_showStreamsPane(self, widget):
	"""
	    Hide the streams pane
	"""
	self.streams_pane.show()
	return
#==============================================================================
    def cb_onSubInsB4(self, widget):
	"""
	    Insert new subtitle before current selected
	"""
	subsList = self.wTree.get_widget("LIST_SUBS")
	selection = subsList.get_selection()
	result = selection.get_selected()
	if result:
            model, iter = result
	    if self.subsListStore.iter_is_valid(iter):
		cur, sTime, eTime = self.subsListStore.get(iter, 0, 1, 2)
		self.Subtitle.subAdd(sTime-1,sTime-2,'',None,1)
		self.subsListStore.insert_before(iter, (cur, sTime-1, sTime-2, 'New subtitle...'))
		self.reorder_SubsListStore()
	return
#==============================================================================
    def cb_onSubInsAfter(self, widget):
	"""
	    Insert new subtitle after current selected
	"""
	subsList = self.wTree.get_widget("LIST_SUBS")
	selection = subsList.get_selection()
	result = selection.get_selected()
	if result:
            model, iter = result
	    if self.subsListStore.iter_is_valid(iter):
		cur, sTime, eTime = self.subsListStore.get(iter, 0, 1, 2)
		self.Subtitle.subAdd(eTime+1,eTime+2,'',None,1)
		cur += 1
		self.subsListStore.insert_after(iter, (cur, eTime+1, eTime+2, 'New subtitle...'))
		self.reorder_SubsListStore()
	return
#==============================================================================
    def reorder_SubsListStore(self):
	"""
	    Reorder the subs listStore when added or deleted
	"""
	iter = self.subsListStore.get_iter_first()
	cur = 0
	while iter is not None:
	    if iter is not None:
		self.subsListStore.set_value(iter, 0, cur)
		cur += 1
		iter = self.subsListStore.iter_next(iter)
#==============================================================================
    def cb_subDel(self, widget):
	"""
	    Delete a subtile from the list store and from memory
	"""
	subsList = self.wTree.get_widget("LIST_SUBS")
	selection = subsList.get_selection()
	result = selection.get_selected()
	if result:
            model, iter = result
	    subKey = self.subsListStore.get(iter, 1)
            self.Subtitle.subDel(subKey[0])
	    model.remove(iter)
	    self.reorder_SubsListStore()
#==============================================================================
    def cb_subOut(self, widget):
	"""
	    Set subtitle endtime
	"""
	if self.player:
	    subsList = self.windowSubsList.get_widget("LIST_SUBS")
	    selection = subsList.get_selection()
	    result = selection.get_selected()
	    if result:
		model, iter = result
		subKey, end_time = self.subsListStore.get(iter, 1, 2)
		try:
		    self.Subtitle.getSub(subKey).end_time = self.p_position/1000000
		    self.Subtitle.subUpdate(subKey)
		except:
		    print "Error while setting subtitle timecode"
		    return
		self.subsListStore.set(iter,2,self.p_position/1000000)
	return True
#==============================================================================
    def cb_subIn(self, widget):
	"""
	    Set subtitle start time
	"""
	if self.player:
	    subsList = self.windowSubsList.get_widget("LIST_SUBS")
	    selection = subsList.get_selection()
	    result = selection.get_selected()
	    if result:
		model, iter = result
		subKey = self.subsListStore.get(iter, 1)
		try:
		    self.Subtitle.getSub(subKey[0]).start_time = self.p_position/1000000
		    self.Subtitle.subUpdate(subKey[0])
		except:
		    print "Error while setting subtitle timecode"
		    return
		self.subsListStore.set(iter,1,self.p_position/1000000)
	return True
#==============================================================================
    def cb_onSubsWindowDelete(self, widget, event):
        widget.hide()
        return True
#==============================================================================
    def cb_StreamWindowDelete(self, widget, event):
        widget.hide()
        return True
#==============================================================================
    def cb_onSubtitleWindow(self, menu):
        if self.windowSubsList:
            WND=self.windowSubsList.get_widget("SUBS_LIST")
            WND.show()
#==============================================================================
    def cb_showSubtitlePane(self, menu):
	"""
	   Show subtitle pane
	"""
	self.subtitle_pane.show()
#==============================================================================
    def cb_onStreamsWindow(self, menu):
        if self.windowStreams:
            WND=self.windowStreams.get_widget("STREAM_WINDOW")
            WND.show()
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
	# We have removed the window
        #if not self.windowStreams:
        #    return
        if not self.streamsTreeStore:
            return
        #TView = self.windowStreams.get_widget("LIST_STREAMS")
        TView = self.wTree.get_widget("LIST_STREAMS")
        TSelec = TView.get_selection()
        TModel, TIter = TSelec.get_selected()
        if not TIter:
            return
        N=TModel.get_value(TIter, 0)
	# FIXME: We should actually get the selected subtitle
	if "Subtitle" in N:
	    self.setSubtitle()
#==============================================================================
    def cb_newStream(self, widget):
	"""
	   Create a new subtitle
	"""
	#TODO: Lets popup something that will let us choose sub type
        if(self.windowNewSubFile==None):
            self.windowNewSubFile=gtk.glade.XML (self.gladefile,"NEW_SUBTITLE")
            #dic={"on_OPEN_BUTTON_CANCEL_clicked": self.cb_openMediaCancel,\
            #    "on_OPEN_BUTTON_OPEN_clicked": self.cb_openMediaOpen }
            #self.windowMediaOpen.signal_autoconnect(dic)
            WND=self.windowNewSubFile.get_widget("NEW_SUBTITLE")
	    WND.show()
        else:
            WND=self.windowNewSubFile.get_widget("NEW_SUBTITLE")
            if not WND:
                self.windowNewSubFile=None
            else:
                WND.show()
	return
#==============================================================================
    def setSubtitle(self):
        if self.Subtitle:
	    # We have removed the window for now
            #if (self.windowStreams):
            #    WND=self.windowSubsList.get_widget("SUBS_LIST")
            #    WND.show()
            self.subsWindowUpdate()
#==============================================================================
    def updateStreamWindow(self):
	#FIXME: This should be more complete and better handled
	# Maybe all streams must be on the same list/dict
        if not self.streamsTreeStore:
            return
        self.streamsTreeStore.clear()
	for sub in self.Subtitles:
            iter = self.streamsTreeStore.append(None)
	    self.streamsTreeStore.set(iter, 0, "Subtitle: "+ sub.filename, 1, self.Subtitles.index(sub))
	    child = self.streamsTreeStore.append(iter)
	    self.streamsTreeStore.set(child, 0, "Type: " + sub.subType, \
		    1, self.Subtitles.index(sub))
        for mInfo in self.media:
            iter = self.streamsTreeStore.append(None)
            self.streamsTreeStore.set(iter, 0, mInfo.source, 1, self.media.index(mInfo))
	    if mInfo.has_video:
                child = self.streamsTreeStore.append(iter)
		self.streamsTreeStore.set(child, 0, "Mimetype: " + mInfo.MIME.split("/")[1], \
			1, self.media.index(mInfo))
                child = self.streamsTreeStore.append(iter)
		self.streamsTreeStore.set(child, 0, "Resolution: %dx%d "% (mInfo.videoWidth, mInfo.videoHeight), \
			1, self.media.index(mInfo))
                child = self.streamsTreeStore.append(iter)
		self.streamsTreeStore.set(child, 0, ("Framerate: %.2f" % mInfo.framerate), \
			1, self.media.index(mInfo))
                child = self.streamsTreeStore.append(iter)
		self.streamsTreeStore.set(child, 0, ("Length: %s s" % mInfo.videoLengthS), \
			1, self.media.index(mInfo))
                child = self.streamsTreeStore.append(iter)
		self.streamsTreeStore.set(child, 0, ("Frames: %d" % (mInfo.videoLengthS/(1/mInfo.framerate))), \
			1, self.media.index(mInfo))
                child = self.streamsTreeStore.append(iter)
#==============================================================================
    def cb_delStream(self, widget):
	"""
	    Remove a stream from the current project
	"""
	#FIXME: We broke this ...
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
	self.TEST_SUB_URI = URI
        WND.hide()

	extension = os.path.splitext(FN)[1]
	tmpSub = Subtitles(FN)
	if extension in tmpSub.getSupportedTypes():
	    #TODO: We should improve the way we check subtitles
	    tmpSub.subLoad(FN)
	    self.Subtitle = tmpSub
	    self.Subtitles.append(tmpSub)
	    self.updateStreamWindow()
	else:
	    #TODO: Check if it is media or throw error
	    MI = MediaInfo(FN, URI)
	    # Lets poll for information
	    gobject.timeout_add(30, self.addMedia, MI)

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
    def cb_onSubsListSelect(self, widget, event):
	"""
	    Do the proper thing when subtitle is selected
	    2 clicks seeks de video to its timecode
	    1 click edits on the TextView
	"""
	#FIXME: Something nasty happens on the selection of the subtitle
	# Only happens the first time and throws an exception
	Row=None
	Selection = widget.get_selection()
	if Selection==None:
	    return
	Model, Rows = Selection.get_selected_rows()
	if Rows != None:
	    #FIXME: Buggy solution!! Has something to do with
	    # button press release event generated...
	    Row = Model[Rows[0][0]]
	    if self.Subtitle:
		Sub = self.Subtitle.subs[Row[1]]
	if event.type == gtk.gdk._2BUTTON_PRESS and event.button == 1:
	    if self.player:
		B=0;
		self.player.set_subtitle_text(Sub.text)
		if self.player.is_playing():
		    B=1
		    self.play_toggled()
		real = long(Row[1]) # in ns
		self.player.seek(real*1000000)
		# allow for a preroll
		self.player.get_state(timeout=50*gst.MSECOND) # 50 ms
		if B==1:
		    self.play_toggled()
	if event.type == gtk.gdk.BUTTON_RELEASE:
	    model, self.cur_edit_sub_iter =  Selection.get_selected() 
	    self.setSubtitleEdit(Sub.text)
#==============================================================================
    def cb_onSubTextEdited(self, cell, path, new_text):
	"""
	    Callback to change subtitle when subtitle text was changed
	"""
	iter = self.subsListStore.get_iter(path)
	subKey, ETime, Text = self.subsListStore.get(iter, 1, 2, 3)
	subtitle = self.Subtitle.getSub(subKey)
	if subtitle.text != new_text:
	    subAttr = subtitle.Attributes
	    self.Subtitle.subDel(subKey)
	    self.Subtitle.subAdd(subKey,ETime,new_text,subAttr,1)
	    self.subsListStore.set(iter,3,new_text)

        return True
#==============================================================================
    def cb_onSubtitleEdit(self, widget, event):
	"""
	    Updates the subtile list in realtime
	"""
	id, subKey, ETime = self.subsListStore.get(self.cur_edit_sub_iter, 0, 1, 2)
	#self.Subtitle.subDel(subKey)
	text = self.txt_subedit.get_buffer().get_property('text')
	self.Subtitle.updateText(subKey,text)
	#self.Subtitle.subAdd(subKey,ETime,text,None,0)
	self.subsListStore.set(self.cur_edit_sub_iter, 3, text) 
	return
#==============================================================================
    def setSubtitleEdit(self,sub):
	"""
	    Set the subtitle to be edited
	"""
	buf = self.txt_subedit.get_buffer()
	buf.set_text(sub)
#==============================================================================
    def subsWindowUpdate(self):
        if not self.Subtitle:
            return
	# We have removed the window for now
        #if self.windowSubsList:
	self.subsListStore.clear()
	j=0
	for i in self.Subtitle.subKeys:
	    S=self.Subtitle.subs[i]
	    iter = self.subsListStore.append(None)
	    self.subsListStore.set(iter,0, j, 
					1, int(S.start_time),
					2, int(S.end_time),
					3, str(S.text))
	    j +=1
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
            Filter.set_name("Subtle project file")
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
    def playerSlowMotion(self, widget):
	"""
	    Put the current playing video in slow motion
	"""
	#TODO: Implement it
	pass
#==============================================================================
    def playerFastForward(self, widget):
	"""
	    Put the current playing video in FastForward 
	"""
	#TODO: Implement it
	pass
#==============================================================================
    def playerSeekForward(self, widget):
	"""
	    Jump some time beyond current position
	"""
	#TODO: Implement it
	pass
#==============================================================================
    def playerSeekRewind(self, widget):
	"""
	    Jump back some time
	"""
	#TODO: Implement it
	pass
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
            Filter.set_name("Subtle project file")
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
	# Frist, wait for media discovery 
	if mInfo.poll():
	    return True 
	mInfo = mInfo.getMedia()
        self.media.append(mInfo)
        self.updateStreamWindow()
	#Set videoWidget sizes according to media standards
	self.videoWidget.set_size_request(mInfo.videoWidth, mInfo.videoHeight)
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
	    if gobject.source_remove(self.update_id):
		    self.update_id = -1
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
            if self.player.is_playing():
                if TText:
		    self.player.set_subtitle_text(TText.text)
		    # Select current playing subtitle
		    if self.mode == ONLINE_MODE:
			Selection = self.subList.get_selection() 
			#FIXME: This sometimes bugs ... Why??
			Selection.select_path(TText.number-1)
			self.setSubtitleEdit(TText.text)
                else:
		    self.player.set_subtitle_text('')
		    if self.mode == ONLINE_MODE:
			self.setSubtitleEdit('')
			# Unselect what is not being played
			Selection = self.subList.get_selection()
			if Selection:
			    Selection.unselect_all()
        if (self.p_position != gst.CLOCK_TIME_NONE):# and (not self.scroll):
            value = self.p_position
            self.adjustment.set_value(value)
        self.labelHour.set_text("%02d"%Hour)
        self.labelMin.set_text("%02d"%Min)
        self.labelSec.set_text("%02d"%Sec)
	#BUG: We are not displaying that correctly
        self.labelMSec.set_text("%09d"%MSec)
	#FIXME: We should know which media is playing
	self.lbl_cur_fps.set_text("%d"%(MSec/1/self.media[0].framerate))
        return True
#==============================================================================
#	MAIN:
#==============================================================================
subtle=Subtle()
gtk.main()
