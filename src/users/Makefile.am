##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = users
bin_PROGRAMS = users-admin

SUBDIRS = e-search-bar
CPPFLAGS =
INCLUDES = $(XST_TOOL_CFLAGS) $(XST_CRACK_LIB_CFLAGS)
CLEANFILES = $(XST_TOOL_CLEANFILES)
EXTRA_DIST = $(XST_TOOL_EXTRA_DIST)

users_admin_LDADD = $(XST_TOOL_LIBS) ./e-search-bar/libesearchbar.a
users_admin_DEPENDENCIES = $(XST_TOOL_DEPENDENCIES) ./e-search-bar/libesearchbar.a
users_admin_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	passwd.c passwd.h \
	transfer.c transfer.h \
	user_group.c user_group.h \
	md5.c md5.h md5_crypt.c \
	e-table.c e-table.h  \
	user_settings.c user_settings.h \
	profile.c profile.h \
	user-group-xml.c user-group-xml.h \
	user-druid.c user-druid.h \
	user-account-editor.c user-account-editor.h

toolpixmaps =
etspecs = group.etspec  net_group.etspec  net_user.etspec  profiles.etspec  users.etspec
