#include "ec.h"

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



//------------------------------------------------------------
//
// Hooks
//
//------------------------------------------------------------

void process_hooks( initHookState state )
{
	ethcat **ec;
	static int workthreadsrunning = 0;
	FN_CALLED;

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
										epicsThreadGetStackSize(epicsThreadStackSmall), &ec_worker_thread, *ec );
					(*ec)->irqthread = epicsThreadMustCreate( ECAT_DNAME, epicsThreadPriorityLow,
										epicsThreadGetStackSize(epicsThreadStackSmall), &ec_irq_thread, *ec );
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
	FN_CALLED;

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

	FN_CALLED;

	pinfo( "%s: Deactivating master, domain %d\n", __func__, e->dnr );
	ecrt_master_deactivate( ecroot->child->mdata.master );

	pinfo( "%s: Releasing master\n", __func__ );
	ecrt_release_master( ecroot->child->mdata.master );

}

ethcat *drvFindDomain( int dnr )
{
	ethcat **ec;
	for( ec = &ecatList; *ec; ec = &(*ec)->next )
		if( (*ec)->dnr == dnr )
			return *ec;

	return NULL;
}

int drvGetRegisterDesc( ethcat *e, domain_register *dreg, int regnr, ecnode **pentry, int b_nr )
{
	ecnode *d;

	if( !e )
	{
		errlogSevPrintf( errlogFatal, "%s: ethcat/e NULL - init failed\n", __func__ );
		return FAIL;
	}

	if( !e->d )
	{
		errlogSevPrintf( errlogFatal, "%s: ethcat domain NULL - domain init failed\n", __func__ );
		return FAIL;
	}

	d = e->d;

	FN_CALLED;
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
	ecnode *d, *pe;
	int i;
	FN_CALLED;

	if( !e )
	{
		errlogSevPrintf( errlogFatal, "%s: ethcat/e NULL - init failed\n", __func__ );
		return FAIL;
	}

	if( !e->d )
	{
		errlogSevPrintf( errlogFatal, "%s: ethcat domain NULL - domain init failed\n", __func__ );
		return FAIL;
	}

	d = e->d;

	pe = ecn_get_pdo_entry_nr( 0, s_nr, sm_nr, p_nr, e_nr );
	if( !pe )
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

	FN_CALLED;

	m = ecn_get_child_nr_type( ecroot, mnr, ECNT_MASTER );
	if( !m )
		return FAIL;

	d = ecn_get_child_nr_type( m, dnr, ECNT_DOMAIN );
	if( !d )
		return FAIL;

	return OK;
}

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

	FN_CALLED;

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

    configure_el6692_entries( m->mdata.master );



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
	(*ec)->d->ddata.sts_lock = epicsMutexMustCreate();
	(*ec)->r_data = (*ec)->d->ddata.rmem;
	(*ec)->w_data = (*ec)->d->ddata.wmem;

    (*ec)->w_mask = calloc( 1, (*ec)->d->ddata.dsize );
	if( !(*ec)->w_mask )
	{
		errlogSevPrintf( errlogFatal, "%s: allocating memory for domain wmask failed\n", __func__ );
		return ERR_OUT_OF_MEMORY;
	}

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

//----------------------
//
// Configure
//
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
    { "ecatConfigure", 4, drvethercatConfigureArgs };

static void drvethercatConfigureFunc( const iocshArgBuf *args )
{
    drvethercatConfigure(
        args[0].ival,
        args[1].dval,
        args[2].ival,
        args[3].ival
    );
}

//----------------------
//
// Domain map
//
//----------------------
static const iocshArg drvethercatDMapArg0 = { "cmd", 		iocshArgString };
static const iocshArg * const drvethercatDMapArgs[] = {
    &drvethercatDMapArg0,
};

static const iocshFuncDef drvethercatDMapDef =
    { "dmap", 1, drvethercatDMapArgs };

static void drvethercatDMapFunc( const iocshArgBuf *args )
{
    dmap(
        args[0].sval
    );
}



//----------------------
//
// stat
//
//----------------------
static const iocshArg drvethercatstatArg0 = { "level", 		iocshArgInt };
static const iocshArg drvethercatstatArg1 = { "dnr", 		iocshArgInt };
static const iocshArg * const drvethercatstatArgs[] = {
    &drvethercatstatArg0,
    &drvethercatstatArg1,
};

static const iocshFuncDef drvethercatstatDef =
    { "stat", 2, drvethercatstatArgs };

static void drvethercatStatFunc( const iocshArgBuf *args )
{
    stat(
        args[0].ival,
        args[1].ival
    );
}


//----------------------
//
// ConfigEL6692
//
//----------------------
static const iocshArg drvethercatConfigEL6692Arg[] = {
		{ "slave_pos",  	iocshArgInt },
		{ "io",  			iocshArgString },
		{ "bitlen", 		iocshArgInt }
};
static const iocshArg * const drvethercatConfigEL6692Args[] = {
    &drvethercatConfigEL6692Arg[0],
    &drvethercatConfigEL6692Arg[1],
    &drvethercatConfigEL6692Arg[2],
};
#if 0
static const iocshFuncDef drvethercatConfigEL6692Def =
    { "ecatConfigEL6692", 3, drvethercatConfigEL6692Args };

static void drvethercatConfigEL6692Func( const iocshArgBuf *args )
{
    ConfigEL6692(
        args[0].ival,
        args[1].sval,
        args[2].ival
    );
}

#endif


//----------------------
//
// ConfigEL6692
//
//----------------------
static const iocshArg drvethercatStSArg[] = {
		{ "from",  	iocshArgString },
		{ "to",  	iocshArgString },
};
static const iocshArg *const drvethercatStSArgs[] = {
    &drvethercatStSArg[0],
    &drvethercatStSArg[1],
};

static const iocshFuncDef drvethercatStSDef =
    { "ecatsts", 2, drvethercatStSArgs };

static void drvethercatStSFunc( const iocshArgBuf *args )
{
    sts(
        args[0].sval,
        args[1].sval
    );
}







//=====================================================
//
// EPICS support structures
//
//=====================================================

static void drvethercat2_registrar()
{
    iocshRegister( &drvethercatConfigureDef, drvethercatConfigureFunc );
    iocshRegister( &drvethercatDMapDef, drvethercatDMapFunc );
    iocshRegister( &drvethercatstatDef, drvethercatStatFunc );
// test    iocshRegister( &drvethercatConfigEL6692Def, drvethercatConfigEL6692Func );
    iocshRegister( &drvethercatStSDef, drvethercatStSFunc );
}

epicsExportRegistrar( drvethercat2_registrar );





struct {
    long number;
    long (*report) ();
    long (*init) ();
} drvecat2 = {
    1,
    NULL, /*drvethercat_report,*/
    NULL
};






