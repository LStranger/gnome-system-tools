interfaces_DATA = \
common.glade \
disks.glade \
hardware.glade \
memory.glade \
nameresolution.glade \
networking.glade \
shares.glade \
time.glade \
users.glade

EXTRA_DIST = $(interfaces_DATA) \
common.glade.h \
disks.glade.h \
hardware.glade.h \
memory.glade.h \
nameresolution.glade.h \
networking.glade.h \
shares.glade.h \
time.glade.h \
users.glade.h

users.glade: %.glade: %.glade.in
	sed 's#<pixmaps_directory>.*</pixmaps_directory>#<pixmaps_directory>'$(pixmapsdir)'</pixmaps_directory>#' < $< > $@
