__name__="oggStream - class of stream in OGG files"
__author__="Maxim Litvinov (aka DarakuTenshi) otaky<at>ukr.net"

import os

OGG_STREAM_VIDEO=1
OGG_STREAM_AUDIO=2
OGG_STREAM_TEXT=3

class oggStream:
    """
    Class for processing stream in OGG file.
    """
#==============================================================================
    def __init__(self, streamSerial, streamType, streamSource):
	"""
	@brief Constructor of oggStream class
	@param[IN] streamSerial - serial number of stream in OGG file
	@param[IN] streamType - one char type id of stream (output from ogginfo tool)
	"""
	self.serial=streamSerial
	self.source=streamSource
	self.intType=0
	self.strType=""
	if(streamType=="v"): self.strType="video"; self.intType=OGG_STREAM_VIDEO
	if(streamType=="a"): self.strType="audio"; self.intType=OGG_STREAM_AUDIO
	if(streamType=="t"): self.strType="text"; self.intType=OGG_STREAM_TEXT
	self.attrNames=[]
	self.attrValues=[]
	return
#==============================================================================
    def getSource(self):
	"""
	Return stream source
	"""
	return self.source;
#==============================================================================
    def getType(self):
	"""
	Return integer value of type of stream
	"""
	return self.intType
#==============================================================================
    def getStrType(self):
	"""
	Return string value of type of stream
	"""
	return self.strType
#==============================================================================
    def getSerial(self):
	"""
	Return serial number of stream
	"""
	return self.serial
#==============================================================================
    def getAttr(self, attrIndex):
	"""
	@brief return attribute of stream.
	
	This function return list of attribute name and attribute value,
	by index.
	@param[IN] attrIndex - index of requested attribute.
	@return list where 0 element is attribute name, and 1st - value.
	"""
	RET=[]
	RET.append(self.attrNames[attrIndex])
	RET.append(self.attrValues[attrIndex])
	return RET
#==============================================================================
    def addAttr(self, attrName, attrValue):
	"""
	@brief add attribute to stream
	
	@param[IN] attrName - name of attribute
	@param[IN] attrValue - value of attribute
	"""
	self.attrNames.append(attrName)
	self.attrValues.append(attrValue)
	return
#==============================================================================    
    def getAttrNumber(self):
	"""
	Get number of attributes.
	@return Attributes number.
	"""
	return len(self.attrNames)
#==============================================================================