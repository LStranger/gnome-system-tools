if BUILD_BOOT
boot_SUBDIR = boot
endif

if BUILD_NETWORK
network_SUBDIR = network
endif

if BUILD_SERVICES
services_SUBDIR = services
endif

if BUILD_TIME
time_SUBDIR = time
endif

if BUILD_USERS
users_SUBDIR = users
endif

if BUILD_DISKS
disks_SUBDIR = disks
endif

if BUILD_SHARES
shares_SUBDIR = shares
endif

# FIXME: no boot nor disks ATM
SUBDIRS = \
	common			\
	$(network_SUBDIR)	\
	$(services_SUBDIR)	\
	$(shares_SUBDIR)	\
	$(time_SUBDIR)		\
	$(users_SUBDIR)
