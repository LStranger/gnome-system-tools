INCLUDES = $(GST_CFLAGS) 		\
	-DDATADIR=\"$(datadir)\"		\
	-I$(top_srcdir)			\
	-I$(top_builddir)

bonobodir = $(libdir)/bonobo
bonobo_LTLIBRARIES = libgst-shares-component.la

libgst_shares_component_la_SOURCES =		\
	gst-shares-component.c				\
	gst-shares-component.h				\
	libmain.c

libgst_shares_component_la_LIBADD = $(GST_LIBS)
libgst_shares_component_la_LDFLAGS = -module -avoid-version

server_in_files = GST_Shares_Component.server.in.in

serverdir = $(libdir)/bonobo/servers
server_DATA = $(server_in_files:.server.in.in=.server)
$(server_in_files:.server.in.in=.server.in): $(server_in_files)
	sed -e "s|\@BONOBODIR\@|$(bonobodir)|" $< > $@

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = $(server_in_files)
CLEANFILES = $(server_DATA) $(server_DATA).in
