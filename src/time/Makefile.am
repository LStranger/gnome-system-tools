##
## Process this file with automake to produce Makefile.in
##
## This file is based on ../shares/Makefile.am please keep the Makefile.am
## of all the tools in sync
##

include ../common/CommonMakefile

toolname = time
bin_PROGRAMS = time-admin

SUBDIRS = e-map
CPPFLAGS =
INCLUDES = $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

time_admin_LDADD = $(GST_TOOL_LIBS) ./e-map/libemap.a
time_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES) ./e-map/libemap.a
time_admin_SOURCES = \
	main.c            time-tool.h \
	tz.c              tz.h \
	tz-map.c          tz-map.h \
	timeserv.c        timeserv.h \
	transfer.c        transfer.h 

toolpixmaps =
etspecs = 
etspecs_DATA = $(etspecs)
