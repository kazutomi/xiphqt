
sub ices_perl_initialize {
	print "Perl subsystem Initializing:\n";
}

sub ices_perl_shutdown {
	print "Perl subsystem shutting down:\n";
}

sub ices_perl_get_next {
	print "Perl subsystem quering for new track:\n";
	return "/home/chad/music/A Perfect Circle - Reinholder.mp3";
}

sub ices_perl_get_current_lineno {
	return 1;
}
