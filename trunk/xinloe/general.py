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

#
# We'll import all external modules here
#
import os
import wx
import ogg2
import time, string
from wxPython.wx import *
from wxPython.lib.scrolledpanel import wxScrolledPanel

def geticon(name, size=1):
  sizes=('16x16','22x22','32x32','48x48')
  f = 'icons/%s/%s.png' % (sizes[size], name)
  if not os.path.exists(f) :
    f = 'icons/%s/%s.png' % (sizes[size], 'unknown')
  return wxImage(f, wxBITMAP_TYPE_PNG).ConvertToBitmap()

def gettext(parent, string, size=1):
  text = wxStaticText(parent, -1, string)
  font = text.GetFont()
  font.SetPointSize(font.GetPointSize()+size)
  text.SetFont(font)
  return text

def timestr(seconds):
  hours   = seconds/3600
  minutes = (seconds-( hours*3600 ))/60
  seconds = (seconds-((hours*3600)+(minutes*60)))
  return '%s:%s:%s' % \
   (str(hours).zfill(2), str(minutes).zfill(2), str(seconds).zfill(2))

def ratestr(bytes, seconds):
  if seconds == 0 : return '0bps'
  bps = (bytes * 8.0) / seconds
  if bps > 1073741823:
    return '%dgbps' % round(bps/1073741824,2)
  if bps > 1048575 :
    return '%dmbps' % round(bps/1048576,1)
  if bps > 1023 : 
    return '%dkbps' % round(bps/1024,0)
  return '%dbps' % round(bps,0)

def bytestr(bytes):
  if bytes > 1073741823:
    return '%dgb' % round(bytes/1073741824,2)
  if bytes > 1048575 :
    return '%dmb' % round(bytes/1048576,1)
  if bytes > 1023 : 
    return '%dkb' % round(bytes/1024,0)
  return '%db' % round(bytes,0)

