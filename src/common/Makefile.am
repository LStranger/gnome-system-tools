noinst_LIBRARIES = libsetuptool.a

libsetuptool_a_SOURCES = \
	xml.c xml.h \
	helpers.c helpers.h \
	text.c text.h \
	tool.c tool.h \
	global.h \
	checked.xpm unchecked.xpm reading.xpm \
	gnomesupport.h

libsetuptool_a_CFLAGS = \
	$(GNOME_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(LIBGLADE_CFLAGS) \
	"`gnome-config --cflags gnomecanvaspixbuf`"

ccenterdir = $(datadir)/control-center
Systemdir = $(ccenterdir)/System

paneldir = $(datadir)/gnome/apps/Settings
Systemdirp = $(paneldir)/System

Directory_files = System.directory
EXTRA_DIST = System.directory

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(Systemdir)
	$(INSTALL_DATA) $(srcdir)/System.directory $(DESTDIR)$(Systemdir)/.directory
	$(mkinstalldirs) $(DESTDIR)$(Systemdirp)
	$(INSTALL_DATA) $(srcdir)/System.directory $(DESTDIR)$(Systemdirp)/.directory
