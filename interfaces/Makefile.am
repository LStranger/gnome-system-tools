interfaces = \
	common.glade					\
	disks.glade 					\
	dhcpd.glade					\
	memory.glade 					\
	nameresolution.glade			\
	network.glade 					\
	shares.glade 					\
	time.glade 					\
	users.glade 					\
	boot.glade					\
	location-manager.glade			\
	internetsharing.glade			\
	rollback-location-management.glade \
	display.glade					\
	print.glade					\
	runlevel.glade					\
	font.glade

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
	$(interfaces:.glade=.glade.in)	\
	$(interfaces:.glade=.glade.h)   \
	fix.pl

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in
	$(top_srcdir)/interfaces/fix.pl $(pixmapsdir) < $< > $@
