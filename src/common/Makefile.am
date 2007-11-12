noinst_LIBRARIES = libsetuptool.a

INCLUDES = \
	$(GST_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DPREFIX=\""$(prefix)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

libsetuptool_a_SOURCES = \
	gst-dialog.c		gst-dialog.h \
	gst-tool.c		gst-tool.h \
	gst-platform-dialog.c	gst-platform-dialog.h \
	gst-conf.c		gst-conf.h \
	gst-filter.c		gst-filter.h \
	gst-service-role.c	gst-service-role.h \
	gst.h

if HAVE_POLKIT
libsetuptool_a_SOURCES += \
	gst-polkit-action.c gst-polkit-action.h \
	gst-polkit-button.c gst-polkit-button.h
endif

EXTRA_DIST = CommonMakefile $(headers)
