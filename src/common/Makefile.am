noinst_LIBRARIES = libsetuptool.a

INCLUDES =		\
	$(GNOME_CFLAGS) 	\
	$(GDK_PIXBUF_CFLAGS) 	\
	$(LIBGLADE_CFLAGS) 	\
	$(GNOME_XML_CFLAGS)	\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

# for now, leave tool.[ch] in
libsetuptool_a_SOURCES = \
	xml.c		\
	xml.h		\
	helpers.c	\
	helpers.h	\
	text.c		\
	text.h		\
	global.h	\
	checked.xpm 	\
	unchecked.xpm 	\
	reading.xpm	\
	xst-dialog.c	\
	xst-dialog.h	\
	xst-tool.c	\
	xst-tool.h	\
	xst-types.h	\
	xst-report-hook.c \
	xst-report-hook.h

#	tool.c		\
#	tool.h

ccenterdir = $(datadir)/control-center
Systemdir = $(ccenterdir)/System

paneldir = $(datadir)/gnome/apps/Settings
Systemdirp = $(paneldir)/System

directory = System.directory
EXTRA_DIST = $(directory).in
CLEANFILES = $(directory)

$(directory): $(directory).in
	sed 's#Icon=.*#Icon='$(pixmapsdir)'/hst.png#' < $< > $@

all-local: $(directory)

install-data-local: $(directory)
	$(mkinstalldirs) $(DESTDIR)$(Systemdir)
	$(INSTALL_DATA) $(srcdir)/System.directory $(DESTDIR)$(Systemdir)/.directory
	$(mkinstalldirs) $(DESTDIR)$(Systemdirp)
	$(INSTALL_DATA) $(srcdir)/System.directory $(DESTDIR)$(Systemdirp)/.directory
