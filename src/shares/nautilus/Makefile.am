INCLUDES = $(NAUTILUS_CFLAGS)				\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)					\
	-I$(top_builddir)

# FIXME: get this from the .pc file
nautilus_extensiondir=$(libdir)/nautilus/extensions-1.0

nautilus_extension_LTLIBRARIES=libnautilus-gst-shares.la

libnautilus_gst_shares_la_SOURCES =     		\
	nautilus-shares.c	nautilus-shares.h	\
	gst-shares-module.c

libnautilus_gst_shares_la_LDFLAGS = -module -avoid-version
libnautilus_gst_shares_la_LIBADD  = $(NAUTILUS_LIBS)

