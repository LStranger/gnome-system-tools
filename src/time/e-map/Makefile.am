CFLAGS = -g -Wall
INCLUDES = $(GNOME_INCLUDEDIR)
LDADD = -lm

bin_PROGRAMS = \
e-map-test

e_map_test_CFLAGS = \
  $(GNOME_CFLAGS) \
  $(GDK_PIXBUF_CFLAGS)

e_map_test_LDADD = \
  $(GNOME_LIBDIR) \
  $(GNOMEUI_LIBS) \
  $(GDK_PIXBUF_LIBS)

e_map_test_SOURCES = \
  e-map.c e-map.h \
  e-map-test.c
