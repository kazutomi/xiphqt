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
import time
from wxPython.wx import *
from wxPython.lib.scrolledpanel import wxScrolledPanel

def geticon(name, size=1):
  sizes=('16x16','22x22','32x32','48x48')
  f = 'icons/%s/%s.png' % (sizes[size], name)
  return wxImage(f, wxBITMAP_TYPE_PNG).ConvertToBitmap()

def gettext(parent, string, size=1):
  text = wxStaticText(parent, -1, string)
  font = text.GetFont()
  font.SetPointSize(font.GetPointSize()+size)
  text.SetFont(font)
  return text


