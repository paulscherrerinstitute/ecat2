#ifndef EC_H
#define EC_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <alarm.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <features.h>
#include <sys/mman.h>
#include <sys/ioctl.h>



#include <devLib.h>
#include <devSup.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsExport.h>
#include <epicsTypes.h>
#include <dbScan.h>
#include <errlog.h>
#include <dbAccess.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <biRecord.h>
#include <boRecord.h>
#include <mbbiRecord.h>
#include <mbboRecord.h>
#include <longinRecord.h>
#include <longoutRecord.h>
#include <stringinRecord.h>
#include <stringoutRecord.h>
#include <waveformRecord.h>
#include <initHooks.h>
#define HAVE_VME
#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <iocsh.h>
#include <callback.h>
#include <recGbl.h>

#include <time.h>

#include "../ethercatmaster/1.5.2/include/ecrt.h"


#include "eccommon.h"

#include "drvethercat.h"
#include "drvethercatint.h"

#include "ecnode.h"
#include "eccfg.h"
#include "ectimer.h"
#include "ectools.h"
#include "ecengine.h"
#include "ecdebug.h"



#endif


















