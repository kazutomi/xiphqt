'''
  Xinloe -- A Python-Based Non-Linear Ogg Editor
  Copyright (C) 2004 Arc Riley <arc@Xiph.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

'''

class GenCodec:
  def __init__(self, header):
    self.name = ''
    self.desc = ''
    self.icon = ''
    self.length = 0
    self.bytes = 0
    self.version = 0

  def PageIn(self, page):
    self.bytes = self.bytes + len(page)

class Vorbis(GenCodec):
  def __init__(self, header):
    GenCodec.__init__(self, '')
    if header[:7] == '\x01vorbis':
      self.icon = 'vorbis'
      if header[7:11] == '\x00\x00\x00\x00' :
        self.name = 'Vorbis I'
        self.desc = 'General Purpose Audio'
        self.version = 0
        self.channels = ord(header[11])
        self.samplerate = ord(header[12]) + (ord(header[13])*256) + \
                  (ord(header[14])*65536) + (ord(header[15])*16777216)
      else :
        self.name = 'Vorbis (Unsupported Version)'
        self.version = chr(ord(header[7]) + (ord(header[8])*256) + \
                   (ord(header[9])*65536) + (ord(header[10])*16777216))
        self.samplerate = 0

  def PageIn(self, page):
    self.bytes = self.bytes + len(page)
    if self.samplerate > 0 :
      self.length = page.granulepos / self.samplerate

    
class Theora(GenCodec):
  def __init__(self, header):
    GenCodec.__init__(self, '')
    if header[:7] == '\x80theora':
      self.icon = 'theora'
      if header[7:9] == '\x03\x02' :
        self.name = 'Theora I'
        self.desc = 'General Purpose Video'
        self.version = '3.2.'+str(ord(header[9]))
        self.size = str(ord(header[16]) + (ord(header[15])*256) + \
                        (ord(header[14])*65536)) + 'x' + \
                    str(ord(header[19]) + (ord(header[18])*256) +
                        (ord(header[17])*65536))
        n = ord(header[25]) + (ord(header[24])*256) + \
            (ord(header[23])*65536) + (ord(header[22])*16777216)
        d = ord(header[29]) + (ord(header[28])*256) + \
            (ord(header[27])*65536) + (ord(header[26])*16777216)
        self.framerate = float(n)/float(d)
        self.b = ord(header[39]) + (ord(header[38])*256) + \
                 (ord(header[37])*65536)
        self.q = int(round((((ord(header[40]) & 252) >> 2) / 6.3)))
        self.keyshift = ((ord(header[40]) & 3) << 3)+\
                        ((ord(header[41]) & 224) >> 5)
     
      else :
        self.name = 'Theora (Unsupported Version)'
        self.version = str(ord(header[7])) + '.' + \
                       str(ord(header[8])) + '.' + \
                       str(ord(header[9])) 
        self.framerate = 0

  def PageIn(self, page):
    self.bytes = self.bytes + len(page)
    if self.framerate > 0 :
      self.length = int((page.granulepos>>self.keyshift) / self.framerate)

class Speex(GenCodec):
  def __init__(self, header):
    GenCodec.__init__(self, '')
    if header[:8] == 'Speex   ':
      self.icon = 'speex'
      self.version = header[8:28]
      if header[28:32] == '\x01\x00\x00\x00' :
        self.name = 'Speex I'
        self.desc = 'Low Bitrate Voice'
        self.samplerate = ord(header[36]) + (ord(header[37])*256) + \
                  (ord(header[38])*65536) + (ord(header[39])*16777216)
        if header[40] == '\x00' :
          self.q = 'NarrowBand'
        elif header[40] == '\x01' :
          self.q = 'WideBand'
        elif header[40] == '\x02' :
          self.q = 'UltraWideBand'
        else : self.q = 'Unknown'
        self.channels = ord(header[48])
        if header[52:56] == '\xff\xff\xff\xff' :
          self.b = 0
        else :
          self.b = ord(header[52]) + (ord(header[53])*256) + \
           (ord(header[54])*65536) + (ord(header[55])*16777216)
        self.framesize = ord(header[56]) + (ord(header[57])*256) + \
                 (ord(header[58])*65536) + (ord(header[59])*16777216)
      else :
        self.name = 'Speex (Unsupported Version)'
        self.version = str(ord(header[28]) + (ord(header[29])*256) + \
                   (ord(header[30])*65536) + (ord(header[31])*16777216))
        self.samplerate = 0

  def PageIn(self, page):
    self.bytes = self.bytes + len(page)
    if self.samplerate > 0 :
      self.length = page.granulepos / self.samplerate

class FLAC(GenCodec):
  def __init__(self, header):
    GenCodec.__init__(self, '')
    if header[:4] == 'fLaC':
      self.name = 'FLAC'
      self.desc = 'Lossless Audio'
      self.icon = 'flac'
      self.version = '0'
      # need samplerate, channels, samplesize, and bitrate

class Writ(GenCodec):
  def __init__(self, header):
    GenCodec.__init__(self, '')
    if header[:5] == '\x00writ':
      if header[5] == '\x01' :
        self.name = 'Writ I'
        self.desc = 'Timed Text Phrases'
        self.icon = ''
        self.version = '1.'+str(ord(header[6]))
        n = ord(header[7]) + (ord(header[8])*256) + \
            (ord(header[9])*65536) + (ord(header[10])*16777216)
        d = ord(header[11]) + (ord(header[12])*256) + \
            (ord(header[13])*65536) + (ord(header[14])*16777216)
        self.granulerate = float(n)/float(d)
      else :
        self.name = 'Writ (Unsupported Version)'
        self.version = str(ord(header[5]))
        self.granulerate = 0

  def PageIn(self, page):
    self.bytes = self.bytes + len(page)
    if self.granulerate > 0 :
      self.length = int(page.granulepos / self.granulerate)

codecs = (Vorbis, Theora, Speex, FLAC, Writ, GenCodec)


