AUTOMAKE_OPTIONS = 1.9 foreign

SUBDIRS =	compat doc include lib src

dist_doc_DATA = COPYING NEWS README

pkgconfigdir =	$(libdir)/pkgconfig
pkgconfig_DATA = iceadmin.pc

CLEANFILES =	core *.core *~ .*~

.PHONY: 	snapshot

snapshot:
	${MAKE} distcheck distdir=${PACKAGE}-snapshot-`date +'%Y%m%d'`
