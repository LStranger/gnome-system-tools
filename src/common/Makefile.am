noinst_LIBRARIES = libsetuptool.a

INCLUDES = \
	$(GST_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
        -DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
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
	gst-auth.c		gst-auth.h		\
	gst-hig-dialog.c	gst-hig-dialog.h	\
	gst-filter.c		gst-filter.h		\
	gst.h						\
	gst-marshal.h					\
	gst-types.h
