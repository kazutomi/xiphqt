'''
  function: Ogg ReSerializer
  last mod: $Id: reserializer.py

This is a useful tool for changing the serial number of an Ogg stream.
Why is it useful?  Because you can't chain two logical streams together 
in the same physical stream which have the same serial numbers. 

'''

import ogg2
import random

infilename = raw_input('Input File: ')
outfilename = raw_input('Output File: ')

inf=open(infilename, 'r')
otf=open(outfilename, 'w')

syncin=ogg2.OggSyncState()
syncout=ogg2.OggSyncState()

serials = {}

while 1: # While there's data to input
  if syncin.input(inf) == 0 : break
  while 1: # While there are pages to output
    page = syncin.pageout()
    if page==None : break
    if page.bos :
      serials[page.serialno] = int(random.random()*2147483647)
      print 'ReSerializing stream %d to %d' % (page.serialno,
                                               serials[page.serialno])
    if serials.has_key(page.serialno) :
      page.serialno = serials[page.serialno]
      syncout.pagein(page)
      while 1:
        if syncout.output(otf) == 0 : break
    else :
      print 'Headless stream %d found, skipping' % page.serialno

print "Done."
