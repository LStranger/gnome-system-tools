INCLUDES = \
	$(GNOME_INCLUDEDIR)	\
	$(GNOME_XML_CFLAGS)	\
	-DMAP_DIR=\""$(pixmapsdir)"\"

LDADD = -lm

noinst_PROGRAMS = \
e-map-test

e_map_test_CFLAGS = \
	$(GNOME_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS)

e_map_test_LDADD = \
  $(GNOME_LIBDIR) \
  $(GNOMEUI_LIBS) \
  $(GNOME_XML_LIB) \
  $(GDK_PIXBUF_LIBS) \
  $(LIBGLADE_LIBS) \
  ../../common/libsetuptool.a

e_map_test_SOURCES = \
  e-map.c e-map.h \
  e-map-test.c
