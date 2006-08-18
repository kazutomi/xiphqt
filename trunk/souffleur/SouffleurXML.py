import xml.dom.minidom

from streams import Media
from streams import Stream

from Subtitles import Sub
from Subtitles import Subtitles

class ProjectXML:
    def __init__(self):
        self.impl = xml.dom.minidom.getDOMImplementation()
        self.doc = self.impl.createDocument(None, "souffleur", None)
        self.root = self.doc.documentElement

        rootAttr= self.doc.createAttribute("type")
        rootAttr.nodeValue="project"

        self.root.setAttributeNode(rootAttr)

        versionEl = self.doc.createElement("version")
        versionTxt = self.doc.createTextNode("0")
        versionEl.appendChild(versionTxt)
        self.root.appendChild(versionEl)

        self.head = None
        self.body = None
        self.version = 0

    def load(self, fileName):
        self.root = None
        self.head = None
        self.body = None
        self.version = None
        self.doc = xml.dom.minidom.parse(fileName)
        if self.doc.documentElement.nodeName != "souffleur":
            return None
        self.root=self.doc.documentElement
        for i in self.root.childNodes:
            if i.nodeName=="head":
                self.head=i
            elif i.nodeName=="body":
                self.body=i
            elif i.nodeName=="version":
                self.version = i.childNodes[0].nodeValue
        return self

    def write(self, fileName):
        HDLR=file(fileName, "w")
        self.doc.writexml(HDLR)
        HDLR.close()

    def addHeadInfo(self, attrName, attrValue):
        if not self.head:
            self.head=self.doc.createElement("head")
        self.root.appendChild(self.head)

        if not attrName or not attrValue:
            return

        attrEl = self.doc.createElement(attrName)
        attrTxt = self.doc.createTextNode(attrValue)
        attrEl.appendChild(attrTxt)
        self.head.appendChild(attrEl)

    def addMedia(self, media):
        if not media:
            return
        if type(media)!=type(Media()):
            return
        if not self.body:
            self.body=self.doc.createElement("body")
            self.root.appendChild(self.body)

        data = self.doc.createElement("data")
        self.body.appendChild(data)

        source = self.doc.createElement("source")
        sType= self.doc.createAttribute("type")
        sType.nodeValue=media.MIME
        source.setAttributeNode(sType)
        sTxt = self.doc.createTextNode(media.source)
        source.appendChild(sTxt)
        data.appendChild(source)

        for i in media.Streams:
            tmpMedia = self.doc.createElement("media")
            data.appendChild(tmpMedia)

            tmpEl = self.doc.createElement("type")
            tmpTxt = self.doc.createTextNode(i.MIME)
            tmpEl.appendChild(tmpTxt)
            tmpMedia.appendChild(tmpEl)

            tmpEl = self.doc.createElement("name")
            tmpTxt = self.doc.createTextNode(i.Name)
            tmpEl.appendChild(tmpTxt)
            tmpMedia.appendChild(tmpEl)

            tmpEl = self.doc.createElement("id")
            tmpTxt = self.doc.createTextNode(str(i.ID))
            tmpEl.appendChild(tmpTxt)
            tmpMedia.appendChild(tmpEl)

            if not i.attrs:
                continue

            attrs = self.doc.createElement("attrs")
            tmpMedia.appendChild(attrs)
            for j in i.attrs.keys():
                tmpEl = self.doc.createElement(j)
                tmpTxt = self.doc.createTextNode(i.attrs[j])
                tmpEl.appendChild(tmpTxt)
                attrs.appendChild(tmpEl)


    def addSubtitle(self, subtitle):
        if not subtitle:
            return
        if type(subtitle)!=type(Subtitles()):
            return
        if not self.body:
            self.body=self.doc.createElement("body")
            self.root.appendChild(self.body)

        data = self.doc.createElement("subtitles")
        self.body.appendChild(data)

        source = self.doc.createElement("source")
        sTxt = self.doc.createTextNode(str(subtitle.subSource))
        source.appendChild(sTxt)
        data.appendChild(source)

        for i in subtitle.subKeys:
            sub = self.doc.createElement("sub")
            tmpEl = self.doc.createElement("start")
            tmpTxt = self.doc.createTextNode(str(subtitle.subs[i].start_time))
            tmpEl.appendChild(tmpTxt)
            sub.appendChild(tmpEl)
            tmpEl = self.doc.createElement("end")
            tmpTxt = self.doc.createTextNode(str(subtitle.subs[i].end_time))
            tmpEl.appendChild(tmpTxt)
            sub.appendChild(tmpEl)
            tmpEl = self.doc.createElement("text")
            tmpTxt = self.doc.createTextNode(str(subtitle.subs[i].text))
            tmpEl.appendChild(tmpTxt)
            sub.appendChild(tmpEl)
            data.appendChild(sub)

    def getHead(self):
        if not self.head:
            return None
        ret={}
        for i in self.head.childNodes:
            ret[i.nodeName]=i.childNodes[0].nodeValue
        return ret

    def getMedia(self):
        if not self.body:
            return None
        ret=[]
        for i in self.body.childNodes:
            if i.nodeName=="data":
                tMedia=Media()
                for j in i.childNodes:
                    if j.nodeName=="source":
                        mType=j.attributes["type"]
                        if not mType:
                            return None
                        tMedia.MIME=mType.nodeValue
                        tMedia.source=j.childNodes[0].nodeValue
                    elif j.nodeName=="media":
                        tStream = Stream()
                        for k in j.childNodes:
                            nodeName = k.nodeName
                            if nodeName == "type":
                                tStream.MIME = k.childNodes[0].nodeValue
                            elif nodeName == "id":
                                tStream.ID = k.childNodes[0].nodeValue
                            elif nodeName == "name":
                                tStream.Name = k.childNodes[0].nodeValue
                            elif nodeName == "attrs":
                                for l in k.childNodes:
                                    tStream.addAttr(l.nodeName, l.childNodes[0].nodeValue)
                            tMedia.addStream(tStream)
                ret.append(tMedia)
        return ret

    def getSubtitle(self):
        if not self.body:
            return None
        ret=[]
        for i in self.body.childNodes:
            if i.nodeName=="subtitles":
                tSubtitles=Subtitles()
                for j in i.childNodes:
                    if j.nodeName=="source":
                        tSubtitles.subSource=j.childNodes[0].nodeValue
                    elif j.nodeName=="sub":
                        tSub=Sub()
                        for k in j.childNodes:
                            nodeName = k.nodeName
                            if nodeName == "start":
                                tSub.start_time=int(k.childNodes[0].nodeValue)
                            elif nodeName == "end":
                                tSub.end_time=int(k.childNodes[0].nodeValue)
                            elif nodeName == "text":
                                tSub.text=str(k.childNodes[0].nodeValue)
                        tSubtitles.subs[tSub.start_time]=tSub
                        tSubtitles.updateKeys()
                ret.append(tSubtitles)
        return ret
