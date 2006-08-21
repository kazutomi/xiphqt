## \file streams.py
# Documentation for streams module of Souffleur project.
# \author Maxim Litvinov (aka DarakuTenshi) <otaky@ukr.net>

## Stream class
# Class for geting info on stream
class Stream:
    ## Constructor
    def __init__(self):
        self.MIME=None
        self.Name=None
        self.attrs={}
        self.ID=None
    
    ## Add attribute.
    # Add attribute on the stream.
    # \param attrName - name of the attribute.
    # \param attrValue - value of the attribute.
    def addAttr(self, attrName, attrValue):
        if not attrName or not attrValue:
            return

        self.attrs[attrName]=attrValue

    ## \var MIME
    # MIME type of the stream.
    
    ## \var Name
    # Stream name in the GStreamer.
    
    ## \var attrs
    # List of the attributes.
    
    ## \var ID
    # ID of the stream in Souffleur current project.


## Media class.
# Class for geting info on media.
class Media:
    ## Constructor
    def __init__(self):
        self.MIME=None
        self.source=None
        self.sourceURI=None
        self.Streams=[]
        self.lastID=0

    ## Add stream.
    # Add stream to the media.
    # \param stream - the stream to add.
    def addStream(self, stream):
        if type(stream)!=type(Stream()):
            return
        if stream.ID <= self.lastID:
            return
        self.Streams.append(stream)
        self.lastID=stream.ID

    ## \var MIME
    # MIME type of the media.
    
    ## \var source
    # Source file name of the media.
    
    ## \var sourceURI
    # Source URO of the media.
    
    ## \var Streams
    # List of the stream of media.
    
    ## \var lastID
    # Last used ID number in the media.
