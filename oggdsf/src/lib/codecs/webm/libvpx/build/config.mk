# This file automatically generated by configure. Do not edit!
TOOLCHAIN := x86-win32-vs8
ALL_TARGETS += libs
ALL_TARGETS += examples
ALL_TARGETS += solution
ifeq ($(MAKECMDGOALS),dist)
DIST_DIR?=vpx-vp8-debug-src-x86-win32mt-vs8-v0.9.2
else
DIST_DIR?=$(DESTDIR)/usr/local
endif
LIBSUBDIR=lib

VERSION_MAJOR=0
VERSION_MINOR=9
VERSION_PATCH=2

CONFIGURE_ARGS=--target=x86-win32-vs8 --enable-debug-libs --enable-install-srcs --enable-codec-srcs --enable-static-msvcrt
CONFIGURE_ARGS?=--target=x86-win32-vs8 --enable-debug-libs --enable-install-srcs --enable-codec-srcs --enable-static-msvcrt