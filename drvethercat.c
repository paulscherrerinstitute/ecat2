#include "ec.h"

#include <fcntl.h>
#include <errno.h>
#include <alarm.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

//-------------------------------------------------------------------


#define ECAT_DNAME	"ecat_d%d"

int drvethercatDebug = 0;
static ethcat *ecatList = NULL;

/*
static void dmmdebug( const char *format, ... )
{
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    fflush( stderr );
}
*/


static inline void *_zalloc( int size )
{
	void *p = calloc( 1, size );
	if( !p )
		errlogSevPrintf( errlogFatal, "%s: Memory allocation failed.\n", __func__ );
	return p;
}
#define zalloc(size) ({ void *__p = _zalloc(size); if(!__p) exit(S_dev_noMemory); __p; })





static epicsUInt16 endian_uint16( epicsUInt16 val )
{
	epicsUInt16 retv = 0;
	char *p = (char *)(&retv);

	*p = val & 0xff;
	*(p+1) = (val & 0xff00) >> 8;

	return retv;
}

static epicsUInt32 endian_uint32( epicsUInt32 val )
{
	epicsUInt32 retv = 0;
	char *p = (char *)(&retv);

	*p 		= val & 0x000000ff;
	*(p+1) 	= (val & 0x0000ff00) >> 8;
	*(p+2) 	= (val & 0x00ff0000) >> 16;
	*(p+3) 	= (val & 0xff000000) >> 24;

	return retv;
}


int drvGetValue( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, int bitspec )
{
	static long get_speclen_counter = 0;

	epicsMutexMustLock( e->rw_lock );
	switch( bitlen )
	{
		case 1:
				*val = (*(e->r_data + offs) >> bit) & 0x01;
				break;
		case 8:
				if( bitspec >= 0 )
					*val = (*(e->r_data + offs) >> bitspec) & 0x01;
				else
					*val = *(e->r_data + offs);
				break;
		case 16:
				if( bitspec >= 0 )
					*val = (endian_uint16( *(epicsUInt16 *)(e->r_data + offs) ) >> bitspec) & 0x0001;
				else
					*val = endian_uint16( *(epicsUInt16 *)(e->r_data + offs) );
				break;
		case 32:
				if( bitspec >= 0 )
					*val = (endian_uint32( *(epicsUInt32 *)(e->r_data + offs) ) >> bitspec) & 0x00000001;
				else
					*val = endian_uint32( *(epicsUInt32 *)(e->r_data + offs) );
				break;
		default:
				get_speclen_counter++;
				break;
	}
	epicsMutexUnlock( e->rw_lock );

    return 0;
}

int drvGetValueMasked( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, epicsInt16 nobt, epicsUInt16 shift, epicsUInt32 mask )
{

	*val = endian_uint32(*(epicsUInt32 *)(e->r_data + offs)) & (epicsUInt32)((((1 << nobt) - 1) & mask) << shift);

    return 0;
}



int drvSetValue( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, int bitspec )
{
	int retv = 0, i;
	char mask;

	epicsMutexMustLock( e->rw_lock );
	switch( bitlen )
	{
		case 1:
				if( *val & 0x01 )
					*(e->w_data + offs) |= (0x01 << bit);
				else
					*(e->w_data + offs) &= ~(0x01 << bit);
				break;
		case 8:
				if( bitspec >= 0 )
				{
					*(e->w_data + offs) &= ~(1 << bitspec);
					*(e->w_data + offs) |= ((*val & 0x01) << bitspec);
				}
				else
					*(e->w_data + offs) = *val;
				break;
		case 16:
				if( bitspec >= 0 )
				{
					*(epicsUInt16 *)(e->w_data + offs) &= ~(1 << bitspec);
					*(epicsUInt16 *)(e->w_data + offs) |= ((endian_uint16(*val) & 0x0001) << bitspec);
				}
				else
					*(epicsUInt16 *)(e->w_data + offs) 	= endian_uint16(*val);
				break;
		case 32:
				if( bitspec >= 0 )
				{
					*(epicsUInt32 *)(e->w_data + offs) &= ~(1 << bitspec);
					*(epicsUInt32 *)(e->w_data + offs) |= ((endian_uint32(*val) & 0x00000001) << bitspec);
				}
				else
					*(epicsUInt32 *)(e->w_data + offs) 	= endian_uint32(*val);
				break;
		default:
				if( bitlen < 1 || bitlen > 8 )
				{
					errlogSevPrintf( errlogFatal, "%s: bitlen %d not allowed\n", __func__, bitlen );
					retv = 1; // error, unaligned field
					break;
				}
				mask = 0;
				for( i = bit; i < (bit+bitlen); i++ )
					mask |= (0x01 << i);
				*(e->w_data + offs) &= ~mask;
				*(e->w_data + offs) |= (*val & 0x000000ff);
				break;
	}
	epicsMutexUnlock( e->rw_lock );

    return retv;
}

