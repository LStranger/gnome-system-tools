## Process this file with automake to produce Makefile.in

SUBDIRS = interfaces backends pixmaps src po
# doc

EXTRA_DIST = \
	HACKING 		\
	BUGS			\
	($schema_in_files) \
	gnome-system-tools.spec \
	intltool-extract.in	\
	intltool-merge.in 	\
	intltool-update.in 


schema_in_files = gnome-system-tools.schemas.in
schemadir = $(sysconfdir)/gconf/schemas
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/gnome-system-tools.schemas
