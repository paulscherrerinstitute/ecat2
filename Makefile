include /ioc/tools/driver.makefile
PROJECT=ecat2
BUILDCLASSES += Linux
EXCLUDE_VERSIONS = 3.13 3.14.8

#ARCH_FILTER = SL6-x86_64 eldk52-e500v2

ARCH_FILTER = eldk52-e500v2

USR_CFLAGS += -fPIC
USR_LDFLAGS += -L /ioc/modules/ecat2/ifc/lib/
USR_LDFLAGS += -lethercat

#---------- e500v2
USR_LDFLAGS_eldk52-e500v2 += -Wl,-rpath=/ioc/modules/ecat2/ifc/lib  

#---------- SL6
USR_LDFLAGS_SL6-x86_64 += -Wl,-rpath=/ioc/modules/ecat2/sl6/lib  

# verbose compiler, verbose linker output
#USR_CFLAGS += -v -Wl,-v
