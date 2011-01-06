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
INCLUDES += $(GST_TOOL_CFLAGS) $(DBUS_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

time_admin_LDADD = ./e-map/libemap.a $(GST_TOOL_LIBS) $(DBUS_LIBS)
time_admin_DEPENDENCIES = ./e-map/libemap.a $(GST_TOOL_DEPENDENCIES)
time_admin_SOURCES = \
	time-tool.c	time-tool.h \
	tz.c		tz.h \
	tz-map.c	tz-map.h \
	ntp-servers-list.c	ntp-servers-list.h	\
	main.c

toolpixmaps =

-include $(top_srcdir)/git.mk
