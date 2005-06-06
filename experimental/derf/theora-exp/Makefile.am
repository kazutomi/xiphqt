
AUTOMAKE_OPTIONS = foreign 1.6 dist-zip dist-bzip2

EXTRA_DIST = doc

SUBDIRS = lib include examples

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = theoraenc.pc theoradec.pc

dist-hook:
	rm -rf `find $(distdir)/doc -name .svn`
