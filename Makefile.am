## Process this file with automake to produce Makefile.in

SUBDIRS = doc interfaces pixmaps src po

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

EXTRA_DIST = \
	HACKING 		\
	BUGS			\
	$(schema_in_files)      \
	intltool-extract.in	\
	intltool-merge.in 	\
	intltool-update.in 	\
	omf.make		\
	xmldocs.make		\
	gnome-doc-utils.make	\
	gnome-system-tools.pc.in


schema_in_files = gnome-system-tools.schemas.in
schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/$(schema_DATA)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnome-system-tools.pc

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	backends/intltool-extract \
	backends/intltool-merge \
	backends/intltool-update \
	gnome-system-tools.schemas \
	gnome-doc-utils.make \
	gnome-system-tools.pc

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

