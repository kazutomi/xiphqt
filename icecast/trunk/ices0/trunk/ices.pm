# This is just a skeleton, something for you to start with.
# All these functions should exist in your module

# Function called to initialize your python environment.
# Should return 1 if ok, and 0 if something went wrong.

sub ices_perl_initialize {
	print "Perl subsystem Initializing:\n";
	return 1;
}

# Function called to shutdown your python enviroment.
# Return 1 if ok, 0 if something went wrong.
sub ices_perl_shutdown {
	print "Perl subsystem shutting down:\n";
}

# Function called to get the next filename to stream. 
# Should return a string.
sub ices_perl_get_next {
	print "Perl subsystem quering for new track:\n";
	return "/home/chad/music/A Perfect Circle - Reinholder.mp3";
}

# If defined, the return value is used for title streaming (metadata)
sub ices_perl_get_metadata {
        return "Artist - Title (Album, Year)";
}

# Function used to put the current line number of
# the playlist in the cue file. If you don't care
# about cue files, just return any integer.
sub ices_perl_get_current_lineno {
	return 1;
}
