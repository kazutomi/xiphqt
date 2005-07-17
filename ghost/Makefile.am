## Process this file with automake to produce Makefile.in. -*-Makefile-*-

# To disable automatic dependency tracking if using other tools than
# gcc and gmake, add the option 'no-dependencies'
AUTOMAKE_OPTIONS = 1.8

#Fools KDevelop into including all files
SUBDIRS = libghost

rpm: dist
	rpmbuild -ta ${PACKAGE}-${VERSION}.tar.gz
