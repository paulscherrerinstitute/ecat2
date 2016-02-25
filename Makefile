include /ioc/tools/driver.makefile
PROJECT=ecat2
BUILDCLASSES += Linux
EXCLUDE_VERSIONS = 3.13 3.14.8

ARCH_FILTER = SL6-x86_64 eldk52-e500v2

#---------- e500v2
USR_CFLAGS_eldk52-e500v2 += -I/opt/eldk-5.2/powerpc-e500v2/sysroots/ppce500v2-linux-gnuspe/usr/include/
USR_LDFLAGS_eldk52-e500v2 += -L/net/slsfs01/export/ifc-exchange/ethercat/lib/ -Wl,-rpath=/ifc-exchange/ethercat/lib/:-rpath=/net/slsfs01/export/ifc-exchange/ethercat/lib/ -lethercat

#---------- SL6-x86_64
USR_LDFLAGS_SL6-x86_64 += -L/afs/psi.ch/group/8431/dmm/git/master_sl6/2.0/lib/.libs/ -lethercat
USR_LDFLAGS_SL6-x86_64 += -Wl,-rpath=/afs/psi.ch/group/8431/dmm/git/master_sl6/2.0/lib/.libs/

USR_LDFLAGS += $(USR_LDFLAGS_$(T_A))

# verbose compiler, verbose linker output
#USR_CFLAGS += -v -Wl,-v
