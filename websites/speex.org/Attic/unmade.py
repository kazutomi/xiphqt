import os, shutil, re
PROLOG = re.compile(r"""^<\?.*\?>
""")
FILES = 'download index vmail projects todo news fsos samples packages docs standards contact comparison'.split()
def writeString(fn, s):
    fh = open(fn, 'w')
    fh.write(s)
    fh.close()
def readString(fn):
    fh = open(fn)
    s = fh.read()
    fh.close()
    return s
    
    
def stripProlog(fn):
    """Gets rid of the XML prolog"""
    shutil.copy(fn, fn + '.tmp')
    s = readString(fn + '.tmp')
    t = PROLOG.sub('', s)
    writeString(fn, t)
    os.remove(fn+'.tmp')
    

    
#for fn in 'index news'.split():
#    os.system('xsltproc -o %s.htfrag make%s.xsl news.xml' % (fn, fn))
for fn in FILES:
    cmd = 'xsltproc -o %s.html transform.xsl %s.htfrag' % (fn, fn)
    print cmd
    os.system(cmd)
for fn in [ s + '.html' for s in FILES]:
    stripProlog(fn)
