noinst_LIBRARIES = libsetuptool.a

INCLUDES =		\
	$(GNOME_CFLAGS) 	\
	$(GDK_PIXBUF_CFLAGS) 	\
	$(LIBGLADE_CFLAGS) 	\
	$(GNOME_XML_CFLAGS)		\
	"`gnome-config --cflags gnomecanvaspixbuf`"


libsetuptool_a_SOURCES = \
	xml.c xml.h \
	helpers.c helpers.h \
	text.c text.h \
	tool.c tool.h \
	global.h \
	checked.xpm unchecked.xpm reading.xpm \
	gnomesupport.h

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