int drvSetValueMasked( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, epicsInt16 nobt, epicsUInt16 shift, epicsUInt32 mask )
{

	epicsMutexMustLock( e->rw_lock );

#if 0
	printf( "\n%s: before: *val=0x%08x, ec*val= 0x%08x, wdata=0x%08x ecwdata=0x%08x (offs.bit:%d.%d, bitlen %d), mask=0x%08x, nobt=%d, shift=%d, mask=%d\n", __func__,
			*val, endian_uint32( *val ), *(epicsUInt32 *)(e->w_data + offs), endian_uint32(*(epicsUInt32 *)(e->w_data + offs)), offs, bit, bitlen, (epicsUInt32)((((1 << nobt) - 1) << shift) & mask), nobt, shift, mask );
#endif

	*(epicsUInt32 *)(e->w_data + offs) &= endian_uint32(~(epicsUInt32)((((1 << nobt) - 1) & mask) << shift) );
	*(epicsUInt32 *)(e->w_data + offs) |= endian_uint32( *val  & (epicsUInt32)((((1 << nobt) - 1) & mask) << shift) );

#if 0
	printf( "\n%s:  after: *val=0x%08x, ec*val= 0x%08x, wdata=0x%08x ecwdata=0x%08x (offs.bit:%d.%d, bitlen %d), mask=0x%08x, nobt=%d, shift=%d, mask=%d\n", __func__,
			*val, endian_uint32( *val ), *(epicsUInt32 *)(e->w_data + offs), endian_uint32(*(epicsUInt32 *)(e->w_data + offs)), offs, bit, bitlen, (epicsUInt32)((((1 << nobt) - 1) << shift) & mask), nobt, shift, mask );
#endif
	epicsMutexUnlock( e->rw_lock );

	return 0;
}




static void create_masks( ecnode *d, char *rmask, char *wmask )
{
	int i, j, nregs = d->ddata.num_of_regs,
			offs, bit, len;
	char *mask;
	ec_direction_t dir;

	memset( rmask, 0, d->ddata.dsize );
	memset( wmask, 0, d->ddata.dsize );

	for( i = 0; i < nregs; i++ )
	{
		dir = d->ddata.reginfos[i].sync->sync_t.dir;
		offs = d->ddata.reginfos[i].byte;
		bit = d->ddata.reginfos[i].bit;
		len = d->ddata.reginfos[i].bit_length;
		mask = (dir ? wmask : rmask);

		if( len >= 8 )
			memset( mask+offs, 0xff, len/8 );
		if( len % 8 )
			for( j = bit; j < bit + (len % 8); j++ )
				*(mask + offs + len/8) |= (1 << j);
	}

}

inline void process_read_values( ecnode *d, char *rmask )
{

	memcpy( d->ddata.rmem, d->ddata.dmem, d->ddata.dsize );
}

inline void process_write_values( ecnode *d, char *wmask )
{
	register int i;

	for( i = 0; i < d->ddata.dsize; i++ )
	{
		d->ddata.dmem[i] &= ~wmask[i];
		d->ddata.dmem[i] |= d->ddata.wmem[i];
	}

}

