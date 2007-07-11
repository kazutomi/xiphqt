#!/usr/bin/env python

import sys, os
from sqlobject import *
import datetime
from datetime import datetime
from time import strptime

# Import variables from config file.
execfile('config.py')

# NOT FINISHED, NEED TO RE-DO
class playlists(SQLObject):
    _connection = conn
    name = StringCol()
    description = StringCol()
    position = IntCol()
    monday = lfl
    tuesday = lfl
    wednesday = lfl
    thursday = lfl
    friday = lfl
    saturday = lfl
    sunday = lfl
    current = BoolCol()

class Log:
    """file like for writes with auto flush after each write
    to ensure that everything is logged, even during an
    unexpected exit."""
    def __init__(self, f):
        self.f = f
    def write(self, s):
        self.f.write(s)
        self.f.flush()

def main():
    #change to data directory if needed
    os.chdir("/mnt/icebreaker/")
    #redirect outputs to a logfile
    sys.stdout = sys.stderr = Log(open(LOGFILE, 'a+'))
    #ensure the that the daemon runs a normal user
#    os.setegid(100)     #set group
#    os.seteuid(1000)     #set user
    #change all 1's to 0 in current
    while BLAH
        #check date/time
        n=datetime.now()
        #DAY OF WEEK
        d=datetime.weekday(n)
        #check playlists table for list

        # make days in columns
        # time in tuples per playlist
    
        n=datetime.now().strftime("%H:%M:%S")
        s = #time in schedule
        # Placeholder, not real formatting
        f=datetime.time(datetime(*strptime(s, "%H:%M")[0:6]))

        # use timedelta
        #if no list
        #    shuffle from files
        #change 0 for current list to 1


if __name__ == "__main__":
    # do the UNIX double-fork magic, see Stevens' "Advanced
    # Programming in the UNIX Environment" for details (ISBN 0201563177)
    try:
        pid = os.fork()
        if pid > 0:
            # exit first parent
            sys.exit(0)
    except OSError, e:
        print >>sys.stderr, "fork #1 failed: %d (%s)" % (e.errno, e.strerror)
        sys.exit(1)

    # decouple from parent environment
    os.chdir("/")   #don't prevent unmounting....
    os.setsid()
    os.umask(0)

    # do second fork
    try:
        pid = os.fork()
        if pid > 0:
            # exit from second parent, print eventual PID before
            #print "Daemon PID %d" % pid
            open(PIDFILE,'w').write("%d"%pid)
            sys.exit(0)
    except OSError, e:
        print >>sys.stderr, "fork #2 failed: %d (%s)" % (e.errno, e.strerror)
        sys.exit(1)

    # start the daemon main loop
    main()
