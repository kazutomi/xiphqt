#!/usr/bin/env python

import oggStreams
import sys

try:
    import pygtk
    #tell pyGTK, if possible, that we want GTKv2
    pygtk.require("2.0")
except:
    #Some distributions come with GTK2, but not pyGTK
    pass
try:
    import gtk
    import gtk.glade
except:
    print "You need to install pyGTK or GTKv2 ",
    print "or set your PYTHONPATH correctly."
    print "try: export PYTHONPATH=",
    print "/usr/local/lib/python2.2/site-packages/"
    sys.exit(1)
#now we have both gtk and gtk.glade imported
#Also, we know we are running GTK v2

class Souffleur:
#    gladefile=""
    def __init__(self):
	"""
	In this init we are going to display the main
	Souffleur window
	"""
	gladefile="souffleur.glade"
	windowname="MAIN_WINDOW"
	self.wTree=gtk.glade.XML (gladefile,windowname)
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
		"on_main_file_open_activate": self.mainFileOpen}
	self.wTree.signal_autoconnect (dic)

	self.windowFileOpen=None
	self.windowStreams=gtk.glade.XML (self.gladefile,"STREAM_WINDOW")
	WND=self.windowStreams.get_widget("STREAM_WINDOW")
	WND.hide()
	self.Streams=None
	return
#==============================================================================
    def mainFileOpen(self, widget):
	if(self.windowFileOpen==None):
	    self.windowFileOpen=gtk.glade.XML (self.gladefile,"OPEN_OGG")
	    dic={"on_OPEN_BUTTON_CANCEL_clicked": self.openFileCancel,\
		"on_OPEN_BUTTON_OPEN_clicked": self.openFileOpen }
	    self.windowFileOpen.signal_autoconnect(dic)
	    WND=self.windowFileOpen.get_widget("OPEN_OGG")
	    Filter=gtk.FileFilter()
	    Filter.set_name("OGM file")
	    Filter.add_pattern("*.og[gm]")
	    WND.add_filter(Filter)
	else:
	    WND=self.windowFileOpen.get_widget("OPEN_OGG")
	    if(WND==None):
		self.windowFileOpen=None
		self.mainFileOpen(widget)
	    else:
		WND.show()
	return
#==============================================================================
    def openFileCancel(self, widget):
	if(self.windowFileOpen==None): return
	WND=self.windowFileOpen.get_widget("OPEN_OGG")
	WND.hide()
	return
#==============================================================================
    def openFileOpen(self, widget):
	WND=self.windowFileOpen.get_widget("OPEN_OGG")
	FN=WND.get_filename()
	if((FN!="")and(FN!=None)):
	    self.Streams=oggStreams.oggStreams(FN)
	WND.hide()
	WND=self.windowStreams.get_widget("STREAM_WINDOW")
	WND.show()
	self.refreshStreamsWindow()
	return
#==============================================================================
    def refreshStreamsWindow(self):
	TStreams=self.Streams.getStreams()
	for OGGStream in Streams:
	    #TODO
	return
#==============================================================================
#	MAIN:
#==============================================================================
souffleur=Souffleur()
gtk.main()
