__name__="oggStreams - class of streams in OGG files"
__author__="Maxim Litvinov (aka DarakuTenshi) otaky<at>ukr.net"

import os
import re

import oggStream

class oggStreams:
    """
    Class for work with OGG.
    """
    def __init__(self, fileName):
	"""
	Constructor of oggStreams class.
	@param[IN] fileName - name of OGG file.
	"""
	#Run ogminfo util on given file.
	OGMINFO=os.popen("ogminfo -v \"" + fileName + "\"", "r")
	if(OGMINFO==None): return NULL
	STAT=OGMINFO.readlines() #Output data from ogminfo to the list of strings.
	OGMINFO.close()

	self.streams={}
	for i in STAT:
	    #Stream info
	    regSTAT=re.compile("\(ogminfo.c\) [(](\w)(\d)[/](\S+) (\d)[)] (\S+)")
	    mathSTAT=regSTAT.match(i)
	    if mathSTAT!=None:
		HASH=mathSTAT.group(1)+mathSTAT.group(2)
		self.streams[HASH]=oggStream.oggStream(mathSTAT.group(4), mathSTAT.group(1), fileName)
	    #Stream attribute
	    regSTAT=re.compile("\(ogminfo.c\) (\S+)[:](\s+)(\S+)[=]([\S\s]+)[\n]")
	    mathSTAT=regSTAT.match(i)
	    if mathSTAT!=None:
		OGGStream=self.streams[mathSTAT.group(1)]
		OGGStream.addAttr(mathSTAT.group(3), mathSTAT.group(4))
	return
#==============================================================================
    def getStreams(self):
	"""
	    This function return a list of streams.
	"""
	return self.streams.values()
#==============================================================================
