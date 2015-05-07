include /ioc/tools/driver.makefile
PROJECT=ecat2
BUILDCLASSES += Linux
EXCLUDE_VERSIONS = 3.13 3.14.8
EXCLUDE_ARCHS = "ppc4xx SL5 SL6 eldk42 eldk51 T2 T202 rt xenomai mvl40-xscale_be V63 V67 ppc603 ppc604"

USR_CFLAGS += -I/opt/eldk-5.2/powerpc-e500v2/sysroots/ppce500v2-linux-gnuspe/usr/include/


#LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat.a

LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat_la-common.o
LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat_la-domain.o
LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat_la-master.o
LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat_la-reg_request.o
LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat_la-sdo_request.o
LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat_la-slave_config.o
LIBOBJS+=../../ethercatmaster/1.5.2/lib/.libs/libethercat_la-voe_handler.o
USR_LDFLAGS += -lrt

