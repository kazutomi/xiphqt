## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = @subdirs@ src conf doc

EXTRA_DIST = README FEATURES BUGS AUTHORS TODO INSTALL COPYING CHANGES FAQ TESTED README.playlist README.old FILES

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
