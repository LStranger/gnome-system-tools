interfaces = \
	boot.glade	\
	common.glade	\
	disks.glade	\
	network.glade	\
	services.glade  \
	shares.glade	\
	time.glade	\
	users.glade

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
	$(interfaces:.glade=.glade.in)	\
	fix.pl

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in
	$(top_srcdir)/interfaces/fix.pl $(pixmapsdir) < $< > $@
