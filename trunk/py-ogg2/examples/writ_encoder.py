'''
  function: Ogg Writ reference encoder
  last mod: $Id: writ_encoder.py,v 1.2 2004/02/24 06:31:49 arc Exp $

This is an example for how py-ogg2 can be used to rapidly design a new 
Ogg codec or test an existing codec's specifications for accuracy.

'''

import ogg2
import struct
import random

class BitPacker :
   def __init__(self) :
      self.pb = ogg2.OggPackBuff()

   def uniwrite(self,ustring) :
      total = 0
      output=[]
      for pos in range(len(ustring)) :
         value = ord(ustring[pos])      
         if value < 256 :
            size = 8
         elif value < 65536 :
            size = 16         
         elif value < 16777216 :
            size = 24         
         else :
            size = 32
         total = total + (size/8)
         output.append((value,size))
      if total>255 :
         raise('ValueError: Unicode must be no greater than 255 bytes long')
      self.pb.write(total,8)
      for char in output :
         self.pb.write(char[0],char[1])

   def write(self,value,size) :
      if size>32 :
         hi=value/4294967296
         lo=value-(hi*4294967296)
         self.pb.write(lo,32)
         self.pb.write(hi,size-32)
      else :
         self.pb.write(value,size)

   def export(self) :
      return self.pb.packetout()      

   def reset(self) :
      self.pb = ogg2.OggPackBuff()
      # return self.pb.reset()

class OggStream :
   def __init__(self) :
      self.os = ogg2.OggStreamState(random.randrange(2**24))
      self.oy = ogg2.OggSyncState()
      self.fd = open('test.writ.ogg','w')
      self.pn = 0

   def packetin(self, packet, granulepos=0) :
      packet.packetno = self.pn
      packet.granulepos = granulepos
      if self.pn == 0 :
         packet.bos = 1
      else :
         self.os.packetin(self.packet)
         if self.pn != 2 :
            print "Flushing: %d" % (self.packet.packetno)
            self.flush()
         else : 
            print "Skipping: %d" % (self.packet.packetno)
      self.packet = packet
      self.pn = self.pn + 1

   def flush(self) :
      page = self.os.flush()
      if page != None :
         self.oy.pagein(page)
         output = '.'
         while len(output)!= 0 :
            output = self.oy.read(4096)
            print type(output), len(output)
            self.fd.write(output)
      else : print 'None!'

   def close(self) :
      self.packet.eos = 1
      self.os.packetin(self.packet)
      self.flush()
      self.fd.close()
      

def ilog(num) :
   for x in range(17) :
      if 2**x >= num+1 :
         return x
   return 0


gnum = 1
gden = 1

langs=(('en','English'),('es','Spanish')) 

# winds: location_x, location_y, width, height, alignment_x, alignment_y
sclx = 4000
scly = 270
bitx = ilog(sclx)
bity = ilog(scly)
totl = (bitx*2)+(bity*2)+4
bitp = ((((totl-1)/8)+1)*8)-totl
winds=((1,2,3,1,3,3),(5,6,7,1,3,3))

# texts: start, duration, lang(s), wind
texts=((05,10, ( u'Hello World!' ,
                 u'Hola, Mundo!' ), 0),
       (12,15, ( u'It\'s a beautiful day to be born.' ,
                 u'Es un d\N{LATIN SMALL LETTER I WITH ACUTE}'+\
                 u'a hermoso para que se llevar'+\
                 u'\N{LATIN SMALL LETTER A WITH ACUTE}.' ), 1) )

bp = BitPacker()    
os = OggStream()

# Start with Header 0
bp.write(0,8)				# header packet 0
bp.write(1953067639,32)			# "writ"
bp.write(0,8)				# version = 0
bp.write(2,8)				# subversion = 0
bp.write(gnum,32)			# granulerate_numerator
bp.write(gden,32)			# granulerate_denominator
os.packetin(bp.export())
bp.reset()

# Start with Header 1
bp.write(1,8)				# header packet 1
bp.write(1953067639,32)			# "writ"
bp.write(len(langs)-1,8)		# num_languages
for lang in langs:
   bp.uniwrite(lang[0])			# lang_name length, string
   bp.uniwrite(lang[1])			# lang_desc length, string
os.packetin(bp.export())
bp.reset()

# Start with Header 1
bp.write(2,8)				# header packet 2
bp.write(1953067639,32)			# "writ"
bp.write(sclx,16)			# location_scale_x
bp.write(scly,16)			# location_scale_y
bp.write(len(winds),8)			# num_windows
for win in winds:
   bp.write(win[0],bitx)		# location_x
   bp.write(win[1],bity)		# location_x
   bp.write(win[2],bitx)		# location_width
   bp.write(win[3],bity)		# location_height
   bp.write(win[4],2)			# alignment_x
   bp.write(win[5],2)			# alignment_y
os.packetin(bp.export())
bp.reset()
   
for text in texts:			# - Each Phrase -
   bp.write(255,8)			# data packet
   bp.write(text[0],64)			# granule_start
   bp.write(text[1],32)			# granule_duration
   for lang in range(len(langs)):
      bp.uniwrite(text[2][lang])	# text length, string
   if len(winds) > 1 :			# only if we need to specify
      bp.write(text[3],8)		# window_id   
   os.packetin(bp.export(), text[0])
   bp.reset()

os.close()

