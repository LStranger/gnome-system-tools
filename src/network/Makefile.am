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
network_admin_SOURCES = 			\
	address-list.c	address-list.h		\
	gst-network-tool.c gst-network-tool.h	\
	transfer.c transfer.h			\
	network-iface.c network-iface.h		\
	network-iface-ethernet.c network-iface-ethernet.h	\
	network-iface-wireless.c network-iface-wireless.h	\
	network-iface-irlan.c network-iface-irlan.h		\
	network-iface-plip.c network-iface-plip.h		\
	network-iface-modem.c network-iface-modem.h		\
	ifaces-list.c ifaces-list.h		\
	callbacks.c callbacks.h			\
	connection.c connection.h		\
	hosts.c hosts.h				\
	location.c location.h			\
	main.c 

toolpixmaps =
