#!/usr/local/bin/python

import ogg2
import struct

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
      return self.pb.reset()

def ilog(num) :
   for x in range(17) :
      if 2**x >= num+1 :
         return x
   return 0


serialno = 5
gnum = 1
gden = 1
sclx = 4000
scly = 270
bitx = ilog(sclx)
bity = ilog(scly)
totl = (bitx*2)+(bity*2)+4
bitp = ((((totl-1)/8)+1)*8)-totl
print totl
print bitp

langs=(('en','English'),('es','Spanish')) 

texts=((05,10,1,2,3,1,3,3, ( u'Hello World!' ,
                         u'Hola, Mundo!' ) ),
       (12,15,5,6,7,1,3,3, ( u'It\'s a beautiful day to be born.' ,
                         u'Es un d\N{LATIN SMALL LETTER I WITH ACUTE}'+\
                         u'a hermoso para que se llevar'+\
                         u'\N{LATIN SMALL LETTER A WITH ACUTE}.' ) ) )

bp = BitPacker()    
os=ogg2.OggStreamState(5)
oy=ogg2.OggSyncState()
fd=open('test.writ.ogg','w')


# Start with Header
bp.write(0,8)				# header packet
bp.write(1953067639,32)			# "writ"
bp.write(0,8)				# version = 0
bp.write(gnum,32)			# granulerate_numerator
bp.write(gden,32)			# granulerate_denominator
bp.write(sclx,16)			# location_scale_x
bp.write(scly,16)			# location_scale_y
bp.write(len(langs)-1,8)		# num_languages
for lang in langs:
   bp.uniwrite(lang[0])			# lang_name length, string
   bp.uniwrite(lang[1])			# lang_desc length, string

packet = bp.export()
packet.bos = 1
os.packetin(packet)
page=os.flush()
pn=1
oy.pagein(page)
oy.output(fd)

bp = BitPacker()    
# bp.reset()

for text in texts:			# - Each Phrase -
   bp.write(255,8)			# data packet
   bp.write(text[0],64)			# granule_start
   bp.write(text[1],32)			# granule_duration
   bp.write(text[2],bitx)		# location_x
   bp.write(text[3],bity)		# location_y
   bp.write(text[4],bitx)		# location_width
   bp.write(text[5],bity)		# location_height
   bp.write(text[6],2)			# alignment_x
   bp.write(text[7],2)			# alignment_y
   bp.write(0,bitp)			# byte padding
   for lang in range(len(langs)):
      bp.uniwrite(text[8][lang])	# text length, string

   packet = bp.export()
   packet.packetno = pn
   packet.granulepos = text[0]+text[1]
   os.packetin(packet)
   pn=pn+1
   page=os.pageout()
   if page != None :   
      oy.pagein(page)
      oy.output(fd)
   bp = BitPacker()    
   #bp.reset()

page=os.flush()
if page != None :   
   oy.pagein(page)
   while oy.output(fd) :
      pass

