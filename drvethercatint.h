#ifndef DRVECATINT_H
#define DRVECATINT_H

/*


------------------------------------

 Various support macros

------------------------------------
#define _DMMTP(a,b) long devethercat_##a##_##b
#define _DMMTP2(a,b) _DMMTP(a,b)
#define _DMMTP3(a,b) devethercat_##a##_##b

#define _dev_init_record(rt) _DMMTP3( init_record, rt )
#define dev_init_record(rt) _DMMTP2( init_record, rt( rt##Record *record ) )
#define _dev_read(rt) _DMMTP3( read, rt )
#define dev_read(rt) _DMMTP2( read, rt( rt##Record *record ) )
#define _dev_write(rt) _DMMTP3( write, rt )
#define dev_write(rt) _DMMTP2( write, rt( rt##Record *record ) )
#define _dev_special_linconv(rt) _DMMTP3( special_linconv, rt )
#define dev_special_linconv(rt) _DMMTP2( special_linconv, rt( rt##Record *record, int after ) )
*/

#define drvethercatNAME    "EtherCAT driver: "
#define dmmDEBUGVAR    drvethercatDebug
#define DBG_all        0x12345

#define dbgprinterr  printf( "%s:%d (C%02d): ", __FILE__, __LINE__, card->cardnumber )
#define printerr     errlogSevPrintf(errlogFatal, "%s:%d (C%02d): ", __FILE__, __LINE__, card->cardnumber )
#define printerr2    errlogSevPrintf(errlogFatal, drvethercatNAME " (card no. %d): ", card->cardnumber )
#define printerr3    errlogSevPrintf(errlogFatal, drvethercatNAME )

#define dmmARGCHECK( cond, errmsg ) \
            do { if( cond ) { printf( drvicsNAME ": " errmsg "\n"); return S_dev_badArgument; } } while(0)

#define dmmCHECKerrno( cond, errmsg ) \
            do { if( cond ) { printerr2; dmmdebug errmsg; return errno; } } while(0)

#define dmmCHECKret( cond, errmsg, retv ) \
            do { if( cond ) { printerr2; dmmdebug errmsg; return retv; } } while(0)

#define dmmCHECK( cond, errmsg, retv ) \
            do { if( cond ) { printerr3; dmmdebug errmsg; return retv; } } while(0)

#define dmmERR( errmsg, retv ) \
            do { printerr3; dmmdebug errmsg; return retv; } while(0)


#define dmmDEBUG(debuglevel, errmsg) \
        do { if( dmmDEBUGVAR >= debuglevel || dmmDEBUGVAR == DBG_all )  { dbgprinterr; dmmdebug errmsg;}  } while (0)

#define dmmDEBUGeq(debuglevel, errmsg)  \
        do { if( (dmmDEBUGVAR & debuglevel) || dmmDEBUGVAR == DBG_all ) { dbgprinterr; dmmdebug errmsg;}  } while (0)

#define dmmDEBUGblk(debuglevel, execblock)  \
        do { if( (dmmDEBUGVAR & debuglevel) || dmmDEBUGVAR == DBG_all ) { dbgprinterr; execblock;}  } while (0)

/*------------------------------------*/
/*                                    */
/* Debug                              */
/*                                    */
/*------------------------------------*/
enum dmmDBG {
    DBG_irqwt2 = 0x00000002,     /* irq wt pass, count    */
    DBG_irqwt1 = 0x00000001,     /* irq wt channels       */
    DBG_init   = 0x00000004,     /*                       */
    DBG_4      = 0x00000008,     /*                       */
    DBG_dma1   = 0x00000010,     /* DMA done              */
    DBG_dma2   = 0x00000020,     /* DMA transfer info     */
    DBG_dma3   = 0x00000040,     /* DMA error             */
    DBG_8      = 0x00000080,     /*                       */
    DBG_9      = 0x00000100,     /*                       */
    DBG_10     = 0x00000200,     /*                       */
    DBG_11     = 0x00000400,     /*                       */
    DBG_12     = 0x00000800,     /*                       */
    DBG_write  = 0x00001000,     /* write                 */
    DBG_read   = 0x00002000,     /* read                  */
    DBG_15     = 0x00004000,     /*                       */
    DBG_16     = 0x00008000,     /*                       */

};




/*------------------------------------*/
/*                                    */
/* devethercat                        */
/*                                    */
/*------------------------------------*/
#define MAGIC 0xbc0d7413 /* crc("PSI EtherCAT") */

typedef struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN readwrite;
    DEVSUPFUN special_linconv;
} devsup;



#define devsupport(rectype) \
devsup devethercat##rectype = {             \
    6,                                      \
    dev_report,                             \
    dev_init,                               \
    dev_init_record,                        \
    dev_get_ioint_info,                     \
    dev_rw_##rectype,             			\
    NULL                                    \
};											\
epicsExportAddress( dset, devethercat##rectype );


#define devsupport_linconv(rectype) \
devsup devethercat##rectype = {             \
    6,                                      \
    dev_report,                             \
    dev_init,                               \
    dev_init_record,                        \
    dev_get_ioint_info,                     \
    dev_rw_##rectype,                       \
    dev_special_linconv_##rectype           \
};											\
epicsExportAddress( dset, devethercat##rectype );


typedef enum {
	REC_ERROR = 0,

	REC_AI,
	REC_AO,
	REC_BI,
	REC_BO,
	REC_MBBI,
	REC_MBBO,
	REC_MBBIDIRECT,
	REC_MBBODIRECT,
	REC_LONGIN,
	REC_LONGOUT,
	REC_STRINGIN,
	REC_STRINGOUT,
	REC_WAVEFORM,
	REC_FANOUT,
	REC_DFANOUT,
	REC_CALC,
	REC_CALCOUT,
	REC_CPID,

	REC_LAST,
} RECTYPE;

typedef enum {
	RIO_ERROR = 0x00,
	RIO_READ = 0x01,
	RIO_WRITE,
	RIO_RW,

} RECIOTYPE;

typedef struct {
	const char *recname;
	RECTYPE rtype;
	RECIOTYPE riotype;
} _rectype;


#define OK	1
#define NOTOK 0
#define FAIL NOTOK
#define FAILURE FAIL
#define EPICFAIL FAILURE



#define pinfo(fmt, args...) printf( fmt, ##args )
#define perr(fmt, args...)  printf( fmt, ##args )
#define pwarn(fmt, args...)   printf( fmt, ##args )
#define pdbg(fmt, args...)    printk( fmt, ##args )

#define perrret(fmt, args...) { printf( fmt, ##args ); return 0; }
#define perrret2(fmt, args...) { printf( fmt, ##args ); return; }
#define perrex(fmt, args...) { printf( fmt, ##args ); exit(1); }


ethcat *drvFindDomain( int dnr );



#endif

