/*-------------------------------------------------------------------*/
/*                                                                   */
/* IRQ thread                                                        */
/*                                                                   */
/*-------------------------------------------------------------------*/
void irq_thread( void *rdata )
{
	ethcat *ec = (ethcat *)rdata;

    while(1)
    {
        epicsEventMustWait( ec->irq );

        scanIoRequest( ec->r_scan );
		scanIoRequest( ec->w_scan );

    }

}

#define IOCTL_MSG_DREC _IOWR(PSI_ECAT_VERSION_MAGIC, 100, int)

#if 1
int ec_domain_received( ecnode *d )
{
    int fd, retv;
	ioctl_trans io;

	io.dnr = d->nr;

	if( (fd = open("/dev/psi_ethercat", O_RDWR)) < 0)
	{
		errlogSevPrintf( errlogFatal, "%s: Cannot open device /dev/psi_ethercat\n", __func__ );
		return FAIL;
	}

	if( (retv = ioctl( fd, IOCTL_MSG_DREC, &io)) < 0 )
	{
		close(fd);
		errlogSevPrintf( errlogFatal, "%s: Register does not exist, or cannot access device /dev/psi_ethercat\n", __func__ );
		return FAIL;
	}

	close(fd);

	return retv;
}
#endif



/*-------------------------------------------------------------------*/
/*                                                                   */
/* Worker thread                                                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
void worker_thread( void *rdata )
{
	int wt_counter = 0, received, delayctr, delayed = 0, recd = 0, forwarded = 0;
	struct timespec rec = { .tv_sec = 0, .tv_nsec = 50000 }, ct;
	ethcat *ec = (ethcat *)rdata;
	ec_master_t *ecm;
	ec_domain_t *ecd;
	char *rmask, *wmask;

	if( !ec->m )
	{
		errlogSevPrintf( errlogFatal, "%s: master not valid\n", __func__ );
		return;
	}
	if( !ec->d )
	{
		errlogSevPrintf( errlogFatal, "%s: domain not valid\n", __func__ );
		return;
	}
	if( !ec->r_data || !ec->w_data )
	{
		errlogSevPrintf( errlogFatal, "%s: Internal error - r_data/w_data not initialized\n", __func__ );
		return;
	}
	if( tmr_init( ec->d->nr, ec->rate ) != OK )
	{
		errlogSevPrintf( errlogFatal, "%s: CPU timer %d failure\n", __func__, ec->d->nr );
		return;
	}

	ecm = ec->m->mdata.master;
	ecd = ec->d->domain_t;
    rmask = calloc( 1, ec->d->ddata.dsize );
    wmask = calloc( 1, ec->d->ddata.dsize );
	if( !rmask || !wmask )
	{
		errlogSevPrintf( errlogFatal, "%s: allocating memory for domain rw masks failed\n", __func__ );
		return;
	}

    create_masks( ec->d, rmask, wmask );

	//----------------------
	while( 1 )
	{

		ecrt_master_receive( ecm );
		ecrt_domain_process( ecd );

		epicsMutexMustLock( ec->rw_lock );
    	process_read_values( ec->d, rmask );
		process_write_values( ec->d, wmask );
		epicsMutexUnlock( ec->rw_lock );

		epicsEventSignal( ec->irq );

		ecrt_domain_queue( ecd );
		ecrt_master_send( ecm );

		forwarded += tmr_wait( 0 );

		delayctr = 0;
		while( 1 )
		{
            ecrt_master_receive( ecm );
    		if( ec_domain_received( ec->d ) )
    		{
    			recd++;
            	break;
    		}
			delayed++;
            delayctr++;

			if( delayctr > 10 )
				break;
			clock_nanosleep( CLOCK_MONOTONIC, 0, &rec, NULL );
		}

		wt_counter++;

#if 0
		if( !(wt_counter % 5000) )
			printf( "wt_counter=%d, delayed=%d, recd=%d, forwarded=%d\n",
							wt_counter, delayed, recd, forwarded );
#endif
	}

	printf( "wt_counter=%d, delayed=%d, recd=%d\n", wt_counter, delayed, received );

	// cleanup not really necessary, but...
	free( wmask );
	free( rmask );

}



//------------------------------------------------------------
//
// Hooks
//
//------------------------------------------------------------

void process_hooks( initHookState state )
{
	ethcat **ec;
	static int workthreadsrunning = 0;

	switch( state )
	{
		case initHookAfterDatabaseRunning:

				if( workthreadsrunning )
					return;
				workthreadsrunning = 1;

				// start domain worker threads
				for( ec = &ecatList; *ec; ec = &(*ec)->next )
				{
					(*ec)->dthread = epicsThreadMustCreate( ECAT_DNAME, 60, // epicsThreadPriorityLow,
										epicsThreadGetStackSize(epicsThreadStackSmall), &worker_thread, *ec );
					(*ec)->irqthread = epicsThreadMustCreate( ECAT_DNAME, epicsThreadPriorityLow,
										epicsThreadGetStackSize(epicsThreadStackSmall), &irq_thread, *ec );
				}
				break;

		default:
					break;
	}


}




//--------------------------------------------------------------------
ethcat *ethercatOpen( int domain_nr )
{
	ethcat *e;
	for( e = ecatList; e; e = e->next )
	{
		if( e->dnr == domain_nr )
			return e;
	}
	return NULL;
}


void drvethercatAtExit( void *arg )
{
	ethcat *e = (ethcat *)arg;

	pinfo( "%s: Deactivating master, domain %d\n", __func__, e->dnr );
	ecrt_master_deactivate( ecroot->child->mdata.master );

	pinfo( "%s: Releasing master\n", __func__ );
	ecrt_release_master( ecroot->child->mdata.master );

}


int drvGetRegisterDesc( ethcat *e, domain_register *dreg, int regnr, ecnode **pentry, int b_nr )
{
	ecnode *d = e->d;

	if( regnr < 0 || regnr > d->ddata.num_of_regs )
	{
		errlogSevPrintf( errlogFatal, "%s: Domain register %d is out of range for this domain\n", __func__, regnr );
		return FAIL;
	}

	dreg->offs = d->ddata.reginfos[regnr].byte;
	dreg->bit = d->ddata.reginfos[regnr].bit;
	dreg->bitlen = d->ddata.reginfos[regnr].bit_length;
	dreg->bitspec = b_nr;

	*pentry = d->ddata.reginfos[regnr].pdo_entry;
	if( !*pentry )
	{
		errlogSevPrintf( errlogFatal, "%s: Domain register %d appears to have no pdo entry associated\n", __func__, regnr );
		return FAIL;
	}

	return OK;
}


int drvGetEntryDesc( ethcat *e, domain_register *dreg, int *dreg_nr, ecnode **pentry, int s_nr, int sm_nr, int p_nr, int e_nr, int b_nr )
{
	ecnode *d = e->d, *pe;
	int i;

	pe = ecn_get_pdo_entry_nr( 0, s_nr, sm_nr, p_nr, e_nr );
	if( !e )
	{
		errlogSevPrintf( errlogFatal, "%s: PDO entry s%d.sm%d.p%d.e%d not found\n", __func__, s_nr, sm_nr, p_nr, e_nr );
		return FAIL;
	}

	if( !pe->domain_entry )
	{
		errlogSevPrintf( errlogFatal, "%s: PDO entry s%d.sm%d.p%d.e%d not registered in any domain\n",
													__func__, s_nr, sm_nr, p_nr, e_nr );
		return FAIL;
	}

	if( pe->domain_entry->de_domain->nr != d->nr )
	{
		errlogSevPrintf( errlogFatal, "%s: PDO entry s%d.sm%d.p%d.e%d registered in other domain (%d)\n",
													__func__, s_nr, sm_nr, p_nr, e_nr, pe->domain_entry->de_domain->nr );
		return FAIL;
	}

	*pentry = pe;
	for( i = 0; i < d->ddata.num_of_regs; i++ )
		if( d->ddata.reginfos[i].pdo_entry == pe )
		{
			*dreg_nr = i;
			dreg->offs = d->ddata.reginfos[i].byte;
			dreg->bit = d->ddata.reginfos[i].bit;
			dreg->bitlen = d->ddata.reginfos[i].bit_length;
			dreg->bitspec = b_nr;
			return OK;
		}

	errlogSevPrintf( errlogFatal, "%s: Internal error - PDO entry s%d.sm%d.p%d.e%d registered in domain %d, but not in reg list\n",
												__func__, s_nr, sm_nr, p_nr, e_nr, pe->domain_entry->de_domain->nr );

	return FAIL;
}


int drvDomainExists( int mnr, int dnr )
{
	ecnode *m, *d;


	m = ecn_get_child_nr_type( ecroot, mnr, ECNT_MASTER );
	if( !m )
		return FAIL;

	d = ecn_get_child_nr_type( m, dnr, ECNT_DOMAIN );
	if( !d )
		return FAIL;

	return OK;
}
#define EC_IOCTL_DOM_RECEIVED         EC_IOWR(0xa0, ec_ioctl_voe_t)


/*-------------------------------------------------------------------*/
/*                                                                   */
/* Startup configuration                                             */
/*                                                                   */
/*-------------------------------------------------------------------*/

