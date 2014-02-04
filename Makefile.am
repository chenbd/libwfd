#
# libwfd - Global Makefile
# Copyright (c) 2013-2014 David Herrmann <dh.herrmann@gmail.com>
#

#
# Library Version Numbers
#

LIBWFD_CURRENT = 0
LIBWFD_REVISION = 0
LIBWFD_AGE = 0

#
# Global Configurations and Initializations
#

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory
AUTOMAKE_OPTIONS = color-tests

SUBDIRS = .

.DELETE_ON_ERROR:

include_HEADERS =
EXTRA_DIST = \
	README \
	COPYING \
	NEWS \
	docs/libwfd.pc.in \
	docs/libwfd.sym
CLEANFILES =
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =
TPHONY =

TESTS =
MEMTESTS =
check_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =

#
# Default CFlags
# Make all files include "config.h" by default. This shouldn't cause any
# problems and we cannot forget to include it anymore.
# Also make the linker discard all unused symbols.
#

AM_CFLAGS = \
	-Wall \
	-pipe \
	-fno-common \
	-ffast-math \
	-fdiagnostics-show-option \
	-fno-strict-aliasing \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections
AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I $(srcdir)/src
AM_LDFLAGS = \
	-Wl,--as-needed \
	-Wl,--gc-sections \
	-Wl,-z,relro \
	-Wl,-z,now

#
# SHL - Static Helper Library
# The SHL subsystem contains several small code pieces used all over libwfd and
# other applications.
#

noinst_LTLIBRARIES += libshl.la

libshl_la_SOURCES = \
	src/shl_llog.h \
	src/shl_macro.h \
	src/shl_ring.h \
	src/shl_ring.c \
	src/shl_util.h \
	src/shl_util.c
libshl_la_CPPFLAGS = $(AM_CPPFLAGS)
libshl_la_LDFLAGS = $(AM_LDFLAGS)
libshl_la_LIBADD = $(AM_LIBADD)

#
# libwfd
# Main library build instructions
#

lib_LTLIBRARIES += libwfd.la
include_HEADERS += src/libwfd.h
pkgconfig_DATA += docs/libwfd.pc

libwfd_la_SOURCES = \
	src/libwfd.h \
	src/rtsp_decoder.c \
	src/rtsp_tokenizer.c
libwfd_la_CPPFLAGS = $(AM_CPPFLAGS)
libwfd_la_LIBADD = libshl.la
EXTRA_libwfd_la_DEPENDENCIES = $(top_srcdir)/docs/libwfd.sym
libwfd_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-version-info $(LIBWFD_CURRENT):$(LIBWFD_REVISION):$(LIBWFD_AGE) \
	-Wl,--version-script="$(top_srcdir)/docs/libwfd.sym"

#
# Tests
#

tests = \
	test_rtsp

if BUILD_HAVE_CHECK
check_PROGRAMS += $(tests)
TESTS += $(tests)
endif

test_sources = \
	test/test_common.h
test_libs = \
	libwfd.la \
	libshl.la \
	$(CHECK_LIBS)
test_cflags = \
	$(AM_CPPFLAGS) \
	$(CHECK_CFLAGS)
test_lflags = \
	$(AM_LDFLAGS)

test_rtsp_SOURCES = test/test_rtsp.c $(test_sources)
test_rtsp_CPPFLAGS = $(test_cflags)
test_rtsp_LDADD = $(test_libs)
test_rtsp_LDFLAGS = $(test_lflags)

#
# Phony targets
#

.PHONY: $(TPHONY)

#
# Empty .SECONDARY target causes alle intermediate files to be treated as
# secondary files. That is, they don't get deleted after make finished.
#

.SECONDARY:
