interfaces = \
	common.glade					\
	disks.glade 					\
	memory.glade 					\
	nameresolution.glade			\
	network.glade 					\
	shares.glade 					\
	time.glade 					\
	users.glade 					\
	boot.glade					\
	location-manager.glade			\
	internetsharing.glade			\
	print.glade

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
	fix.pl				\
	$(interfaces:.glade=.glade.in)	\
	$(interfaces:.glade=.glade.h)

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in
	$(top_srcdir)/interfaces/fix.pl $(pixmapsdir) < $< > $@
