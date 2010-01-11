INCLUDES = $(GST_TOOL_CFLAGS)				\
	$(NAUTILUS_CFLAGS)				\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)					\
	-I$(top_builddir)

nautilus_extensiondir=$(NAUTILUSDIR)

nautilus_extension_LTLIBRARIES=libnautilus-gst-shares.la

libnautilus_gst_shares_la_SOURCES =     		\
	nautilus-shares.c	nautilus-shares.h	\
	gst-shares-module.c

libnautilus_gst_shares_la_LDFLAGS = -module -avoid-version
libnautilus_gst_shares_la_LIBADD  = $(NAUTILUS_LIBS)


-include $(top_srcdir)/git.mk
