helpdir = $(datadir)/gnome/help/networking-admin/C

help_DATA = index.html intro.html primary.html	\
	ln14.html

# These are the sources:

help_SGML = networking-admin.sgml

EXTRA_DIST = $(help_DATA) $(help_SGML)

# Automake does not understand how to build docbook sources.
# The stylesheets being used to build the html are available from 
# ftp://ftp.cygnus.com/pub/home/rosalia/ (aka Mark Galassi):

DB_STYLESHEET=/usr/lib/sgml/stylesheets/cygnus-both.dsl\#html

%.html: $(help_SGML)
	-db2html networking-admin.sgml >/dev/null;\
	mv networking-admin/*.html .;\
	rm -rf networking-admin
