## Process this file with automake to produce Makefile.in

SUBDIRS = e-search-bar

INCLUDES = \
	-I$(top_srcdir)/src/common 	\
	-I$(top_srcdir)/intl 		\
	$(GNOME_INCLUDEDIR)		\
	$(GNOME_CFLAGS) 		\
	$(GDK_PIXBUF_CFLAGS) 		\
	$(LIBGLADE_CFLAGS)		\
	$(GNOME_XML_CFLAGS)		\
	$(GAL_CFLAGS)

sbin_PROGRAMS = users-admin

users_admin_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	passwd.c passwd.h \
	transfer.c transfer.h \
	user_group.c user_group.h \
	md5.c md5.h md5_crypt.c \
	e-table.c e-table.h  \
	user_settings.c user_settings.h \
	profile.c profile.h

users_admin_CFLAGS = 

users_admin_LDADD = \
$(GNOME_LIBDIR) \
$(GNOMEUI_LIBS) \
$(GNOME_XML_LIB) \
$(GDK_PIXBUF_LIBS) \
$(LIBGLADE_LIBS) \
$(INTLLIBS) \
$(GAL_LIBS) \
../common/libsetuptool.a \
./e-search-bar/libesearchbar.a

#desktop = users.desktop
#security = users.security
#EXTRA_DIST = $(desktop).in users.pam $(security).in
#CLEANFILES = $(desktop) $(desktop).menu $(desktop).cc $(security)

toolname = users
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
	sed 's#Icon=.*#Icon='$(pixmapsdir)'/users.png#' < $< > $@

$(desktop).menu: $(desktop)
	sed 's#Exec=.*#Exec=users-admin#' < $< > $@

$(desktop).cc: $(desktop)
	sed 's#Exec=.*#Exec=users-admin#' < $< > $@

#	sed 's#Exec=.*#Exec=root-manager users-admin#' < $< > $@

$(security): $(security).in
	sed 's#PROGRAM=.*#PROGRAM='$(DESTDIR)$(sbindir)'/users-admin#' < $< > $@

all-local: $(desktop).menu $(desktop).cc $(security)

install-data-local: $(desktop).menu $(desktop).cc
	$(mkinstalldirs) $(DESTDIR)$(datadir)/gnome/apps/Settings/System
##	$(mkinstalldirs) $(DESTDIR)$(datadir)/control-center/System
	$(INSTALL_DATA) $(srcdir)/users.desktop.menu $(DESTDIR)$(datadir)/gnome/apps/Settings/System/users.desktop
##	$(INSTALL_DATA) $(srcdir)/users.desktop.cc $(DESTDIR)$(datadir)/control-center/System/users.desktop
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	rm -f $(DESTDIR)$(bindir)/users-admin
	ln -s consolehelper $(DESTDIR)$(bindir)/users-admin
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pam.d
	$(INSTALL_DATA) $(srcdir)/users.pam $(DESTDIR)$(sysconfdir)/pam.d/users-admin
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/security/console.apps
	$(INSTALL_DATA) $(srcdir)/$(security) $(DESTDIR)$(sysconfdir)/security/console.apps/users-admin
