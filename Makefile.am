## Process this file with automake to produce Makefile.in

SUBDIRS = interfaces backends pixmaps src po
# doc

EXTRA_DIST = \
	HACKING 		\
	BUGS			\
	$(schema_in_files)      \
	gnome-system-tools.spec \
	intltool-extract.in	\
	intltool-merge.in 	\
	intltool-update.in 	\
	intltool-modules/XML/Parser/Style/OrigTree.pm \
	gnome-system-tools.pc.in


schema_in_files = gnome-system-tools.schemas.in
schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/$(schema_DATA)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnome-system-tools.pc

CLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	backends/intltool-extract \
	backends/intltool-merge \
	backends/intltool-update \
	gnome-system-tools.schemas \
	gnome-system-tools.pc
