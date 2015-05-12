#ifndef DRVETHERCAT_H
#define DRVETHERCAT_H


#define MAXSTR	(256+1)


/*----------------------------------------*/
/*                                        */
/* drvethercat                            */
/*                                        */
/*----------------------------------------*/

typedef struct {
	int offs;
	int bit;
	int bitlen;

	int bitspec;

	int rw_dir;
} domain_register;




typedef struct {
	int dnr;
	char *rmem;
	char *wmem;

	int dreg_nr;
	domain_register *dreg_info;

} ioctl_trans;



typedef struct ecat {
	struct ecat *next;


	ecnode *m;
	ecnode *d;
	long rate;
	double freq;

	IOSCANPVT r_scan;
	IOSCANPVT w_scan;

	int dnr; // domain nr

	char *mmap_fname;

	char *r_data;
	char *w_data;

	char *w_mask;

	epicsMutexId rw_lock;

	int dsize;
	epicsThreadId dthread; // domain worker thread

	epicsEventId irq;
	epicsThreadId irqthread; // domain irq thread



	int test;

} ethcat;

//------------------------------------------------------
#define EC_MAX_NUM_DEVICES 1
#define EC_DATAGRAM_NAME_SIZE 20
/** Size of the EtherCAT address field. */
#define EC_ADDR_LEN 4
#define EC_MAX_DOMAINS	10

//------------------------------------------------------

void process_hooks( initHookState state );


ethcat *ethercatOpen( int domain_nr );

int drvGetRegisterDesc( ethcat *e, domain_register *dreg, int regnr, ecnode **pentry, int b_nr );
int drvGetEntryDesc( ethcat *e, domain_register *dreg, int *dreg_nr, ecnode **pentry, int s_nr, int sm_nr, int p_nr, int e_nr, int b_nr );
int drvDomainExists( int mnr, int dnr );

int drvGetValue( ethcat *e, int offs, int bit, epicsUInt32 *rval, int bitlen, int bitspec, int wrval );
int drvSetValue( ethcat *e, int offs, int bit, epicsUInt32 *rval, int bitlen, int bitspec );

int drvGetValueMasked( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, epicsInt16 nobt, epicsUInt16 shift, epicsUInt32 mask );
int drvSetValueMasked( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, epicsInt16 nobt, epicsUInt16 shift, epicsUInt32 mask );


extern int wt_counter[EC_MAX_DOMAINS];
extern int delayed[EC_MAX_DOMAINS];
extern int recd[EC_MAX_DOMAINS];
extern int forwarded[EC_MAX_DOMAINS];


#endif


















