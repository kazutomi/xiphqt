## \file Subtitles.py
# Documentation for subtitles module of Souffleur project.
# \author Maxim Litvinov (aka DarakuTenshi) <otaky@ukr.net>
# \todo Add support of different subtitles format.

import os
import string

SUB_NONE=0
SUB_SRT=1

## Sub class.
# The Sub class, tha handle subtitle
class Sub:

    ## Constructor
    def __init__(self):
        self.text=""
        self.start_time=None
        self.end_time=None
        self.Attributes=None

#==============================================================================
    ## Check subtitle time.
    # This function check if subtitle visibility in given time.
    # \param[in] time - time to check
    # \return 1 - if visibility in time, 0 - otherwise
    def isInTime(self, time):
        if( (time>=self.start_time) and (time<=self.end_time) ):
            return 1
        else:
            return 0

    ## \var text
    # A variable to store subtitle text
    
    ## \var start_time
    # A variable to store a start time of visibility of subtitle (in ns).
    
    ## \var end_time
    # A variable to store a end time of visibility of subtitle (in ns).
    
    ## \var Attributes
    # A array of attributes of subtitle. (NOT USED YET)

#==============================================================================

## Subtitles calss
# Class for subtitles stuff (load, save, get...)
class Subtitles:
    
    ## Constructor
    def __init__(self):
        self.subs={}
        self.subSource=None
        self.subKeys=[]

#==============================================================================
    ## Load subtitles.
    # Load subtitles from file whith associated stream ID.
    # \param fileName - name of subtitles file.
    # \param ID - stream ID.
    def subLoad(self, fileName, ID):
        FILE=os.open(fileName, os.O_RDONLY)
        FS=os.fstat(FILE)
        DATA=os.read(FILE,FS.st_size)
        os.close(FILE)

        self._subSRTLoadFromString(DATA)

        self.subSource=ID

#==============================================================================
    ## Save subtitles.
    # Save subtitles to the file.
    # \param FN - file name.
    # \param format - the store format of subtitles. (NOT USED YET)
    def subSave(self, FN, format):
        if (self.subSource!=None):
            FUN=os.open(FN,os.O_WRONLY|os.O_CREAT|os.O_TRUNC)
            N=1
            for i in self.subKeys:
                SUB = self.subs[int(i)]
                Text=str(N)+"\r\n"
                Hour, Min, Sec, MSec = self._subTime2SRTtime(SUB.start_time)
                Text+="%02d:%02d:%02d,%03d"%(Hour, Min, Sec, MSec)
                Text+=" --> "
                Hour, Min, Sec, MSec = self._subTime2SRTtime(SUB.end_time)
                Text+="%02d:%02d:%02d,%03d"%(Hour, Min, Sec, MSec)+"\r\n"
                Text+=SUB.text+"\r\n"
                if (SUB.text[-2]!="\r\n"):
                    Text+="\r\n"
                os.write(FUN, Text)
                N+=1
            os.close(FUN)

#==============================================================================
    ## Convert subtitle time to SRT format.
    # Convert subtitle time for saving in SRT subtitles file.
    # \param time - subtitle time.
    # \return list of: hour, minute, second and milisecond
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
    ## Load SRT formated subtitles.
    # Load SRT formated subtitles from given string.
    # \param DATA - string of SRT subtitles.
    def _subSRTLoadFromString(self, DATA):
        if (string.find(DATA, "\r\n")==-1):
            DATA=string.split(DATA,"\n")
        else:
            DATA=string.split(DATA,"\r\n")
        i=0
        while(i<len(DATA)):
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
            self.subs[int(ST)]=TS
        self.updateKeys()

#==============================================================================    
    ## Delete subtitle.
    # Delete subtitle from subtitles array.
    # \param time - key of subtitle in "subs" list.
    def subDel(self, time):
        del self.subs[time]
        self.updateKeys()
        
#==============================================================================
    ## Add subtitle.
    # Add subtitle to the "subs" list.
    # \param STime - start time of the subtitle.
    # \param ETime - end time of the subtitle.
    # \param Attrs - attributes of the subtitle.
    # \param isUpdate - to update (or not) keys array of "subs" list.
    def subAdd(self, STime, ETime, Text, Attrs, isUpdate=0):
        TS=Sub()
        TS.text=Text
        TS.start_time=STime
        TS.end_time=ETime
        TS.Attributes=Attrs
        self.subs[int(STime)]=TS
        if isUpdate==1:
            self.updateKeys()

#==============================================================================    
    ## Update keys array.
    # Update array of "subs" keys.
    def updateKeys(self):
        self.subKeys=self.subs.keys()
        self.subKeys.sort()

#==============================================================================
    ## Update subtitle.
    # Update subtitle key.
    # \param upSubKey - subtitle to update.
    def subUpdate(self, upSubKey):
        Sub = self.subs[upSubKey]
        self.subDel(upSubKey)
        self.subAdd(Sub.start_time, Sub.end_time, Sub.text, Sub.Attributes, 1)

#==============================================================================
    ## Get subtitle.
    # Get subtitle with given time of visibility.
    # \param time - time of requested subtitle.
    # \return subtitle or "None".
    def getSub(self, time):
        i=0
        for i in self.subKeys:
            if(time>=i):
                if(self.subs[i].isInTime(time)==1):
                    return self.subs[i]
            else:
                return None
        return None

    ## \var subs
    # List of loaded subtitles.
    
    ## \var subSource
    # The source of subtitle 
    
    ## \var subKeys
    # Array of "subs" keys (hashs)
