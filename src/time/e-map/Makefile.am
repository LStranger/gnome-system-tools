INCLUDES = \
	$(GNOME_INCLUDEDIR)	\
	$(GNOME_XML_CFLAGS)	\
	-DMAP_DIR=\""$(pixmapsdir)"\" \
	$(XST_TOOL_CFLAGS)

LDADD = -lm $(XST_TOOL_LIBS)
LIBS+=$(LDADD)

noinst_PROGRAMS = \
e-map-test

e_map_test_CFLAGS = \
	$(GNOME_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS)

e_map_test_LDADD = \
  ../../common/libsetuptool.a

e_map_test_SOURCES = \
  e-map.c e-map.h \
  e-map-test.c
