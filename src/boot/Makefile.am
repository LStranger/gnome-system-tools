##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = boot
bin_PROGRAMS = boot-admin

## SUBDIRS =
CPPFLAGS =
INCLUDES = $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

boot_admin_LDADD = $(GST_TOOL_LIBS)
boot_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES)
boot_admin_SOURCES = \
	main.c                              \
	callbacks.c 		callbacks.h     \
	transfer.c 		transfer.h      \
	table.c 		table.h       \
	boot-settings.c	boot-settings.h \
	boot-image.c		boot-image.h    \
	boot-druid.c		boot-druid.h    \
	boot-append-gui.c       boot-append-gui.h

toolpixmaps =
