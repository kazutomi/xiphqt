#!/usr/bin/env python

# Import modules
import os
import mutagen
from mutagen.oggvorbis import OggVorbis
from sqlobject import *

# Import variables from config file.
execfile('config.py')

class data(SQLObject):
        _connection = conn
        album1 = StringCol()
        artist1 = StringCol()
        title1 = StringCol()
        path1 = StringCol()
        genre1 = StringCol()
try:
    data.dropTable()
except:
    pass

data.createTable()

# Walk filesystem,
for path, dirs, files in os.walk(file_dir):
    
# Check for files
    if len(files) > 0:
# find number of files
        w = len(files)
# Set counter to zero.
        n = 0
# while counter is less than file length. (possibly try less than or equal to?)
        while n < w:
# join path and files
            g = os.path.join(path, files[n])
# split extension from path
            i = os.path.splitext(g)
# increment counter
            n += 1
# check if extension is .ogg (possibly check for mime type later?)
            if i[1] != ".ogg":
                exit
# read metadata from tag, assign to variables, check errors
            try:
                h = mutagen.oggvorbis.OggVorbis(g)
            except mutagen.oggvorbis.OggVorbisHeaderError:
                album = ''
                artist = ''
                title = ''
                genre = ''
            try:
                album = h['album']
            except KeyError:
                album = ''
            try:
                artist = h['artist']
            except KeyError:
                artist = ''
            try:
                title = h['title']
            except KeyError:
                title = ''
            try:
                genre = h['genre']
            except KeyError:
                genre = ''
# Add row to table.
            try:
                data(path1 = g, artist1 = artist, album1 = album, title1 = title, genre1 = genre)
            except UnicodeDecodeError:
                exit
            except UnicodeEncodeError:
                exit
# Print metadata, path
            try:
                print artist, album, title, genre, g
            except UnicodeEncodeError:
                artist = artist.encode('utf-8')
                album = album.encode('utf-8')
                title = title.encode('utf-8')
                genre = genre.encode('utf-8')
                print artist, album, title, genre, g
