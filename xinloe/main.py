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
from sandbox import *
from infobox import *

class MainFrame(wxFrame):
  def __init__(self):
    wxFrame.__init__(self, None, -1, 'Xinloe', size=(620,400))
    self.CenterOnScreen()
    self.CreateStatusBar()
    self.SetStatusText("Ready.")
    self.ToolLabels = 0
    self.mainWin = MainWindow(self)
    self.winCount = 0

    menuBar = wxMenuBar()
    toolBar = self.CreateToolBar(wxTB_HORIZONTAL|wxNO_BORDER|wxTB_FLAT|wxTB_TEXT)

    items=(('&File',(('&New', 'Create a new project', self.OnNewWindow, 'filenew'),
                     ('&Open', 'Open a saved project', self.OnToolClick, 'fileopen'),
                     ('&Close', 'Close Project', self.CloseWindow))),
           ('&Edit',(('Cu&t', 'Move the selected text or item(s) to the clipboard', self.OnToolClick, 'editcut'),
                     ('&Copy', 'Copy the selected text or item(s) to the clipboard', self.OnToolClick, 'editcopy'),
                     ('&Paste', 'Paste the clipboard contents', self.OnToolClick, 'editpaste'))),
           ('&Stream',(('Sta&rt', 'Move active cursor to head of the project', 
                        self.OnToolClick, 'player_start'),
                       ('&Rewind', 'Move active cursor to start of the chain',
                        self.OnToolClick, 'player_rew'),
                       ('&Play', 'Stream from active cursor', 
                        self.OnToolClick, 'player_play'),
                       ('&Stop', 'Stop streaming from active cursor', 
                        self.OnToolClick, 'player_stop'),
                       ('&Forward', 'Move active cursor to the end of the chain',   
                        self.OnToolClick, 'player_fwd'),
                       ('&End', 'Move active cursor to tail of the project', 
                        self.OnToolClick, 'player_end'))))
                     

    i = 0
    for m in items :
      menu = wxMenu()
      for t in m[1] :
        if t : 
          menu.Append(i, t[0], t[1])
          EVT_MENU(self, i, t[2])
          if len(t) > 3 :
            if self.ToolLabels :
              toolBar.AddLabelTool(i, t[0].replace('&',''), geticon(t[3]), longHelp=t[1])
            else :
              toolBar.AddSimpleTool(i, geticon(t[3]), t[0].replace('&',''), t[1])
            EVT_TOOL(self, i, t[2])
            EVT_TOOL_RCLICKED(self, i, self.OnToolRClick)
        else  : 
          menu.AppendSeparator()
        i = i + 1
      menuBar.Append(menu, m[0])
      toolBar.AddSeparator()

    self.SetMenuBar(menuBar)

    EVT_MENU_HIGHLIGHT_ALL(self, self.OnMenuHighlight)

  def OnMenuHighlight(self, event):
  # Show how to get menu item imfo from this event handler
    id = event.GetMenuId()
    item = self.GetMenuBar().FindItemById(id)
    text = item.GetText()
    help = item.GetHelp()
    #print text, help
    event.Skip() # but in this case just call Skip so the default is done

  def CloseWindow(self, event):
    self.Close()

  def OnToolClick(self, event):
    tb = self.GetToolBar()
    #tb.EnableTool(10, not tb.GetToolEnabled(10))

  def OnToolRClick(self, event):
    return

  def OnNewWindow(self, evt):
    self.winCount = self.winCount + 1
    win = wxPanel(self.mainWin.projectWin, -1)
    self.mainWin.projectWin.AddPage(win, 'Untitled %d' % self.winCount)
    #win = wxMDIChildFrame(self.mainWin.projectWin, -1, "Untitled %d" % self.winCount)
    #canvas = MyCanvas(win)
    #win.Show(True)

  def OnExit(self, evt):
    self.Close(True)


class MainWindow(wxSplitterWindow):
  def __init__(self, parent):
    wxSplitterWindow.__init__(self, parent, -1)

    # Project Window
    self.projectWin = ProjectPanel(self)

    # Bottom Window
    self.bottomWin = BottomWindow(self)

    self.SetMinimumPaneSize(5)
    #self.SplitHorizontally(self.sandboxWin, self.projectWin, 100)
    #self.SplitVertically(self.projectWin, self.sandboxWin, 100)
    self.SplitHorizontally(self.projectWin, self.bottomWin, 250)


class ProjectPanel(wxNotebook):
  def __init__(self, parent):
    wxNotebook.__init__(self, parent, -1, style=wxNB_BOTTOM)

class BottomWindow(wxSplitterWindow):
  def __init__(self, parent):
    wxSplitterWindow.__init__(self, parent, -1)

    # Sandbox Window
    self.sandboxWin = SandboxPanel(self)

    # Infobox Window
    self.infoboxWin = InfoboxPanel(self)

    self.SetMinimumPaneSize(5)
    self.SplitVertically(self.sandboxWin, self.infoboxWin, 200)
 



import wx                  # This module uses the new wx namespace
#print "wx.VERSION_STRING = ", wx.VERSION_STRING

import sys, os

#----------------------------------------------------------------------------


class RunApp(wx.App):
  def __init__(self):
    wx.App.__init__(self, 0)

  def OnInit(self):
    wx.InitAllImageHandlers()
    wx.Log_SetActiveTarget(wx.LogStderr())

    win = MainFrame()
    win.Show(True)

    self.SetTopWindow(win)
    self.frame = win
    return True

  def OnCloseFrame(self, evt):
    if hasattr(self, "window") and hasattr(self.window, "ShutdownDemo"):
      self.window.ShutdownDemo()
    evt.Skip()


app = RunApp()
app.MainLoop()




