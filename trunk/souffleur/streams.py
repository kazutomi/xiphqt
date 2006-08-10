class Stream:
    def __init__(self):
        self.MIME=None
        self.Name=None
        self.attrs={}
        self.ID=None
    
    def addAttr(self, attrName, attrValue):
        if not attrName or not attrValue:
            return

        self.attrs[attrName]=attrValue

class Media:
    def __init__(self):
        self.MIME=None
        self.source=None
        self.sourceURI=None
        self.Streams=[]
        self.lastID=0

    def addStream(self, stream):
        if type(stream)!=type(Stream()):
            return
        if stream.ID <= self.lastID:
            return
        self.Streams.append(stream)
        self.lastID=stream.ID
