INCLUDES = \
	$(GNOME_INCLUDEDIR)	\
	$(GNOME_XML_CFLAGS)	\
	$(GST_TOOL_CFLAGS)

EXTRA_DIST= search-bar-marshal.list

search-bar-marshal.h: search-bar-marshal.list
	( @GLIB_GENMARSHAL@ search-bar-marshal.list --header > search-bar-marshal.tmp \
	&& mv search-bar-marshal.tmp search-bar-marshal.h ) \
	|| (rm -f search-bar-marshal.tmp && exit 1 )

search-bar-marshal.c: search-bar-marshal.h
	( @GLIB_GENMARSHAL@ search-bar-marshal.list --body > search-bar-marshal.tmp \
	&& mv search-bar-marshal.tmp search-bar-marshal.c ) \
	|| (rm -f search-bar-marshal.tmp && exit 1 )

($search_bar_la_OBJECTS): search-bar-marshal.h

noinst_LIBRARIES =              \
        libsearchbar.a

BUILT_SOURCES= search-bar-marshal.c search-bar-marshal.h

libsearchbar_a_SOURCES =   \
	search-bar.c       \
	search-bar.h	   \
	search-bar-marshal.c \
	search-bar-marshal.h 


-include $(top_srcdir)/git.mk
