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

class WorkPanel(wxNotebook):
  def __init__(self, parent):
    wxNotebook.__init__(self, parent, -1, style=wxNB_BOTTOM)
    self.winCount = 0

  def NewProject(self): 
    self.winCount = self.winCount + 1
    win = self.MyCanvas(self)
    self.AddPage(win, 'Untitled %d' % self.winCount)

  class MyCanvas(wxScrolledPanel):
    def __init__(self, win):
      wxScrolledPanel.__init__(self, win)
      self.SetBackgroundColour('White')
      self.workwin = WorkboxScrolledPanel(self)
      self.worksizer = wxBoxSizer(wxVERTICAL)
      self.worksizer.Add(self.workwin, 1, wxEXPAND, 0)
      self.SetSizer(self.worksizer)
      self.SetupScrolling()
      self.SetAutoLayout(1)


class WorkboxScrolledPanel(wxWindow, wxPanel):
  def __init__(self, parent):
    wxPanel.__init__(self, parent, -1)
    self.SetBackgroundColour('White')
    self.SetAutoLayout(1)
    
