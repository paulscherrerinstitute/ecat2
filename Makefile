include /ioc/tools/driver.makefile
PROJECT=ecat2
BUILDCLASSES += Linux
EXCLUDE_VERSIONS = 3.13 3.14.8
EXCLUDE_ARCHS = "ppc4xx SL5 SL6 eldk42 eldk51 T2 T202 rt xenomai mvl40-xscale_be V63 V67 ppc603 ppc604"

USR_CFLAGS += -I/opt/eldk-5.2/powerpc-e500v2/sysroots/ppce500v2-linux-gnuspe/usr/include/

#USR_LDFLAGS  += -L/net/slsfs01/export/ifc-exchange/ethercat/lib/ -Wl,-rpath=/ifc-exchange/ethercat/lib/:-rpath=/net/slsfs01/export/ifc-exchange/ethercat/lib/ -lethercat

# test
USR_LDFLAGS  += -L/net/slsfs01/export/ifc-exchange/ethercat/lib/ -Wl,-rpath=/ifc-exchange/ethercat/lib/:-rpath=/net/slsfs01/export/ifc-exchange/ethercat/lib/ -lethercat

#LIBOBJS+=../../ethercatmaster/2.0/lib/.libs/libethercat_la-common.o
#LIBOBJS+=../../ethercatmaster/2.0/lib/.libs/libethercat_la-domain.o
#LIBOBJS+=../../ethercatmaster/2.0/lib/.libs/libethercat_la-master.o
#LIBOBJS+=../../ethercatmaster/2.0/lib/.libs/libethercat_la-reg_request.o
#LIBOBJS+=../../ethercatmaster/2.0/lib/.libs/libethercat_la-sdo_request.o
#LIBOBJS+=../../ethercatmaster/2.0/lib/.libs/libethercat_la-slave_config.o
#LIBOBJS+=../../ethercatmaster/2.0/lib/.libs/libethercat_la-voe_handler.o
USR_LDFLAGS += -lrt

