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
	font.glade

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
	fix.pl				\
	$(interfaces:.glade=.glade.in)	\
	$(interfaces:.glade=.glade.h)

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in fix.pl
	$(top_srcdir)/interfaces/fix.pl $(pixmapsdir) < $< > $@
