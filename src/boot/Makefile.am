## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/src/common 	\
	-I$(top_srcdir)/intl 		\
	$(GNOME_INCLUDEDIR)		\
	$(GNOME_CFLAGS) 		\
	$(GDK_PIXBUF_CFLAGS) 		\
	$(LIBGLADE_CFLAGS)		\
	$(GNOME_XML_CFLAGS)		\
	$(GAL_CFLAGS)

sbin_PROGRAMS = boot-admin

boot_admin_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	transfer.c transfer.h \
	e-table.c e-table.h \
	boot-settings.c boot-settings.h 

boot_admin_CFLAGS = 

boot_admin_LDADD = \
$(GNOME_LIBDIR) \
$(GNOMEUI_LIBS) \
$(GNOME_XML_LIB) \
$(GDK_PIXBUF_LIBS) \
$(LIBGLADE_LIBS) \
$(INTLLIBS) \
$(GAL_LIBS) \
../common/libsetuptool.a

#desktop = boot.desktop
#security = boot.security
#EXTRA_DIST = $(desktop).in boot.pam $(security).in
#CLEANFILES = $(desktop) $(desktop).menu $(desktop).cc $(security)

toolname = boot
desktop  = $(toolname).desktop
security = $(toolname).security

CLEANFILES = \
	$(desktop) \
	$(desktop).menu \
	$(desktop).cc \
	$(security)

EXTRA_DIST = \
	$(toolname).pam	\
	$(security)		\
	$(security).in		\
	$(desktop).in 		\
	$(desktop).cc		\
	$(desktop).menu

$(desktop): %.desktop: %.desktop.in
	sed 's#Icon=.*#Icon='$(pixmapsdir)'/boot.png#' < $< > $@

$(desktop).menu: $(desktop)
	sed 's#Exec=.*#Exec=boot-admin#' < $< > $@

$(desktop).cc: $(desktop)
	sed 's#Exec=.*#Exec=boot-admin#' < $< > $@

#	sed 's#Exec=.*#Exec=root-manager boot-admin#' < $< > $@

$(security): $(security).in
	sed 's#PROGRAM=.*#PROGRAM='$(DESTDIR)$(sbindir)'/boot-admin#' < $< > $@

all-local: $(desktop).menu $(desktop).cc $(security)

install-data-local: $(desktop).menu $(desktop).cc
	$(mkinstalldirs) $(DESTDIR)$(datadir)/gnome/apps/Settings/System
##	$(mkinstalldirs) $(DESTDIR)$(datadir)/control-center/System
	$(INSTALL_DATA) boot.desktop.menu $(DESTDIR)$(datadir)/gnome/apps/Settings/System/boot.desktop
##	$(INSTALL_DATA) $(srcdir)/boot.desktop.cc $(DESTDIR)$(datadir)/control-center/System/boot.desktop
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	rm -f $(DESTDIR)$(bindir)/boot-admin
	ln -s consolehelper $(DESTDIR)$(bindir)/boot-admin
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pam.d
	$(INSTALL_DATA) $(srcdir)/boot.pam $(DESTDIR)$(sysconfdir)/pam.d/boot-admin
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/security/console.apps
	$(INSTALL_DATA) $(srcdir)/$(security) $(DESTDIR)$(sysconfdir)/security/console.apps/boot-admin
