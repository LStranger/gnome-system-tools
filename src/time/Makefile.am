## Process this file with automake to produce Makefile.in

SUBDIRS = e-map

INCLUDES = $(XST_TOOL_CFLAGS)

bin_PROGRAMS = time-admin

time_admin_SOURCES = \
	main.c            time-tool.h \
	xst-spin-button.c xst-spin-button.h \
	tz.c              tz.h \
	tz-map.c          tz-map.h \
	timeserv.c        timeserv.h \
	transfer.c        transfer.h 

time_admin_LDADD = 	$(XST_TOOL_LIBS) ./e-map/e-map.o

#desktop = time.desktop
#security = time.security
#EXTRA_DIST = $(desktop).in time.pam $(security).in
#CLEANFILES = $(desktop) $(desktop).menu $(desktop).cc $(security)

toolname = time
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
	sed 's#Icon=.*#Icon='$(pixmapsdir)'/time.png#' < $< > $@

$(desktop).menu: $(desktop)
	sed 's#Exec=.*#Exec=time-admin#' < $< > $@

$(desktop).cc: $(desktop)
	sed 's#Exec=.*#Exec=time-admin#' < $< > $@

#	sed 's#Exec=.*#Exec=root-manager time-admin#' < $< > $@

$(security): $(security).in
	sed 's#PROGRAM=.*#PROGRAM='$(DESTDIR)$(sbindir)'/time-admin#' < $< > $@

all-local: $(desktop).menu $(desktop).cc $(security)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/gnome/apps/Settings/System
##	$(mkinstalldirs) $(DESTDIR)$(datadir)/control-center/System
	$(INSTALL_DATA) time.desktop.menu $(DESTDIR)$(datadir)/gnome/apps/Settings/System/time.desktop
##	$(INSTALL_DATA) $(srcdir)/time.desktop.cc $(DESTDIR)$(datadir)/control-center/System/time.desktop
##	$(mkinstalldirs) $(DESTDIR)$(bindir)
##	rm -f $(DESTDIR)$(bindir)/time-admin
##	ln -s consolehelper $(DESTDIR)$(bindir)/time-admin
##	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pam.d
##	$(INSTALL_DATA) $(srcdir)/time.pam $(DESTDIR)$(sysconfdir)/pam.d/time-admin
##	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/security/console.apps
##	$(INSTALL_DATA) $(srcdir)/$(security) $(DESTDIR)$(sysconfdir)/security/console.apps/time-admin
