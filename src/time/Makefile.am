## Process this file with automake to produce Makefile.in

SUBDIRS = e-map

INCLUDES = \
	-I$(top_srcdir)/src/common 	\
	-I$(top_srcdir)/intl 		\
	$(GNOME_INCLUDEDIR)		\
	$(GNOME_CFLAGS) 		\
	$(GDK_PIXBUF_CFLAGS) 		\
	$(LIBGLADE_CFLAGS)		\
	$(GNOME_XML_CFLAGS)

sbin_PROGRAMS = time-admin

time_admin_SOURCES = \
	main.c \
	tz.c tz.h \
	tz-map.c tz-map.h \
	timeserv.c timeserv.h \
	transfer.c transfer.h

time_admin_LDADD = \
$(GNOME_LIBDIR) \
$(GNOMEUI_LIBS) \
$(GNOME_XML_LIB) \
$(GDK_PIXBUF_LIBS) \
$(LIBGLADE_LIBS) \
$(INTLLIBS) \
../common/libsetuptool.a \
./e-map/e-map.o

desktop = time.desktop

EXTRA_DIST = $(desktop)

CLEANFILES = $(desktop).menu $(desktop).cc

$(desktop).menu: $(desktop)
	sed 's#Exec=.*#Exec=time-admin#' < $< > $@

$(desktop).cc: $(desktop)
	sed 's#Exec=.*#Exec=root-manager time-admin#' < $< > $@

all-local: $(desktop).menu $(desktop).cc

install-data-local:
	$(INSTALL_DATA) $(srcdir)/time.desktop.menu $(DESTDIR)$(datadir)/gnome/apps/Settings/System/time.desktop
	$(INSTALL_DATA) $(srcdir)/time.desktop.cc $(DESTDIR)$(datadir)/control-center/System/time.desktop
	$(INSTALL) -d $(DESTDIR)$(bindir)
	rm -f $(DESTDIR)$(bindir)/time-admin
	ln -s consolehelper $(DESTDIR)$(bindir)/time-admin
