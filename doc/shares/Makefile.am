helpdir = $(datadir)/gnome/help/shares-admin/C

help_DATA = index.html gpl.html intro.html importing.html exporting.html \
	ln14.html

# These are the sources:

help_SGML = shares-admin.sgml gpl.sgml 

EXTRA_DIST = $(help_DATA) $(help_SGML)

# Automake does not understand how to build docbook sources.
# The stylesheets being used to build the html are available from 
# ftp://ftp.cygnus.com/pub/home/rosalia/ (aka Mark Galassi):

DB_STYLESHEET=/usr/lib/sgml/stylesheets/cygnus-both.dsl\#html

%.html: $(help_SGML)
	-db2html shares-admin.sgml >/dev/null;\
	mv shares-admin/*.html .;\
	rm -rf shares-admin
