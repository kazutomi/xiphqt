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
import handlers

class SBTreeCtrl(wxTreeCtrl):
  def OnCompareItems(self, item1, item2):
    t1 = self.GetItemText(item1)
    t2 = self.GetItemText(item2)
    if t1 < t2: return -1
    if t1 == t2: return 0
    return 1


class SandboxPanel(wxPanel):
  def __init__(self, parent):
    wxPanel.__init__(self, parent, -1, style=wxWANTS_CHARS)
    EVT_SIZE(self, self.OnSize)

    #self.tree = wxTreeListCtrl(self, -1, style = wxTR_TWIST_BUTTONS)
    self.tree = SBTreeCtrl(self, -1, wxDefaultPosition, wxDefaultSize,
                           wxTR_HAS_BUTTONS | wxTR_EDIT_LABELS
                           | wxTR_MULTIPLE | wxTR_HIDE_ROOT
                           | wxTR_TWIST_BUTTONS)
    isz = (16,16)
    il = wxImageList(isz[0], isz[1])
    self.lclidx = il.Add(geticon('dev-lcl',0))
    self.p2pidx = il.Add(geticon('dev-p2p',0))
    self.webidx = il.Add(geticon('dev-web',0))
    self.fldridx     = il.Add(wxArtProvider_GetBitmap(wxART_FOLDER, wxART_OTHER, isz))
    self.fldropenidx = il.Add(wxArtProvider_GetBitmap(wxART_FILE_OPEN, wxART_OTHER, isz))
    self.fileidx     = il.Add(wxArtProvider_GetBitmap(wxART_REPORT_VIEW, wxART_OTHER, isz))
    self.muxpackidx  = il.Add(geticon('mux-packed',0))
    self.muxopenidx  = il.Add(geticon('mux-opened',0))

    self.codecidx = { 'vorbis'  : il.Add(geticon('vorbis',0)),
                      'theora'  : il.Add(geticon('theora',0)),
                      'speex'   : il.Add(geticon('speex',0)),
                      'flac'    : il.Add(geticon('flac',0)),
                      'nonfree' : il.Add(geticon('nonfree',0)),
                      ''        : il.Add(geticon('unknown',0))}

    self.tree.SetImageList(il)
    self.il = il

    self.root = self.tree.AddRoot("")
    self.tree.SetPyData(self.root, None)

    self.devlocal = self.tree.AppendItem(self.root, 'Local Media')
    self.tree.SetPyData(self.devlocal, None) 
    self.tree.SetItemImage(self.devlocal, self.lclidx, which = wxTreeItemIcon_Normal)

    self.share = self.tree.AppendItem(self.root, 'P2P Media')
    self.tree.SetPyData(self.share, None) 
    self.tree.SetItemImage(self.share, self.p2pidx, which = wxTreeItemIcon_Normal)

    self.web = self.tree.AppendItem(self.root, 'Web Media')
    self.tree.SetPyData(self.web, None) 
    self.tree.SetItemImage(self.web, self.webidx, which = wxTreeItemIcon_Normal)

    self.tree.Expand(self.root)
    EVT_RIGHT_DOWN(self.tree, self.OnRightClick)


  def OnSize(self, evt):
    self.tree.SetSize(self.GetSize())

  def OnRightClick(self, evt):
    pt = evt.GetPosition()
    item, flags = self.tree.HitTest(pt)
    if item == self.devlocal : 
      self.PopupDevLocalMenu(pt)

  def PopupDevLocalMenu(self, pos) :
    if not hasattr(self, "devlocalPopupNew"):
      self.devlocalPopupNewFile = wxNewId()
      EVT_MENU(self, self.devlocalPopupNewFile, self.OnNewLocalFile)
      #self.devlocalPopupNewDir = wxNewId()
      #EVT_MENU(self, self.devlocalPopupNewDir, self.OnNewLocalDir)
    menu = wxMenu()
    menu.Append(self.devlocalPopupNewFile, "Add New File")
    self.PopupMenu(menu, pos) 
    menu.Destroy()

  def OnNewLocalFile(self, evt):
    dlg = wxFileDialog(None, "Choose a file", os.getcwd(), "", 
                       "Ogg Media (.ogg)|*.ogg|All Files (*)|*",
                       wxOPEN | wxMULTIPLE | wxCHANGE_DIR )
    if dlg.ShowModal() == wxID_OK:
      paths = dlg.GetPaths()
      for path in paths : 
        self.AddNewFile(path)
    dlg.Destroy()


  def AddNewFile(self, path):
    fd = open(path,'r')
    if fd.read(4) != 'OggS' :
      print 'Non-Ogg file detected!'
      return
    fd.seek(0)
    newfile = self.tree.AppendItem(self.devlocal, os.path.split(path)[1])
    self.tree.SetPyData(newfile, None)
    self.tree.SetItemImage(newfile, self.fileidx, which = wxTreeItemIcon_Normal)
    chain = self.tree.AppendItem(newfile, 'Chain #0')
    self.tree.SetPyData(chain, None) 
    self.tree.SetItemImage(chain, self.muxpackidx, which = wxTreeItemIcon_Normal)
    self.tree.SetItemImage(chain, self.muxopenidx, which = wxTreeItemIcon_Expanded)

    sy = ogg2.OggSyncState()
    while sy:
      sy.input(fd)
      while 1:
        page = sy.pageout()
        if page :
          if page.pageno > 0 :
            sy = None
            break 
          st = ogg2.OggStreamState(page.serialno)
          st.pagein(page)
          packet = st.packetout()
          bp = ogg2.OggPackBuff(packet)
          header = ""
          while 1:
            byte = bp.read(8)
            if type(byte) == int : header = header + chr(byte)
            else : break
          for c in handlers.codecs :
            handler = c(header)
            if handler.name : break
          stream = self.tree.AppendItem(chain,  handler.name)
          self.tree.SetPyData(stream, handler) 
          if not self.codecidx.has_key(handler.icon) :
            print 'Missing icon for %s' % handler.name
            handler.icon = ''
          self.tree.SetItemImage(stream, self.codecidx[handler.icon],
                                 which = wxTreeItemIcon_Normal)
        else : break    
