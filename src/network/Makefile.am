##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

@XML_I18N_MERGE_DESKTOP_RULE@
###@__X_ST_TOOL_DESKTOP_IN_RULE@
include ../common/CommonMakefile

toolname = network
bin_PROGRAMS = network-admin

CPPFLAGS =
INCLUDES = $(XST_TOOL_CFLAGS)
CLEANFILES = $(XST_TOOL_CLEANFILES)
EXTRA_DIST = $(XST_TOOL_EXTRA_DIST)

network_admin_LDADD = $(XST_TOOL_LIBS) 
network_admin_DEPENDENCIES = $(XST_TOOL_DEPENDENCIES)
network_admin_SOURCES = \
	main.c \
	callbacks.c	callbacks.h	\
	transfer.c	transfer.h	\
	connection.c	connection.h	\
	hosts.c		hosts.h		\
	ppp-druid.c	ppp-druid.h

toolpixmaps =
etspecs = 
etspecs_DATA = $(etspecs)
desktop = $(toolname).desktop

all-local: $(desktop)
