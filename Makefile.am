## Process this file with automake to produce Makefile.in

SUBDIRS = doc icons interfaces pixmaps src po

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


# gsettingsschemadir and gschema_compile are defined by the GLIB_GSETTINGS
# macro in configure.in
gsettingsschema_DATA = org.gnome.system-tools.gschema.xml
# This rule will check your schemas for validity before installation
@GSETTINGS_CHECK_RULE@
if GSETTINGS_SCHEMAS_INSTALL
install-data-hook:
	$(gschema_compile) $(DESTDIR)$(gsettingsschemadir)
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnome-system-tools.pc

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	backends/intltool-extract \
	backends/intltool-merge \
	backends/intltool-update \
	gnome-doc-utils.make \
	gnome-system-tools.pc

MAINTAINERCLEANFILES = \
	INSTALL			\
	aclocal.m4              \
	config.guess            \
	config.h.in             \
	config.sub              \
	depcomp                 \
	install-sh              \
	ltmain.sh               \
	missing                 \
	mkinstalldirs           \
	omf.make                \
        xmldocs.make		\
	`find "$(srcdir)" -type f -name Makefile.in -print`


DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper


-include $(top_srcdir)/git.mk
