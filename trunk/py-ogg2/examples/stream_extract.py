'''

This example displays the bitstreams found in an Ogg file and allows the
user to select one for extraction into a seperate file.

by Arc Riley <arc@xiph.org>

'''

import sys

try:
   import ogg2
except:
   print 'Could not import ogg2 module.'
   sys.exit(1)


def getserials(f) :
   streams = []

   def testpage(page) :
      if page.bos :
         try :
            streams.index(page.serialno)
         except ValueError :
            print len(streams), page.serialno
            streams.append(page.serialno)      

   sync = ogg2.OggSyncState()
   while sync.input(f) :
      while 1 :
         page = sync.pageout()
         if page == None : break
         testpage(page)
   return streams

def chooseprompt(streams) :
   while 1:
      choice = raw_input('Index: ')
      if choice.isdigit() :
         choice = int(choice) 
         if choice>-1 and choice<len(streams) : 
            break
         else :
            print 'Not in range (0 - %d).' % (len(streams)-1,)
      else :
         print 'Numbers only.'
   return streams[choice]

def extractit(cserial, f_in, f_out) :
   print "Extracting serialno %d" % cserial
   syncin = ogg2.OggSyncState()
   syncout = ogg2.OggSyncState()
   f_in.seek(0)
   while syncin.input(f_in) :
      while 1 :
         page = syncin.pageout()
         if page == None : break
         if page.serialno == cserial :
            syncout.pagein(page)
            while syncout.output(f_out) : pass
   print 'done.'   


def main() :
   import getopt, string
   try:
      opts, args = getopt.getopt(sys.argv[1:], 'v')
   except getopt.error:
      sys.stderr.write('Usage: ' + sys.argv[0] + ' infile outfile\n')
      sys.exit(1)
   if len(args) != 2 :
      sys.stderr.write('Usage: ' + sys.argv[0] + ' infile outfile\n')
      sys.exit(1)
   try:
      f_in = open(args[0],'r')
   except:
      sys.stderr.write('%s could not be opened for input.' % args[0])
      sys.exit(1)
   try:
      f_out = open(args[1],'w')
   except:
      sys.stderr.write('%s could not be opened for output.' % args[1])
      sys.exit(1)

   # Serial execution, these are seperate to make it easier to read   
   extractit(chooseprompt(getserials(f_in)), f_in, f_out)

   f_in.close()
   f_out.close()
         

if __name__ == '__main__' :
   main()

