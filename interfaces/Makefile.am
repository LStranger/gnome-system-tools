interfaces = \
	common.glade					\
	disks.glade 					\
	memory.glade 					\
	nameresolution.glade			\
	network.glade 					\
	internet_connection_druid.glade	\
	networking.glade 				\
	shares.glade 					\
	time.glade 					\
	users.glade 					\
	boot.glade

interfaces_DATA = $(interfaces)

EXTRA_DIST = \
	fix.pl 						\
	common.glade.in 				\
	common.glade.h 				\
	disks.glade.in 				\
	disks.glade.h 					\
	memory.glade.in 				\
	memory.glade.h 				\
	nameresolution.glade.in			\
	nameresolution.glade.h 			\
	network.glade.in 				\
	network.glade.h 				\
	internet_connection_druid.glade.in	\
	internet_connection_druid.glade.h 	\
	networking.glade.in 			\
	networking.glade.h				\
	shares.glade.in				\
	shares.glade.h					\
	time.glade.in					\
	time.glade.h					\
	users.glade.in					\
	users.glade.h					\
	boot.glade.in					\
	boot.glade.h

CLEANFILES = $(interfaces)

$(interfaces): %.glade: %.glade.in
	./fix.pl $(pixmapsdir) < $< > $@
