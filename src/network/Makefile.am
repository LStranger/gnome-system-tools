## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/src/common 	\
	-I$(top_srcdir)/intl 		\
	$(GNOME_INCLUDEDIR)		\
	$(GNOME_CFLAGS) 		\
	$(LIBGLADE_CFLAGS)		\
	$(GNOME_XML_CFLAGS)

sbin_PROGRAMS = network-admin

network_admin_SOURCES = \
	main.c \
	callbacks.c	callbacks.h	\
	transfer.c	transfer.h	\
	connection.c	connection.h	\
	ppp-druid.c	ppp-druid.h

network_admin_LDADD = \
$(GNOME_LIBDIR) \
$(GNOMEUI_LIBS) \
$(GNOME_XML_LIB) \
$(GDK_PIXBUF_LIBS) \
$(LIBGLADE_LIBS) \
$(INTLLIBS) \
../common/libsetuptool.a

desktop = network.desktop
security = network.security

CLEANFILES = $(desktop) $(desktop).menu $(desktop).cc $(security)

EXTRA_DIST = $(desktop).in network.pam $(security).in

$(desktop): %.desktop: %.desktop.in
	sed 's#Icon=.*#Icon='$(pixmapsdir)'/networking.png#' < $< > $@

$(desktop).menu: $(desktop)
	sed 's#Exec=.*#Exec=network-admin#' < $< > $@

$(desktop).cc: $(desktop)
	sed 's#Exec=.*#Exec=root-manager network-admin#' < $< > $@

$(security): $(security).in
	sed 's#PROGRAM=.*#PROGRAM='$(DESTDIR)$(sbindir)'/network-admin#' < $< > $@

all-local: $(desktop).menu $(desktop).cc $(security)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/gnome/apps/Settings/System
	$(mkinstalldirs) $(DESTDIR)$(datadir)/control-center/System
	$(INSTALL_DATA) $(srcdir)/network.desktop.menu $(DESTDIR)$(datadir)/gnome/apps/Settings/System/network.desktop
	$(INSTALL_DATA) $(srcdir)/network.desktop.cc $(DESTDIR)$(datadir)/control-center/System/network.desktop
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	rm -f $(DESTDIR)$(bindir)/network-admin
	ln -s consolehelper $(DESTDIR)$(bindir)/network-admin
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pam.d
	$(INSTALL_DATA) $(srcdir)/network.pam $(DESTDIR)$(sysconfdir)/pam.d/network-admin
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/security/console.apps
	$(INSTALL_DATA) $(srcdir)/$(security) $(DESTDIR)$(sysconfdir)/security/console.apps/network-admin
