##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = network
bin_PROGRAMS = network-admin

## SUBDIRS =
CPPFLAGS =
INCLUDES = $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

network_admin_LDADD = $(GST_TOOL_LIBS) 
network_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES)
network_admin_SOURCES = \
	main.c \
	callbacks.c	callbacks.h	\
	transfer.c	transfer.h	\
	connection.c	connection.h	\
	hosts.c		hosts.h		\
	ppp-druid.c	ppp-druid.h

toolpixmaps =
etspecs = 
