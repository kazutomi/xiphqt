#!/usr/bin/env python

import license
print license.__doc__

import sys, os
prefix = sys.exec_prefix
config = "%s/lib/python%s/config" % (prefix, version)
os.system("cp %s/Makefile.pre.in ." % config)
os.system("make -f Makefile.pre.in boot")
os.system("make")
try: import ao 
except ImportError, m:
    print "Couldn't import ao: %s" % str(m)
    print "Probably aomodule.so didn't compile correctly."
    sys.exit(1)
