## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = time-admin

time_admin_SOURCES = \
	main.c \
	timezone.c timezone.h \
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
../common/libsetuptool.a

time_admin_LDFLAGS = `gnome-config --libs gnomecanvaspixbuf`
