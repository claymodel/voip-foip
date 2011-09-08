AM_OPTIONS = gnu

DEFS += -include $(top_builddir)/include/confdefs.h

if FALSE
noinst_PROGRAMS = test

test_SOURCES = test.c
test_LDADD = $(top_builddir)/corelib/libcallweaver.la
endif FALSE

noinst_LTLIBRARIES = libtime.la 

libtime_la_SOURCES = localtime.c tzfile.h private.h
libtime_la_LDFLAGS = -no-undefined -avoid-version 

INCLUDES = -I$(top_srcdir)/include

#-static
