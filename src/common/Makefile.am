noinst_LIBRARIES = libsetuptool.a

INCLUDES = \
	$(GST_CFLAGS)					\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

libsetuptool_a_SOURCES =				\
	gst-xml.c		gst-xml.h		\
	text.c			text.h			\
	gst-widget.c		gst-widget.h		\
	gst-dialog.c		gst-dialog.h		\
	gst-tool.c		gst-tool.h		\
	gst-report-hook.c	gst-report-hook.h	\
	gst-report-line.c	gst-report-line.h	\
	gst-platform.c		gst-platform.h		\
	gst-ui.c		gst-ui.h		\
	gst-util.c		gst-util.h		\
	gst-conf.c		gst-conf.h		\
	gst-su.c		gst-su.h		\
	gst.h						\
	gst-marshal.h					\
	checked.xpm					\
	unchecked.xpm					\
	reading.xpm					\
	gst-types.h

ccenterdir = $(datadir)/control-center
Systemdir = $(ccenterdir)/System

Gnomeccdirp = $(ccenterdir)/capplets/$(groups)

paneldir = $(datadir)/gnome/apps/Settings
Systemdirp = $(paneldir)/System

panel_DATA = $(groups_files)

EXTRA_DIST = $(directory).in.in $(groups_files) CommonMakefile
CLEANFILES = $(directory) $(groups_files)


all-local: $(directory)

directory = System.directory
groups = System
groups_files = $(groups:=.directory)
groups_in_files = $(groups_files:.directory=.directory.in)

$(groups_in_files): %.directory.in: %.directory.in.in
	    sed s#Icon=#Icon=$(pixmapsdir)/# < $< > $@

$(groups_files): %.directory: %.directory.in
	$(top_builddir)/intltool-merge -d $(top_srcdir)/po $< $@

install-data-local:
##	$(mkinstalldirs) $(DESTDIR)$(Systemdir)
##	$(INSTALL_DATA) $(srcdir)/System.directory $(DESTDIR)$(Systemdir)/.directory
	$(mkinstalldirs) $(DESTDIR)$(Systemdirp)
	$(INSTALL_DATA) System.directory $(DESTDIR)$(Systemdirp)/.directory
	$(mkinstalldirs) $(DESTDIR)$(Gnomeccdirp)
	$(INSTALL_DATA) System.directory $(DESTDIR)$(Gnomeccdirp)/.directory
#	$(mkinstalldirs) $(datadir)/control-center/capplets/$(cappletgroup)
#     $(INSTALL_DATA) $(srcdir)/$(groups).directory $(datadir)/control-center/capplets/$(groups).directory

