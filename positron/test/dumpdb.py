import sys
import neuros
import db

def dumpdb(woid_db):
    for record in woid_db.get_records():
        if record == None:
            print "-> Null Record\n"
            continue

        print "-> Title: %s\nPlaylist: %s\tArtist: %s\tAlbum: %s" % tuple(record[:4])
        print "Genre: %s\tRecordings: %s" % tuple(record[4:6])
        print "Time: %d sec\tSize: %d kB" % tuple(record[6:8])
        print "Path: %s\n" % (record[8],)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print """dumpdb.py [neuros_root] [database name]
        Ex: dumpdb.py neuros_root audio"""
        sys.exit()

    n = neuros.Neuros(sys.argv[1])
    w = n.open_db(sys.argv[2])
    dumpdb(w)
    n.close_db(sys.argv[2])
    
