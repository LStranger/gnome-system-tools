helpdir = $(datadir)/gnome/help/nameresolution-admin/C

help_DATA = index.html gpl.html intro.html identification.html dns.html \
	wins.html searchdomains.html staticnames.html ln14.html

# These are the sources:

help_SGML = nameresolution-admin.sgml gpl.sgml 

EXTRA_DIST = $(help_DATA) $(help_SGML)

# Automake does not understand how to build docbook sources.
# The stylesheets being used to build the html are available from 
# ftp://ftp.cygnus.com/pub/home/rosalia/ (aka Mark Galassi):

DB_STYLESHEET=/usr/lib/sgml/stylesheets/cygnus-both.dsl\#html

%.html: $(help_SGML)
	-db2html nameresolution-admin.sgml >/dev/null;\
	mv nameresolution-admin/*.html .;\
	rm -rf nameresolution-admin
