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

class InfoboxPanel(wxWindow, wxScrolledPanel):
  def __init__(self, parent):
    wxScrolledPanel.__init__(self, parent, -1)
    self.SetBackgroundColour('White')
    infobox = wxBoxSizer(wxVERTICAL)
    title = gettext(self, ' Theora I - General Purpose Video ', 5)
    general = gettext(self, '00:01:59 205kbps (293293942 bytes)', 3)
    topbox = wxBoxSizer(wxHORIZONTAL)
    bmp = geticon('theora',3)
    logo = wxStaticBitmap(self, -1, bmp, wxPoint(16, 16),
                          wxSize(bmp.GetWidth(), bmp.GetHeight()))
    topbox.Add(logo, 0, wxALIGN_LEFT, 4)
    titlebox = wxBoxSizer(wxVERTICAL)
    titlebox.Add(title, 0, wxALIGN_LEFT, 4)
    titlebox.Add(general, 0, wxALIGN_CENTER, 4)
    topbox.AddSizer(titlebox, 0)
    self.SetSizer(infobox)
    self.SetAutoLayout(1)
    self.SetupScrolling(scroll_x=False)    
    infobox.AddSizer(topbox, 0)

