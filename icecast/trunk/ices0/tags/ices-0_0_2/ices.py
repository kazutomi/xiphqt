from string import *
import sys

# This is just a skeleton, something for you to start with.
# All these functions should exist in your module

ices_module_version = "0.0.1"

songnumber = -1

# Function called from the python api just to verify that your file
# is ok.  
def testfunction ():
	print 'Should module version', ices_module_version, 'initializing...'

# Function called to get the next filename to stream. 
# Should return a string.
def ices_python_get_next ():
	print 'Executing get_next() function...'
	return 'Very nice song.mp3'

# Function called to initialize your python environment.
# Should return 1 if ok, and 0 if something went wrong.
def ices_python_initialize ():
	print 'Executing initialize() function..'
	return 1

# Function called to shutdown your python enviroment.
# Return 1 if ok, 0 if something went wrong.
def ices_python_shutdown ():
	print 'Executing shutdown() function...'
	return 1

# Function used to put the current line number of
# the playlist in the cue file. If you don't care
# about cue files, just return any integer.
# It should, however, return 0 the very first time
# and then never 0 again. This is because the metadata
# updating function should be delayed a little for
# the very first song, because the icecast server may
# not have accepted the stream yet.
def ices_python_get_current_lineno ():
	global songnumber
	print 'Executing get_current_lineno() function...'
	songnumber = songnumber + 1
	return songnumber
