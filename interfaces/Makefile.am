interfaces = \
	boot.glade	\
	common.glade	\
	network.glade	\
	time.glade	\
	users.glade	\
	runlevel.glade
#	font.glade
#	disks.glade
#	dhcpd.glade
#	memory.glade
#	nameresolution.glade
#	shares.glade
#	location-manager.glade
#	internetsharing.glade 
#	rollback-location-management.glade
#	display.glade
#	print.glade

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
	$(interfaces:.glade=.glade.in)	\
	$(interfaces:.glade=.glade.h)   \
	fix.pl

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in
	$(top_srcdir)/interfaces/fix.pl $(pixmapsdir) < $< > $@
