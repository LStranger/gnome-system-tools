##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

@XML_I18N_MERGE_DESKTOP_RULE@
include ../common/CommonMakefile

toolname = boot
bin_PROGRAMS = boot-admin

## SUBDIRS =
CPPFLAGS =
INCLUDES = $(XST_TOOL_CFLAGS)
CLEANFILES = $(XST_TOOL_CLEANFILES)
EXTRA_DIST = $(XST_TOOL_EXTRA_DIST)

boot_admin_LDADD = $(XST_TOOL_LIBS)
boot_admin_DEPENDENCIES = $(XST_TOOL_DEPENDENCIES)
boot_admin_SOURCES = \
	main.c                              \
	callbacks.c 		callbacks.h     \
	transfer.c 		transfer.h      \
	e-table.c 		e-table.h       \
	boot-settings.c	boot-settings.h \
	boot-image.c		boot-image.h    \
	boot-image-editor.c boot-image-editor.h \
	boot-druid.c		boot-druid.h

toolpixmaps =
etspecs = boot.etspec
