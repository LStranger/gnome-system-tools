##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = runlevel
bin_PROGRAMS = runlevel-admin

## SUBDIRS =
CPPFLAGS =
INCLUDES = $(XST_TOOL_CFLAGS)
CLEANFILES = $(XST_TOOL_CLEANFILES)
EXTRA_DIST = $(XST_TOOL_EXTRA_DIST)

runlevel_admin_LDADD = $(XST_TOOL_LIBS)
runlevel_admin_DEPENDENCIES = $(XST_TOOL_DEPENDENCIES)
runlevel_admin_SOURCES = \
	main.c                              \
	callbacks.c 		callbacks.h     \
	table.c			table.h	\
	transfer.c		transfer.h
toolpixmaps =
etspecs = 
