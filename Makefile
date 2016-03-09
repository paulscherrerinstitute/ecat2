include /ioc/tools/driver.makefile
PROJECT=ecat2
BUILDCLASSES += Linux
EXCLUDE_VERSIONS = 3.13 3.14.8

#ARCH_FILTER = SL6-x86_64 eldk52-e500v2

ARCH_FILTER = eldk52-e500v2




#USR_CFLAGS_eldk52-e500v2 += -L/net/slsfs01/export/ifc-exchange/ethercat/lib/ 
#USR_CFLAGS_SL6-x86_64    += -L/afs/psi.ch/group/8431/dmm/git/master_sl6/2.0/lib/.libs/
#USR_CFLAGS += -static -lethercat-2.0.2
#USR_CFLAGS +=  
#USR_CFLAGS += -L/net/slsfs01/export/ifc-exchange/ethercat/lib/ 
#USR_CFLAGS_eldk52-e500v2 += -Wl,-rpath=/ifc-exchange/ethercat/lib/
#USR_CFLAGS_eldk52-e500v2 += -Wl,-rpath=/net/slsfs01/export/ifc-exchange/ethercat/lib/



USR_CFLAGS += -fPIC

#---------- e500v2
USR_CFLAGS_eldk52-e500v2 += -I/opt/eldk-5.2/powerpc-e500v2/sysroots/ppce500v2-linux-gnuspe/usr/include/
USR_CFLAGS_eldk52-e500v2 += -fPIC
USR_LDFLAGS_eldk52-e500v2 += -Wl,-whole-archive /net/slsfs01/export/ifc-exchange/ethercat/lib/libethercat-test.a -Wl,-no-whole-archive 


#USR_CFLAGS_eldk52-e500v2 += -L/net/slsfs01/export/ifc-exchange/ethercat/lib/ 
#USR_CFLAGS_eldk52-e500v2 += -Wl,-lethercat
#USR_LDFLAGS_eldk52-e500v2 += -static
#USR_LDFLAGS_eldk52-e500v2 += -Wl,-whole-archive ../libethercat-2.0.2.a -Wl,-no-whole-archive 
#USR_LDFLAGS_eldk52-e500v2 += -Wl,-whole-archive /net/slsfs01/export/ifc-exchange/ethercat/lib/libethercat.a -Wl,-no-whole-archive 
#USR_CFLAGS_eldk52-e500v2 += -Wl,-rpath=/ifc-exchange/ethercat/lib/
#USR_CFLAGS_eldk52-e500v2 += -Wl,-rpath=/net/slsfs01/export/ifc-exchange/ethercat/lib/

#---------- SL6-x86_64
#USR_LDFLAGS_SL6-x86_64 += -L/afs/psi.ch/group/8431/dmm/git/master_sl6/2.0/lib/.libs/ 
#USR_LDFLAGS_SL6-x86_64 += -lethercat
#USR_LDFLAGS_SL6-x86_64 += -L/usr/lib/ -L/usr/lib64/
#USR_LDFLAGS_SL6-x86_64 += -Wl,-rpath=/afs/psi.ch/group/8431/dmm/git/master_sl6/2.0/lib/.libs/
#USR_LDFLAGS_SL6-x86_64 += -static
#USR_CFLAGS += -v -Wl,-v

#USR_LDFLAGS += $(USR_LDFLAGS_$(T_A))

# verbose compiler, verbose linker output
#USR_CFLAGS += -v -Wl,-v
