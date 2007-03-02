
AUTOMAKE_OPTIONS = foreign 1.6 dist-zip dist-bzip2

EXTRA_DIST = doc

SUBDIRS = lib include examples

if TH_ENCODER
TH_ENCODER_FILES=theoraenc.pc 
else
TH_ENCODER_FILES=
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(TH_ENCODER_FILES) theoradec.pc theoracompat.pc

dist-hook:
	rm -rf `find $(distdir)/doc -name .svn`
