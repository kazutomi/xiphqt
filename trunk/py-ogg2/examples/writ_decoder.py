'''
  function: Ogg Writ reference encoder
  last mod: $Id: writ_decoder.py,v 1.2 2003/12/01 07:18:51 arc Exp $

This is an example for how py-ogg2 can be used to rapidly design a new
Ogg codec or test an existing codec's specifications for accuracy.

'''

import ogg2
from struct import unpack

def ilog(num) :
   for x in range(17) :
      if 2**x >= num+1 :
         return x
   return 0

# Grab 'em pages!
op = ogg2.OggSyncState()
infile = open('test.writ.ogg','r')
while op.input(infile) :
   pass
pages = []
while 1 :
   page=op.pageout()
   if page == None :
      break
   else :
      pages.append(page)

# Churn 'em into packets!
os = ogg2.OggStreamState(pages[0].serialno)
for page in pages :
   os.pagein(page)

while 1 :
   packet = os.packetout()
   if packet == None :
      break
   else :
      ob=ogg2.OggPackBuff(packet)
      if ob.read(8)==0 :
         if ob.read(32) == 1953067639 :
            version = ob.read(8)
            print 'Version: %d' % version
            gnum = ob.read(32)
            gdom = ob.read(32)
            print 'Granulerate: %d/%d' % (gnum, gdom)
            lscx = ob.read(16)
            lscy = ob.read(16)
            bitx = ilog(lscx)
            bity = ilog(lscy)
            totl = (bitx*2)+(bity*2)+4
            bitp = ((((totl-1)/8)+1)*8)-totl
            lnum = ob.read(8)
            langs = []
            print 'langs =', lnum
            for l in range(lnum+1) :
               llen = ob.read(8)
               data = ''
               for a in range(llen) :
                  data = data + chr(ob.read(8))
               lname = unicode(data)
               llen = ob.read(8)
               data = ''
               for a in range(llen) :
                  data = data + chr(ob.read(8))
               ldesc = unicode(data)
               langs.append((lname,ldesc))
               print 'Language: %s (%s)' % (langs[l][0], langs[l][1])
            print ''
         else :
            print 'Non-Writ Packet?'
            continue
      else :
         print 'Phrase:'
         gstart = ob.read(32) + (ob.read(32)*4294967296)
         gdurat = ob.read(32)
         print '   Time: %d - %d' % (gstart, gstart+gdurat)   
         locx = ob.read(ilog(lscx))
         locy = ob.read(ilog(lscy))
         print '   Location: %d,%d' % (locx, locy)
         locw = ob.read(ilog(lscx))
         loch = ob.read(ilog(lscy))
         print '   Size: %d,%d' % (locw, loch)
         alix = ob.read(2)
         aliy = ob.read(2)
         print '   Alignment: %d,%d' % (alix, aliy)
         ob.read(bitp)
         for l in range(lnum+1) :
            tlen = ob.read(8)
            data = u''
            for a in range(tlen) :
               inp = ob.read(8)
               if inp == None : 
                  print 'ERROR: Premature end of packet.'
                  break
               data = data + unichr(inp)
            print (u'   '+langs[l][0]+u': '+data).encode('latin-1')
         print ''
         
