boot_glade = boot.glade
network_glade = network.glade
services_glade = services.glade
time_glade = time.glade
users_glade = users.glade
disks_glade = disks.glade

interfaces = common.glade	\
	$(boot_glade) 		\
	$(network_glade)	\
	$(services_glade)	\
	$(time_glade)		\
	$(users_glade)		\
	$(disks_glade)

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
	$(interfaces:.glade=.glade.in)	\
	fix.pl

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in
	$(top_srcdir)/interfaces/fix.pl $(pixmapsdir) < $< > $@
