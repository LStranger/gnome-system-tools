##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = disks
bin_PROGRAMS = disks-admin

## SUBDIRS =
INCLUDES += -Wall $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

disks_admin_LDADD = $(GST_TOOL_LIBS)
disks_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES)
disks_admin_SOURCES = \
	main.c                             \
	transfer.c		  transfer.h	\
	gst-disks-tool.c	  gst-disks-tool.h \
	disks-storage.c 	  disks-storage.h \
	disks-storage-disk.c 	  disks-storage-disk.h \
	disks-mountable.c	  disks-mountable.h	\
	disks-partition.c 	  disks-partition.h \
	disks-storage-cdrom.c     disks-storage-cdrom.h \
	disks-cdrom-disc.c	  disks-cdrom-disc.h	\
	disks-cdrom-disc-data.c	  disks-cdrom-disc-data.h \
	disks-cdrom-disc-audio.c  disks-cdrom-disc-audio.h \
	disks-cdrom-disc-mixed.c  disks-cdrom-disc-mixed.h \
	disks-cdrom-disc-dvd.c  disks-cdrom-disc-dvd.h \
	disks-storage-floppy.c    disks-storage-floppy.h \
	disks-factory-storage.c   disks-factory-storage.h \
	disks-gui.c		  disks-gui.h \
	callbacks.c		  callbacks.h

toolpixmaps =

-include $(top_srcdir)/git.mk
