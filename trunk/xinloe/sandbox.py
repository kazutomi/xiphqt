import os
import ogg2
from wxPython.wx import *
from wxPython.gizmos import wxTreeListCtrl

import images

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
    self.lclidx = il.Add(images.geticon('dev-lcl',0))
    self.p2pidx = il.Add(images.geticon('dev-p2p',0))
    self.webidx = il.Add(images.geticon('dev-web',0))
    self.fldridx     = il.Add(wxArtProvider_GetBitmap(wxART_FOLDER, wxART_OTHER, isz))
    self.fldropenidx = il.Add(wxArtProvider_GetBitmap(wxART_FILE_OPEN, wxART_OTHER, isz))
    self.fileidx     = il.Add(wxArtProvider_GetBitmap(wxART_REPORT_VIEW, wxART_OTHER, isz))
    self.muxpackidx  = il.Add(images.geticon('mux-packed',0))
    self.muxopenidx  = il.Add(images.geticon('mux-opened',0))
    self.vorbisidx   = il.Add(images.geticon('vorbis',0))
    self.theoraidx   = il.Add(images.geticon('theora',0))
    self.speexidx    = il.Add(images.geticon('speex',0))
    self.flacidx     = il.Add(images.geticon('flac',0))
    self.unknownidx  = il.Add(images.geticon('unknown',0))

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
          if header[:7] == '\x01vorbis':
            if header[7:11] == '\x00\x00\x00\x00' : 
              ch = ord(header[11])
              sa = ord(header[12])+(ord(header[13])*256)
              sa = sa+(ord(header[14])*65536)+(ord(header[15])*16777216)
              stream = self.tree.AppendItem(chain,  'Vorbis I (%d channels, %d samples/sec)' % (ch, sa))
              self.tree.SetPyData(stream, ('vorbis', 0, ch, sa)) 
            else : 
              stream = self.tree.AppendItem(chain,  'Vorbis (Unsupported Version)')
              self.tree.SetPyData(stream, ('vorbis',)) 
            self.tree.SetItemImage(stream, self.vorbisidx, which = wxTreeItemIcon_Normal)
          elif header[:7] == '\x80theora':
            stream = self.tree.AppendItem(chain,  'Theora')
            self.tree.SetPyData(stream, None) 
            self.tree.SetItemImage(stream, self.theoraidx, which = wxTreeItemIcon_Normal)
          elif header[:5] == 'Speex':
            stream = self.tree.AppendItem(chain,  'Speex')
            self.tree.SetPyData(stream, None) 
            self.tree.SetItemImage(stream, self.speexidx, which = wxTreeItemIcon_Normal)
          elif header[:4] == 'fLaC':
            stream = self.tree.AppendItem(chain,  'FLAC')
            self.tree.SetPyData(stream, None) 
            self.tree.SetItemImage(stream, self.flacidx, which = wxTreeItemIcon_Normal)
          elif header[:5] == '\x00writ':
            stream = self.tree.AppendItem(chain,  'Writ')
            self.tree.SetPyData(stream, None) 
            self.tree.SetItemImage(stream, self.unknownidx, which = wxTreeItemIcon_Normal)
          else :
            stream = self.tree.AppendItem(chain,  'Unknown Codec')
            self.tree.SetPyData(stream, None) 
            self.tree.SetItemImage(stream, self.unknownidx, which = wxTreeItemIcon_Normal)
        else : break    