long drvethercatConfigure(
    int domain_nr,
    double freq,
    int autoconfig,
    int autostart
)

{
    ethcat **ec;
    ecnode *m;


    // check arguments
    if( domain_nr < 0 ||
    	freq < 0.001 || freq > 13000.0 ||
    	autoconfig < 0 || autoconfig > 1 ||
    	autostart < 0 || autostart > 1
   	)
    {
        printf( "----------------------------------------------------------------------------------\n" );
        printf( "Usage: drvethercatConfigure( domain_nr )\n\n");
        printf( " Argument        Desc\n");
        printf( " domain_nr       Number of the corresponding EtherCAT domain\n");
        printf( " frequency       EtherCAT frequency in Hz\n");
        printf( " autoconfig      Should driver autoconfigure the domain?\n");
        printf( " autostart       Number of the corresponding EtherCAT domain\n");
        printf( " \nExample:\n");
        printf( " drvethercatConfigure 0, 200, 1, 1\n");
        printf( "----------------------------------------------------------------------------------\n" );

        printf( "\n***** ecat2 driver called with drvethercatConfigure %d, %f, %d, %d\n",
        		domain_nr, freq, autoconfig, autostart );
        printf( "\n***** ecat2 EPICS driver not loaded!\n\n");
        return 0;
    }


    if( !ecroot )
    {

		//---------------------------------
		// create root and master
		ecroot = ecn_add_child_type( NULL, ECNT_ROOT );
		if( !ecroot )
		{
			errlogSevPrintf( errlogFatal, "%s: creating root node failed\n", __func__ );
			return S_dev_noMemory;
		}

		ecroot->nr = 0;
    }

    if( !ecroot->child )
    {
		// currently fixed to a single master
		m = ecn_add_child_type( ecroot, ECNT_MASTER );
		if( !m )
		{
			errlogSevPrintf( errlogFatal, "%s: creating master node failed\n", __func__ );
			return S_dev_noMemory;
		}

		m->nr = 0;

		// allocate a master
	    pinfo( "Requesting master..." );
	    if( !(m->mdata.master = ecrt_request_master( 0 )) )
	        perrret( "\nRequesting master 0 failed. EtherCAT Master not running or not responding.\n" );
	    pinfo( "succeeded\n" );
    }
    else
    	m = ecroot->child;



    // query master about the current config
    if( !master_create_physical_config( m ) )
    {
		errlogSevPrintf( errlogFatal, "%s: creating master config failed\n", __func__ );
		return S_dev_badRequest;
    }

	//---------------------------------
    // create and init domain

    // is this domain already registered?
    for( ec = &ecatList; *ec; ec = &(*ec)->next )
    	if( (*ec)->dnr == domain_nr )
    	{
    		errlogSevPrintf( errlogFatal, "%s: EtherCAT domain %d has already been registered\n", __func__, domain_nr );
    		return S_dev_identifyOverlap;
    	}


    *ec = malloc( sizeof(ethcat) );
	if( *ec == NULL )
	{
		errlogSevPrintf( errlogFatal, "%s: Memory allocation failed.\n", __func__ );
		return S_dev_noMemory;
	}

    (*ec)->next = NULL;
    (*ec)->dnr = domain_nr;
	(*ec)->rw_lock = epicsMutexMustCreate();
	(*ec)->rate = (long)((double)1000000000.0/(double)freq);
	(*ec)->m = m;
	(*ec)->irq = epicsEventMustCreate( epicsEventEmpty );


	//----------------------------------
	if( !((*ec)->d = add_domain( m, (*ec)->rate )) )
	{
		errlogSevPrintf( errlogFatal, "%s: Domain init and autoconfig failed.\n", __func__ );
		return S_dev_noMemory;
	}
	(*ec)->d->nr = domain_nr;
	(*ec)->r_data = (*ec)->d->ddata.rmem;
	(*ec)->w_data = (*ec)->d->ddata.wmem;


	//----------------------------------
	// init irq scanning
    scanIoInit( &((*ec)->r_scan) );
    scanIoInit( &((*ec)->w_scan) );

    // register atexit callback
    //epicsAtExit( drvethercatAtExit, *ec );

	//----------------------------------
	printf( PPREFIX "\n" );
	printf( PPREFIX "Domain %d\n", domain_nr );
	printf( PPREFIX "Domain frequency %.1f Hz (1 network packet sent every %f ms)\n", freq, (double)((*ec)->rate)/(double)1000000 );
	printf( PPREFIX "Domain registers %d\n", (*ec)->d->ddata.num_of_regs );
	printf( PPREFIX "Domain size %d bytes (allocated %d bytes)\n", (*ec)->d->ddata.dsize, (*ec)->d->ddata.dallocated );
	printf( PPREFIX "Slave-to-slave entries %d\n", (*ec)->d->ddata.num_of_sts_entries );



	printf( PPREFIX "Domain %d configure process completed\n", domain_nr );
    return 0;
}












