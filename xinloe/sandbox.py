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
    self.parent = parent

    EVT_SIZE(self, self.OnSize)
    self.tid = wxNewId()
    self.tree = SBTreeCtrl(self, self.tid, wxDefaultPosition, 
                           wxDefaultSize, wxTR_HAS_BUTTONS 
                           | wxTR_HIDE_ROOT | wxTR_TWIST_BUTTONS)

    EVT_RIGHT_DOWN(self.tree, self.OnRightDown)
    EVT_TREE_SEL_CHANGED(self, self.tid, self.OnSelChanged)

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


  def OnSize(self, evt):
    self.tree.SetSize(self.GetSize())

  def OnRightDown(self, evt):
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
    dlg = wxFileDialog(self, "Choose a file", os.getcwd(), "", 
                       "Ogg Media (.ogg)|*.ogg|All Files (*)|*",
                       wxOPEN 
                       # | wxMULTIPLE we're not ready for this yet 
                       # | wxCHANGE_DIR this messes up our path!
                       )
    if dlg.ShowModal() == wxID_OK:
      paths = dlg.GetPaths()
      for path in paths : 
        newfile = LocalFile(self, path)
    dlg.Destroy()

  def OnSelChanged(self, evt):
    item = evt.GetItem()
    handler = self.tree.GetItemData(item).GetData()
    if handler :
      self.parent.infoboxWin.ShowCodec(handler)


class LocalFile:
  def __init__(self, parent, path):
    self.parent = parent
    self.path = path
    self.chains = []
    self.length = 0
    self.bytes = 0
    self.eof = False
    self.name = os.path.split(path)[1]
    self.desc = 'application/ogg'
    self.icon = 'oggfile'

    self.sy = ogg2.OggSyncState()
    self.fd = open(path,'r')
    if self.fd.read(4) != 'OggS' :
      return      
    self.fd.seek(0)
    self.branch = parent.tree.AppendItem(parent.devlocal, self.name)
    parent.tree.SetPyData(self.branch, self)
    parent.tree.SetItemImage(self.branch, parent.fileidx, which = wxTreeItemIcon_Normal)
    
    self.page = None
    while not self.eof:
      self.chains.append(self.Chain(self))
      self.bytes = self.bytes + self.chains[-1].bytes      
      self.length = self.length + self.chains[-1].length      

  class Chain:
    def __init__(self, parent):
      self.parent = parent
      grandparent = parent.parent
      self.icon = ''

      chain = grandparent.tree.AppendItem(parent.branch, \
       'Chain %d (%s offset)' % (len(parent.chains), timestr(parent.length)))
      grandparent.tree.SetPyData(chain, self) 
      grandparent.tree.SetItemImage(chain, grandparent.muxpackidx, 
                                    which = wxTreeItemIcon_Normal)
      grandparent.tree.SetItemImage(chain, grandparent.muxopenidx, 
                                    which = wxTreeItemIcon_Expanded)
      self.serials = {}

      bitstreams = self.GetNewStreams()
      for handler in bitstreams:
        self.serials[handler.serialno] = handler
        stream = grandparent.tree.AppendItem(chain, handler.name)
        grandparent.tree.SetPyData(stream, handler) 
        if not grandparent.codecidx.has_key(handler.icon) :
          print 'Missing icon for %s' % handler.name
          handler.icon = ''
        grandparent.tree.SetItemImage(stream, 
          grandparent.codecidx[handler.icon], which = wxTreeItemIcon_Normal)

      while parent.page and parent.page.pageno > 0:
        self.serials[parent.page.serialno].PageIn(parent.page)
        parent.page = None
        while not parent.page:
          if self.parent.sy.input(parent.fd) == 0 : 
            parent.eof  = True
            break  # End of file reached.
          parent.page = parent.sy.pageout()

      self.bytes = 0
      self.length = 0
      for handler in bitstreams:
        self.bytes = self.bytes + handler.bytes
        if handler.length > self.length :
          self.length = handler.length
  

    def GetNewStreams(self):
      parent = self.parent

      bitstreams = []
      while 1:
        while 1:
          while not parent.page:
            if parent.sy.input(parent.fd) == 0 : 
              parent.page = None
              parent.eof  = True
              return bitstreams  # End of file reached.
            parent.page = parent.sy.pageout()
          if parent.page.pageno > 0 :
            return bitstreams      

          serialno = parent.page.serialno
          pagesize = len(parent.page)
          st = ogg2.OggStreamState(serialno)
          st.pagein(parent.page)
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
          bitstreams.append(handler)
          handler.state = st
          handler.serialno = serialno
          handler.bytes = pagesize
          parent.page = None
