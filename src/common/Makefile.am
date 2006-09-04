noinst_LIBRARIES = libsetuptool.a

INCLUDES = \
	$(GST_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DPREFIX=\""$(prefix)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

libsetuptool_a_SOURCES = \
	gst-widget.c		\
	gst-dialog.c		\
	gst-tool.c		\
	gst-platform-dialog.c	\
	gst-conf.c		\
	gst-filter.c		\
	gst-service-role.c

EXTRA_DIST = CommonMakefile
