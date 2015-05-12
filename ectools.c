
#include "ec.h"


#define pe_name(node) 	(d->ddata.reginfos[i].pdo_entry->pdo_entry_t->name ? get_pdo_entry_t(node)->name : "<no name>")


//-------------------------------------------------------------------
//
// DMap
//
//-------------------------------------------------------------------


static void ect_print_d_entry_value( ethcat *e, int dnr, domain_reg_info *dreginfo, int showpe )
{
	int byte, bit, len;
	epicsUInt32 val;
	char sbuf[128];

	byte = dreginfo->byte;
	bit = dreginfo->bit;
	len = dreginfo->bit_length;
	printf( "[%s] ", dreginfo->sync->sync_t.dir == 1 ? "OUT" : "IN " );
	printf( "d%d.r%d = ",
						dnr,
						dreginfo->domain_entry->nr
				);


	memset( sbuf, ' ', 64 );
	sbuf[12] = 0;

	drvGetValue( e, byte, bit, &val, len, -1, 0 );

	switch( len )
	{
		case 1:  sprintf( sbuf, "%d", val & (1 << bit) ? 1 : 0 ); break;
		case 8:  sprintf( sbuf, "0x%02x", (unsigned char)val ); break;
		case 16: sprintf( sbuf, "0x%04x", (unsigned short)val ); break;
		case 32: sprintf( sbuf, "0x%08x", (unsigned int)val ); break;
	}

	sbuf[strlen(sbuf)] = ' ';
	printf( "%s ", sbuf );

	if( !showpe )
		return;

	printf( " ---> " );


	printf( "s%d.sm%d.p%d.e%d (%d bit%s) 0x%04x:0x%02x",
						dreginfo->slave->nr,
						dreginfo->sync->nr,
						dreginfo->pdo->nr,
						dreginfo->pdo_entry->nr,
						len, //d->ddata.reginfos[i].pdo_entry->pdo_entry_t.bit_length,
						dreginfo->pdo_entry->pdo_entry_t.bit_length == 1 ? "" : "s",
						dreginfo->pdo_entry->pdo_entry_t.index,
						dreginfo->pdo_entry->pdo_entry_t.subindex

			);
}

static int ect_print_d_entry_values( int dnr )
{
	int i;
	ecnode *d = ecn_get_domain_nr( 0, dnr );
	ethcat *e = drvFindDomain( dnr );

	if( !d || !e )
	{
		errlogSevPrintf( errlogFatal, "%s: domain %d does not exist\n", __func__, dnr );
		return S_dev_badRequest;
	}

	for( i = 0; i < d->ddata.num_of_regs; i++ )
	{
		ect_print_d_entry_value( e, dnr, &d->ddata.reginfos[i], 1 );
		printf( "\n" );
	}

	printf( "\n" );

	return 1;
}

static int ect_print_d_entry_values_compact( int dnr )
{
	int i, j = 0;
	ecnode *d = ecn_get_domain_nr( 0, dnr );
	ethcat *e = drvFindDomain( dnr );

	if( !d || !e )
	{
		errlogSevPrintf( errlogFatal, "%s: domain %d does not exist\n", __func__, dnr );
		return S_dev_badRequest;
	}

	for( i = 0; i < d->ddata.num_of_regs; i++ )
	{

		ect_print_d_entry_value( e, dnr, &d->ddata.reginfos[i], 0 );

		if( !(++j % 4) )
			printf( "\n" );
		else
			printf( "  " );
	}

	printf( "\n" );

	return 1;
}

long dmap( int dnr )
{
    if( dnr < 0 )
    {
        printf( "----------------------------------------------------------------------------------\n" );
        printf( "Usage: dmap [domain_nr]\n\n");
        printf( " Argument        Desc\n");
        printf( " domain_nr       Number of the corresponding EtherCAT domain\n");
        printf( " \nExample:\n");
        printf( " dmap 0\n");
        printf( "----------------------------------------------------------------------------------\n" );

        printf( "\n***** ecat2 driver called with dmap %d\n", dnr );
        return 0;
    }


    return ect_print_d_entry_values( dnr );
}





//-------------------------------------------------------------------
//
// Stat
//
//-------------------------------------------------------------------

int ect_print_stats( int level, int dnr )
{
	int i;
	ecnode *d = ecn_get_domain_nr( 0, dnr );
	ethcat *e = drvFindDomain( dnr );
	char *packet;

	if( !d || !e )
	{
		errlogSevPrintf( errlogFatal, "%s: domain %d does not exist\n", __func__, dnr );
		return ERR_BAD_REQUEST;
	}
	packet = calloc( 1, d->ddata.dsize );
	if( !packet )
	{
		errlogSevPrintf( errlogFatal, "%s: out of memory\n", __func__ );
		return ERR_OUT_OF_MEMORY;
	}

	printf( "------------------------------\n" );
	printf( "PSI EtherCAT driver statistics\n" );
	printf( "------------------------------\n" );

	printf( "Worker thread: " );
	printf( "thread cycles %d, delayed %d, received %d, timer cycles %d\n", wt_counter[dnr], delayed[dnr], recd[dnr], forwarded[dnr] );

	epicsMutexMustLock( e->rw_lock );
	memcpy( packet, d->ddata.dmem, d->ddata.dsize );
	epicsMutexUnlock( e->rw_lock );

	printf( "                           ============\n" );
	printf( "                            Domain %d:\n", dnr );
	printf( "                           ============\n" );
		printf( "                         Running?: %s \n", d->ddata.is_running ? "Yes" : "No" );
	printf( "                             Rate: 1 packet every %.2f ms", (double)d->ddata.rate/(double)1000000.0 );
	if( d->ddata.rate > 0 )
		printf( " (%.1f packets/second)\n", (double)1000000000.0/(double)d->ddata.rate );
	else
		printf( " (N/A)\n" );
	printf( "                 No. of registers: %d \n", d->ddata.num_of_regs );
	printf( "                  Size (in bytes): %d \n", d->ddata.dsize );
	printf( "      Memory allocated (in bytes): %d \n", d->ddata.dallocated );
	printf( "    No. of slave-to-slave entries: %d \n", d->ddata.num_of_sts_entries );

	printf( "\nDomain %d content (%d bytes):", dnr, d->ddata.dsize );

	for( i = 0; i < d->ddata.dsize; i++ )
	{

		if( !(i % 16) )
			printf( "\n0x%04x: ", i );
		printf( "%02x ", *(packet+i) );
		if( i && !((i+1) % 8))
			printf( " " );
		if( i && ((i+1) % 8) && !((i+1) % 16) )
			printf( "\n" );
	}
	printf( "\n" );

	return 0;
}


long stat( int level, int dnr )
{
    if( level < 0 || dnr < 0 )
    {
        printf( "----------------------------------------------------------------------------------\n" );
        printf( "Usage: stat [level]\n\n");
        printf( " Argument        Desc\n");
        printf( " level           Level of info\n");
        printf( " domain_nr       Number of the corresponding EtherCAT domain\n");
        printf( " \nExamples:\n");
        printf( " stat\n");
        printf( " stat 0 0\n");
        printf( " stat 2 0\n");
        printf( "----------------------------------------------------------------------------------\n" );

        printf( "\n***** ecat2 driver called with stat %d %d\n", level, dnr );
        return 0;
    }


    return ect_print_stats( level, dnr );
}




