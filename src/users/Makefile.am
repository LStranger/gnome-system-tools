## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/intl \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = users-admin

users_admin_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	transfer.c transfer.h

users_admin_CFLAGS = $(GNOME_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(LIBGLADE_CFLAGS)

users_admin_LDADD = \
$(GNOME_LIBDIR) \
$(GNOMEUI_LIBS) \
$(GNOME_XML_LIB) \
$(GDK_PIXBUF_LIBS) \
$(LIBGLADE_LIBS) \
$(INTLLIBS) \
../common/libsetuptool.a

desktop = users.desktop

EXTRA_DIST = $(desktop)

CLEANFILES = $(desktop)

$(desktop): %.desktop: %.desktop.in
	sed 's#Icon=.*#Icon='$(pixmapsdir)'/users.png#' < $< > $@

sysdir = $(datadir)/control-center/System
sys_DATA = users.desktop

install-data-local:
	$(INSTALL_DATA) $(srcdir)/users.desktop $(DESTDIR)$(datadir)/gnome/apps/Settings/System/users.desktop
