## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST= log.h yp_db.h yp_routines.h run_cachegen.sh

bin_PROGRAMS = yp_cgi yp_cachegen yp_clean yp_random yp_check


yp_cgi_SOURCES = yp_cgi.c log.c yp_routines.c
yp_cachegen_SOURCES = yp_cachegen.c log.c yp_routines.c
yp_clean_SOURCES = yp_clean.c log.c yp_routines.c
yp_random_SOURCES = yp_random.c log.c yp_routines.c
yp_check_SOURCES = yp_check.c log.c check_socket.c yp_routines.c
    
CFLAGS = @CFLAGS@ @MYSQL_CFLAGS@ -I${prefix}/include -I/usr/include/libcgi `xml2-config --cflags`
LIBS = @LIBS@ @MYSQL_LIBS@ -L${prefix}/lib `xml2-config --libs`


static:
	$(MAKE) all LDFLAGS="${LDFLAGS} -all-static"

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

