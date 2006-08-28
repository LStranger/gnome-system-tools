##
## Process this file with automake to produce Makefile.in
##
## Please don't modify this file unless you know what you are doing
## Please keep the Makefile.am from all the tools in sync
##

include ../common/CommonMakefile

toolname = users
bin_PROGRAMS = users-admin

SUBDIRS = 
INCLUDES = $(GST_TOOL_CFLAGS)
CLEANFILES = $(GST_TOOL_CLEANFILES)
EXTRA_DIST = $(GST_TOOL_EXTRA_DIST)

users_admin_LDADD = $(GST_TOOL_LIBS) 
users_admin_DEPENDENCIES = $(GST_TOOL_DEPENDENCIES) 
users_admin_SOURCES = \
	main.c 			\
	passwd.c 		\
	users-tool.c 		\
	callbacks.c 		\
	user-settings.c 	\
	group-settings.c 	\
	users-table.c 		\
	groups-table.c 		\
	table.c table.h 	\
	privileges-table.c 	\
	group-members-table.c 	\
	user-profiles.c		\
	test-battery.c

toolpixmaps =
