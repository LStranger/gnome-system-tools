##
## Process this file with automake to produce Makefile.in
##
## This file is based on ../shares/Makefile.am please keep the Makefile.am
## of all the tools in sync
##

@XML_I18N_MERGE_DESKTOP_RULE@
@XST_TOOL_DESKTOP_IN_RULE@

toolname = time
bin_PROGRAMS = time-admin

SUBDIRS = e-map
CPPFLAGS =

INCLUDES = $(XST_TOOL_CFLAGS)
CLEANFILES = $(XST_TOOL_CLEANFILES)
EXTRA_DIST = $(XST_TOOL_EXTRA_DIST)

time_admin_LDADD = $(XST_TOOL_LIBS) ./e-map/e-map.o
time_admin_SOURCES = \
	main.c            time-tool.h \
	xst-spin-button.c xst-spin-button.h \
	tz.c              tz.h \
	tz-map.c          tz-map.h \
	timeserv.c        timeserv.h \
	transfer.c        transfer.h 

toolpixmaps =
etspecs = 
etspecs_DATA = $(etspecs)
desktop = $(toolname).desktop

all-local: $(desktop)