/*-------------------------------------------------------------------*/
/*                                                                   */
/* Boilerplate                                                       */
/*                                                                   */
/*-------------------------------------------------------------------*/

static const iocshArg drvethercatConfigureArg0 = { "domainnr", 		iocshArgInt };
static const iocshArg drvethercatConfigureArg1 = { "freq", 			iocshArgDouble };
static const iocshArg drvethercatConfigureArg2 = { "autoconfig",	iocshArgInt };
static const iocshArg drvethercatConfigureArg3 = { "autostart", 	iocshArgInt };
static const iocshArg * const drvethercatConfigureArgs[] = {
    &drvethercatConfigureArg0,
    &drvethercatConfigureArg1,
    &drvethercatConfigureArg2,
    &drvethercatConfigureArg3
};

static const iocshFuncDef drvethercatConfigureDef =
    { "drvethercatConfigure", 4, drvethercatConfigureArgs };

static void drvethercatConfigureFunc( const iocshArgBuf *args )
{
    drvethercatConfigure(
        args[0].ival,
        args[1].dval,
        args[2].ival,
        args[3].ival
    );
}


static void drvethercat2_registrar()
{
    iocshRegister( &drvethercatConfigureDef, drvethercatConfigureFunc );
}

epicsExportRegistrar( drvethercat2_registrar );





/* EPICS drvSupport structure */
struct {
    long number;
    long (*report) ();
    long (*init) ();
} drvecat2 = {
    1,
    NULL, /*drvethercat_report,*/
    NULL
};






