interfaces = \
common.glade \
disks.glade \
hardware.glade \
memory.glade \
nameresolution.glade \
networking.glade \
shares.glade \
time.glade \
users.glade

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
fix.pl \
common.glade.in \
common.glade.h \
disks.glade.in \
disks.glade.h \
hardware.glade.in \
hardware.glade.h \
memory.glade.in \
memory.glade.h \
nameresolution.glade.in \
nameresolution.glade.h \
networking.glade.in \
networking.glade.h \
shares.glade.in \
shares.glade.h \
time.glade.in \
time.glade.h \
users.glade.in \
users.glade.h

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in
	./fix.pl $(pixmapsdir) < $< > $@
