#from MiscUtils.DBPool import DBPool
import psycopg

# the number of items shown per box
# NOTE: feel free to tweak these
MUSICSITES = 3
LINKS = 3
SOFTWARE = 5
NEWS = 10

# set up the database connection pool
#datapool = DBPool(psycopg, 5, 'dbname=vorbis user=jack', serialize=0)
database = psycopg.connect('dbname=vorbis user=jack')
database.autocommit(1)
#datapool = DBPool(psycopg, 5, 'dbname=vorbis user=jack', autocommit=1)
