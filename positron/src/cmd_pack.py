"""positron pack:\tRemoves unused space from databases

  positron pack

     Packs all databases.

  positron pack <database1> <database2> ...

     Packs all entries from a particular database.  Valid database names
     are: audio, pcaudio, unidedhisi, idedhisi, failedhisi
"""

from neuros import Neuros
import neuros as neuros_module
import util

    
def run(config, neuros, args):
    if len(args) == 0:
        args = ["audio", "unidedhisi", "idedhisi", "failedhisi"]

    for arg in args:
        try:
            database = neuros.open_db(arg)
            print "  Packing database \"%s\"..." % (arg,)
            database.pack()
            neuros.close_db(arg)
        except neuros_module.Error, e:
            print "Error:", e

    print
    print "Done!"
