## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST= iconv_string.h log.h yp_db.h yp_routines.h run_cachegen.sh

bin_PROGRAMS = yp_cgi yp_cachegen yp_clean yp_random


yp_cgi_SOURCES = yp_cgi.c log.c yp_routines.c
yp_cachegen_SOURCES = yp_cachegen.c log.c yp_routines.c
yp_clean_SOURCES = yp_clean.c log.c yp_routines.c
yp_random_SOURCES = yp_random.c log.c yp_routines.c
    
CFLAGS = @CFLAGS@ @MYSQL_CFLAGS@ @XML_CPPFLAGS@ -I${prefix}/include
CPPFLAGS = @CPPFLAGS@ @MYSQL_CFLAGS@ @XML_CPPFLAGS@ -I${prefix}/include
LIBS = @LIBS@ @MYSQL_LIBS@ @XML_LIBS@ -L${prefix}/lib


debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

