noinst_LIBRARIES = libsetuptool.a

libsetuptool_a_SOURCES = \
xml.c xml.h \
helpers.c helpers.h \
tool.c tool.h \
global.h \
checked.xpm unchecked.xpm reading.xpm \
gnomesupport.h

libsetuptool_a_CFLAGS = $(GNOME_CFLAGS) $(GDK_PIXBUF_CFLAGS) $(LIBGLADE_CFLAGS) "`gnome-config --cflags gnomecanvaspixbuf`"
