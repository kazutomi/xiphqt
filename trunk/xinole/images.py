from wxPython.wx import *

def geticon(name, size=1):
  sizes=('16x16','22x22','32x32')
  f = 'icons/%s/%s.png' % (sizes[size], name)
  return wxImage(f, wxBITMAP_TYPE_PNG).ConvertToBitmap()
