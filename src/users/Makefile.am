##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = users
bin_PROGRAMS = users-admin

SUBDIRS = search-bar
CPPFLAGS =
INCLUDES = $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

users_admin_LDADD = $(GST_TOOL_LIBS) ./search-bar/libsearchbar.a
users_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES) ./search-bar/libsearchbar.a
users_admin_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	user-settings.c user-settings.h \
	group-settings.c group-settings.h \
	passwd.c passwd.h \
	transfer.c transfer.h \
	user_group.c user_group.h \
	md5.c md5.h md5_crypt.c \
	users-table.c users-table.h \
	groups-table.c groups-table.h \
	table.c table.h  \
	user-group-xml.c user-group-xml.h \
	profiles-table.c profiles-table.h \
	profile-settings.c profile-settings.h \
	privileges-table.c privileges-table.h

toolpixmaps =
