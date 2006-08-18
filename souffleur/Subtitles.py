import os
import string

SUB_NONE=0
SUB_SRT=1

class Sub:
    def __init__(self):
        self.text=""
        self.start_time=None
        self.end_time=None
        self.subType=SUB_NONE
        self.Attributes=None
#==============================================================================
    def isInTime(self, time):
        if( (time>=self.start_time) and (time<=self.end_time) ):
            return 1
        else:
            return 0

#==============================================================================
#==============================================================================
class Subtitles:
    def __init__(self):
        self.subs={}
        self.subSource=None
        self.subType=SUB_NONE
        self.subKeys=[]
#==============================================================================
    def subLoad(self, fileName, ID):
        FILE=os.open(fileName, os.O_RDONLY)
        FS=os.fstat(FILE)
        DATA=os.read(FILE,FS.st_size)
        os.close(FILE)

        self._subSRTLoadFromString(DATA)

        self.subSource=ID
#==============================================================================    
    def subSave(self, FN, format):
        if (self.subSource!=None):
            FUN=os.open(FN,os.O_WRONLY|os.O_CREAT|os.O_TRUNC)
            N=1
            for i in self.subKeys:
                SUB = self.subs[int(i)]
                Text=str(N)+"\r\n"
                Hour, Min, Sec, MSec = self._subTime2SRTtime(SUB.start_time)
                #Text+=str(Hour)+":"+str(Min)+":"+str(Sec)+","+str(MSec)
                Text+="%02d:%02d:%02d,%03d"%(Hour, Min, Sec, MSec)
                Text+=" --> "
                Hour, Min, Sec, MSec = self._subTime2SRTtime(SUB.end_time)
                #Text+=str(Hour)+":"+str(Min)+":"+str(Sec)+","+str(MSec)+"\n"
                Text+="%02d:%02d:%02d,%03d"%(Hour, Min, Sec, MSec)+"\r\n"
                Text+=SUB.text+"\r\n"
                if (SUB.text[-2]!="\r\n"):
                    Text+="\r\n"
                os.write(FUN, Text)
                N+=1
            os.close(FUN)
#==============================================================================
    def _subTime2SRTtime(self, time):
        tTime = time
        MSec = tTime%1000
        tTime /=1000
        Sec = tTime%60
        tTime /= 60
        Min = tTime%60
        Hour = tTime/60
        return Hour, Min, Sec, MSec
#==============================================================================
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
            Text=Text[0:-1]
            ST=int(Timing[0:2])*3600000+int(Timing[3:5])*60000+int(Timing[6:8])*1000+int(Timing[9:12])
            ET=int(Timing[17:19])*3600000+int(Timing[20:22])*60000+int(Timing[23:25])*1000+int(Timing[26:29])
            
            TS=Sub()
            TS.text=Text
            TS.start_time=ST
            TS.end_time=ET
            TS.subType=self.subType
            self.subs[int(ST)]=TS
        self.updateKeys()
#==============================================================================    
    def subDel(self, time):
        del self.subs[time]
        self.updateKeys()
 #==============================================================================   
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
#==============================================================================    
    def updateKeys(self):
        self.subKeys=self.subs.keys()
        self.subKeys.sort()
#==============================================================================
    def subUpdate(self, upSubKey):
        Sub = self.subs[upSubKey]
        self.subDel(upSubKey)
        self.subAdd(Sub.start_time, Sub.end_time, Sub.text, Sub.Attributes, 1)
#==============================================================================
    def getSub(self, time):
        i=0
        for i in self.subKeys:
            if(time>=i):
                if(self.subs[i].isInTime(time)==1):
                    return self.subs[i]
            else:
                return None
        return None
