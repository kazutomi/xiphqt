#!/usr/bin/env python

#	$Id: oggtail.py,v 1.1 2001/05/02 01:49:11 andrew Exp $

# Send to stdout an ogg stream composed of the last PERCENT percent of
# file OGGFILE.
#
# example:  oggtail.py --percent=PERCENT OGGFILE
#
# (PERCENT is rounded down to the nearest page.  Cutting at the
# nearest packet would be more complicated.)

# First version by Mike Coleman <mkc@mathdogs.com>, April 2001

# TODO: This still doesn't work quite right.  'ogg123' complains with
# a warning if we pipe oggtail's output directly into it, so probably
# there's a minor error in the stream formatting somewhere.


import getopt
import ogg
import sys

_debug = 0

def usage():
    sys.exit("usage: oggtail.py [--debug] --percent=PERCENT OGGFILE")

def debug(s):
    if _debug:
        sys.stderr.write(s)
        sys.stderr.write('\n')


def copy_packets(from_, to_, outstream=None, count=None, start_pageno=0, start_granulepos=0):
    """
    Copy 'count' packets from file 'from_' to file 'to_'.  If 'count'
    is None, copy the rest of 'from_'.  Use 'start_granulepos' as the
    initial granulepos tag for pages.  Bytes will be skipped in
    'from_' to sync, as needed, and copying will stop if a chain
    boundary is encountered.
    """

    # XXX: warn on desync, except at beginning?

    copied = 0
    written = 0
    pageno = start_pageno
    granule_offset = None

    instream = None

    # get it?  insync?  get it?  i'm so funny you can't stand it!
    insync = ogg.OggSyncState()

    while count == None or copied < count:
        b = from_.read(65536)           # size doesn't really matter
        if not b:
            break
        insync.bytesin(b)

        skipped = 1
        while skipped != 0:
            skipped, page = insync.pageseek()
            if skipped > 0:
                if instream and page.serialno() != serialno:
                    # we hit a chain boundary
                    break
                if not instream:
                    serialno = page.serialno()
                    instream = ogg.OggStreamState(serialno)
                    outstream = ogg.OggStreamState(serialno)
                page.pageno = pageno
                debug('*** %s' % page.pageno())
                pageno = pageno + 1
                instream.pagein(page)
                while count == None or copied < count:
                    p = instream.packetout()
                    if not p:
                        break
                    if p.granulepos != -1:
                        if granule_offset == None:
                            granule_offset = p.granulepos
                        p.granulepos = p.granulepos - granule_offset
                    debug('copied %s' % p)
                    outstream.packetin(p)
                    copied = copied + 1
                while 1:
                    pg = outstream.pageout()
                    if not pg:
                        break
                    debug('writing %s' % pg)
                    # FIX: should check success of write
                    written = written + pg.writeout(to_)
            elif skipped < 0:
                print 'skipped', -skipped, 'bytes'

    pg = outstream.flush()
    if pg:
        written = written + pg.writeout(to_)

    return (written, pageno - start_pageno, outstream)
    



opts, pargs = getopt.getopt(sys.argv[1:], '', ['debug', 'percent='])

print sys.argv, opts, pargs
if len(opts) < 1 or len(pargs) != 1:
    usage()

for o, a in opts:
    print o, a
    if o == '--percent':
        try:
            percent = float(a)
        except ValueError:
            usage()
    elif o == '--debug':
        _debug = 1
    else:
        usage()

if not 0.0 <= percent <= 100.0:
    usage()

file = pargs[0]

f = open(file, 'rb')

f.seek(0, 2)
f_size = f.tell()
f.seek(0)

# first copy the three header packets
n, p, os = copy_packets(f, sys.stdout, None, 3)
debug('copied header: %d bytes, %d pages' % (n, p))
debug(str(os))

# then seek to percent and copy out the rest
f.seek(int((100.0 - percent) / 100.0 * f_size))
n, p, os = copy_packets(f, sys.stdout, os)
debug('copied tail: %d bytes, %d pages' % (n, p))
debug(str(os))
