## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/src/common 	\
	-I$(top_srcdir)/intl 		\
	$(GNOME_INCLUDEDIR)		\
	$(GNOME_CFLAGS) 		\
	$(GDK_PIXBUF_CFLAGS) 		\
	$(LIBGLADE_CFLAGS)		\
	$(GNOME_XML_CFLAGS)

sbin_PROGRAMS = users-admin

users_admin_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	transfer.c transfer.h \
	md5.c md5.h md5_crypt.c

users_admin_CFLAGS = 

users_admin_LDADD = \
$(GNOME_LIBDIR) \
$(GNOMEUI_LIBS) \
$(GNOME_XML_LIB) \
$(GDK_PIXBUF_LIBS) \
$(LIBGLADE_LIBS) \
$(INTLLIBS) \
../common/libsetuptool.a

desktop = users.desktop

EXTRA_DIST = $(desktop).in

CLEANFILES = $(desktop) $(desktop).menu $(desktop).cc

$(desktop): %.desktop: %.desktop.in
	sed 's#Icon=.*#Icon='$(pixmapsdir)'/users.png#' < $< > $@

$(desktop).menu: $(desktop)
	sed 's#Exec=.*#Exec=users-admin#' < $< > $@

$(desktop).cc: $(desktop)
	sed 's#Exec=.*#Exec=root-manager users-admin#' < $< > $@

all-local: $(desktop).menu $(desktop).cc

install-data-local: $(desktop).menu $(desktop).cc
	$(INSTALL_DATA) $(srcdir)/users.desktop.menu $(DESTDIR)$(datadir)/gnome/apps/Settings/System/users.desktop
	$(INSTALL_DATA) $(srcdir)/users.desktop.cc $(DESTDIR)$(datadir)/control-center/System/users.desktop
	$(INSTALL) -d $(DESTDIR)$(bindir)
	rm -f $(DESTDIR)$(bindir)/users-admin
	ln -s consolehelper $(DESTDIR)$(bindir)/users-admin
