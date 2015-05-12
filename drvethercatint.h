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

















