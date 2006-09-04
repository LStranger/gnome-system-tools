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

headers = \
	gst-widget.h		\
	gst-dialog.h		\
	gst-tool.h		\
	gst-platform-dialog.h	\
	gst-conf.h		\
	gst-filter.h		\
	gst-service-role.h	\
	gst-types.h		\
	gst-marshal.h		\
	gst.h

EXTRA_DIST = CommonMakefile $(headers)
