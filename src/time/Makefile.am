## Process this file with automake to produce Makefile.in

SUBDIRS = e-map

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = time-admin

time_admin_SOURCES = \
	main.c \
	tz.c tz.h \
 	timeserv.c timeserv.h \
 	transfer.c transfer.h

time_admin_CFLAGS = $(GNOME_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(LIBGLADE_CFLAGS) "`gnome-config --cflags gnomecanvaspixbuf`"

time_admin_LDADD = \
$(GNOME_LIBDIR) \
$(GNOMEUI_LIBS) \
$(GNOME_XML_LIB) \
$(GDK_PIXBUF_LIBS) \
$(LIBGLADE_LIBS) \
$(INTLLIBS) \
../common/libsetuptool.a \
./e-map/e-map.o

time_admin_LDFLAGS = `gnome-config --libs gnomecanvaspixbuf`

EXTRA_DIST = time.desktop

sysdir = $(datadir)/control-center/System
sys_DATA = time.desktop

install-data-local:
	$(INSTALL_DATA) $(srcdir)/time.desktop $(DESTDIR)$(datadir)/gnome/apps/Settings/System/time.desktop
