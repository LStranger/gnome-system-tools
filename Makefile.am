## Process this file with automake to produce Makefile.in

SUBDIRS = doc icons interfaces pixmaps src po

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

EXTRA_DIST = \
	HACKING 		\
	BUGS			\
	intltool-extract.in	\
	intltool-merge.in 	\
	intltool-update.in 	\
	omf.make		\
	xmldocs.make		\
	gnome-doc-utils.make	\
	gnome-system-tools.pc.in \
	org.gnome.system-tools.gschema.xml.in

gsettings_SCHEMAS = org.gnome.system-tools.gschema.xml
@GSETTINGS_RULES@
@INTLTOOL_XML_NOMERGE_RULE@

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
	gnome-system-tools.pc \
	org.gnome.system-tools.gschema.xml

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
