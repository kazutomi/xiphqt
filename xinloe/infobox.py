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

from general import *

class InfoboxPanel(wxScrolledPanel):
  def __init__(self, parent):
    wxScrolledPanel.__init__(self, parent, -1)
    self.infowin = InfoboxScrolledPanel(self)
    self.topsizer = wxBoxSizer(wxVERTICAL)
    self.topsizer.Add(self.infowin, 1, wxEXPAND, 0)
    self.SetSizer(self.topsizer)
    self.SetupScrolling(scroll_x=False)
    self.SetAutoLayout(1)

  def ShowCodec(self, handler):
    self.topsizer.Remove(self.infowin)
    self.infowin.Destroy()
    self.infowin = InfoboxScrolledPanel(self)
    self.topsizer.Add(self.infowin, 1, wxEXPAND, 0)

    bmp = geticon(handler.icon,3)
    logo = wxStaticBitmap(self.infowin, -1, bmp, wxPoint(16, 16),
                          wxSize(bmp.GetWidth(), bmp.GetHeight()))
    title = gettext(self.infowin, ' %s - %s '%(handler.name, handler.desc), 5)
    general = gettext(self.infowin, '%s %s (%s)' % (timestr(handler.length), 
                       ratestr(handler.bytes, handler.length), 
                       bytestr(handler.bytes)), 3)

    topbox = wxBoxSizer(wxHORIZONTAL)
    topbox.Add(logo, 0, wxALIGN_LEFT, 4)
    titlebox = wxBoxSizer(wxVERTICAL)
    titlebox.Add(title, 0, wxALIGN_LEFT, 4)
    titlebox.Add(general, 0, wxALIGN_CENTER, 4)
    topbox.AddSizer(titlebox, 0)
    infobox = wxBoxSizer(wxVERTICAL)
    infobox.AddSizer(topbox, 0)
    self.infowin.SetAutoLayout(1)
    self.infowin.SetSizer(infobox)
    self.topsizer.Layout()

class InfoboxScrolledPanel(wxWindow, wxPanel):
  def __init__(self, parent):
    wxPanel.__init__(self, parent, -1)
    self.SetBackgroundColour('White')
    self.SetAutoLayout(1)
    
