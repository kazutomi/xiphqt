import os
import string
#from datetime import time
#from array import array

SUB_NONE=0
SUB_SRT=1

class Sub:
    def __init__(self):
        self.text=""
        self.start_time=None
        self.end_time=None
        self.subType=SUB_NONE
        self.Attributes=None

    def isInTime(self, time):
        if( (time>=self.start_time) and (time<=self.end_time) ):
            return 1
        else:
            return 0


class Subtitles:
    def __init__(self):
        self.subs={}
        self.subSource=None
        self.subType=SUB_NONE
        self.subKeys=[]

    def subLoad(self, fileName):
        FILE=os.open(fileName, os.O_RDONLY)
        FS=os.fstat(FILE)
        DATA=os.read(FILE,FS.st_size)
        os.close(FILE)

        self._subSRTLoadFromString(DATA)

        self.subSource=fileName

    def _subSRTLoadFromString(self, DATA):
        self.subType=SUB_SRT
        if (string.find(DATA, "\r\n")==-1):
            DATA=string.split(DATA,"\n")
        else:
            DATA=string.split(DATA,"\r\n")
        i=0
        while(i<len(DATA)):
            #i=i+1
            if(i>=len(DATA)):
                break
            N = DATA[i]
            i+=1
            if(i>=len(DATA)):
                break
            Timing = DATA[i]
            Text="";
            i+=1
            if(i>=len(DATA)):
                break
            while(DATA[i]!=""):
                Text=Text+DATA[i]+"\n"
                i+=1
            i+=1
            
            ST=int(Timing[0:2])*3600000+int(Timing[3:5])*60000+int(Timing[6:8])*1000+int(Timing[9:12])
            ET=int(Timing[17:19])*3600000+int(Timing[20:22])*60000+int(Timing[23:25])*1000+int(Timing[26:29])
            
            TS=Sub()
            TS.text=Text
            TS.start_time=ST
            TS.end_time=ET
            TS.subType=self.subType
            self.subs[int(ST)]=TS
        self.updateKeys()
    
    def subDel(self, time):
        del self.subs[time]
        self.updateKeys()
    
    def subAdd(self, STime, ETime, Text, Attrs, isUpdate=0):
        TS=Sub()
        TS.text=Text
        TS.start_time=STime
        TS.end_time=ETime
        TS.subType=self.subType
        TS.Attributes=Attrs
        self.subs[int(STime)]=TS
        if isUpdate==1:
            self.updateKeys()
    
    def updateKeys(self):
        self.subKeys=self.subs.keys()
        self.subKeys.sort()

    def getSub(self, time):
        i=0
        for i in self.subKeys:
            if(time>=i):
                if(self.subs[i].isInTime(time)==1):
                    return self.subs[i]
            else:
                return None
        return None
