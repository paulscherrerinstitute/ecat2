
#include "ec.h"


#define pe_name(node) 	(d->ddata.reginfos[i].pdo_entry->pdo_entry_t->name ? get_pdo_entry_t(node)->name : "<no name>")
void print_pdo_entry( ecnode *pe, int bitspec );

//----------------------------------------------------------------------
char *strtoupper( char *s )
{
	int ix = -1;
	if( !s )
		return NULL;
	while( s[++ix] )
		if( isalpha(s[ix]) )
			s[ix] = toupper( s[ix] );
	return s;
}

char *strtolower( char *s )
{
	int ix = -1;
	if( !s )
		return NULL;
	while( s[++ix] )
		if( isalpha(s[ix]) )
			s[ix] = tolower( s[ix] );
	return s;
}


//----------------------------------------------------------------------

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
	printf( "[%s] ", dreginfo->sync->sync_t.dir == 1 ? "O" : "I" );
	printf( "0x%04x:0x%02x ",
			dreginfo->pdo_entry->pdo_entry_t.index,
			dreginfo->pdo_entry->pdo_entry_t.subindex
			);
	printf( "%4d.%d  ", byte, bit );


	sprintf( sbuf, "%d bit%s ",
			len,
			len == 1 ? " " : "s"
			);
	printf( "%10s", sbuf );

	sprintf( sbuf, "r%d = ",
						//dnr,
						dreginfo->domain_entry->nr
				);
	printf( "%8s", sbuf );


	drvGetValue( e, byte, bit, &val, len, -1, 0, -1, -1 );

	switch( len )
	{
		case 1:  sprintf( sbuf, "%d", val & 0x00000001 ); break;
		case 8:  sprintf( sbuf, "0x%02x", (unsigned char)val ); break;
		case 16: sprintf( sbuf, "0x%04x", (unsigned short)val ); break;
		case 32: sprintf( sbuf, "0x%08x", (unsigned int)val ); break;
		default:
			if( len < 1 || len > 7 )
				break;
			sprintf( sbuf, "0x%02x", (val & ((1 << len) - 1) << bit) >> bit );
			break;
	}
	printf( "%-10s", sbuf );
	if( !showpe )
		return;

	printf( " @ " );

	printf( "s%d.sm%d.p%d.e%d",
						dreginfo->slave->nr,
						dreginfo->sync->nr,
						dreginfo->pdo->nr,
						dreginfo->pdo_entry->nr

			);

//	printf( " offset %d.%d (0x%x.%d)", dreginfo->byte, dreginfo->bit, dreginfo->byte, dreginfo->bit );
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

#define PRINT_RECVAL  1

#if PRINT_RECVAL
static void ect_print_val( ethcat *e, RECTYPE rectype, int byte, int bit, int bitlen, int bitspec,
																		int nobt, int shift, int mask, int sio,
																		int byteoffs, int bytelen )
{
	epicsUInt32 val;
	char sbuf[128] = { 0 }, sval[40+1], oval[40+1];

	if( rectype == REC_MBBI || rectype == REC_MBBO )
	{
	    drvGetValueMasked( e, byte, bit, &val, bitlen, nobt, shift, mask, byteoffs, bytelen );
	    val >>= shift;
	}
	else if( rectype == REC_STRINGIN || rectype == REC_STRINGOUT )
	{
	    drvGetValueString( e, byte, bitlen, sval, oval, byteoffs, bytelen );
	    sprintf( sbuf, "offset +%d, ", byteoffs );
	    printf( " = '%s' (%slength %d)", sval, sbuf, bytelen );
	    return;
	}
	else
		drvGetValue( e, byte, bit, &val, bitlen, bitspec, 0, byteoffs, bytelen );

	if( bitspec > -1 )
		bitlen = 1;



	switch( bitlen )
	{
		case 1:  sprintf( sbuf, "%d", (bitspec > -1 ? val >> bitspec : val) & 0x00000001 ); break;
		case 8:  sprintf( sbuf, "0x%02x", (unsigned char)val ); break;
		case 16: sprintf( sbuf, "0x%04x", (unsigned short)val ); break;
		case 32: sprintf( sbuf, "0x%08x", (unsigned int)val ); break;
		default:
			if( bitlen < 1 || bitlen > 7 )
				break;
			sprintf( sbuf, "0x%02x", (val & ((1 << bitlen) - 1) << bit) >> bit );
			break;
	}
	printf( " = %s", sbuf );
}
#endif

static void ect_print_d_entry_value_rec( ethcat *e, domain_reg_info *dreginfo )
{
	conn_rec **cr = &dreginfo->pdo_entry->cr;
	int morethan1 = 0, sio = 0;

	if( *cr )
		if( (*cr)->next )
		{
			morethan1 = 1;
			printf( "\n" );
		}

	for( ; *cr; cr = &(*cr)->next )
	{
		if( morethan1 )
			printf( "                                                           " );


		switch( (*cr)->rectype )
		{
			case REC_AI:
			case REC_BI:
			case REC_MBBI:
			case REC_LONGIN:
			case REC_STRINGIN:
			case REC_AAI:
							printf( " ---> " );
							break;
			case REC_AO:
			case REC_BO:
			case REC_MBBO:
			case REC_LONGOUT:
			case REC_STRINGOUT:
			case REC_AAO:
							printf( " <--- " );
							break;
			default:
							printf( " --- " );
							break;
		}
		if( (*cr)->dreg_info->bitspec > -1 )
			printf( "b%d: ", (*cr)->dreg_info->bitspec );
		else
			switch( (*cr)->rectype )
			{
				case REC_AI:
								break;

				case REC_AO:
								break;

				case REC_BI:
								break;

				case REC_BO:
								break;

				case REC_MBBI:
								if( ((mbbiRecord *)((*cr)->rec))->nobt )
									printf( "b%d..b%d: ", ((mbbiRecord *)((*cr)->rec))->shft,
													((mbbiRecord *)((*cr)->rec))->shft + ((mbbiRecord *)((*cr)->rec))->nobt - 1 );
								break;

				case REC_MBBO:
								if( ((mbboRecord *)((*cr)->rec))->nobt )
									printf( "b%d..b%d: ", ((mbboRecord *)((*cr)->rec))->shft,
													((mbboRecord *)((*cr)->rec))->shft + ((mbboRecord *)((*cr)->rec))->nobt - 1 );
								break;

				case REC_LONGIN:
								break;

				case REC_LONGOUT:
								break;

				case REC_STRINGIN:
								break;

				case REC_STRINGOUT:
								break;
				default:
								break;
			}


		printf( "%s %s",
				rectypes[(*cr)->ix].recname,
				(*cr)->rec->name
				);

#if PRINT_RECVAL
		if( (*cr)->rectype == REC_STRINGIN || (*cr)->rectype == REC_STRINGOUT )
			sio = 1;
		ect_print_val( e, (*cr)->rectype, (*cr)->dreg_info->offs, (*cr)->dreg_info->bit, (*cr)->dreg_info->bitlen,
							(*cr)->dreg_info->bitspec, (*cr)->dreg_info->nobt, (*cr)->dreg_info->shft, (*cr)->dreg_info->mask,
							sio,
							(*cr)->dreg_info->byteoffs, (*cr)->dreg_info->bytelen );
#endif

		if( (*cr)->next )
			printf( "\n" );
	}
}



int ect_print_d_entry_values_rec( int dnr )
{
	int i;
	ecnode *d = ecn_get_domain_nr( 0, dnr );
	ethcat *e = drvFindDomain( dnr );

	if( !d || !e )
	{
		errlogSevPrintf( errlogFatal, "%s: domain %d does not exist\n", __func__, dnr );
		return ERR_BAD_REQUEST;
	}

	for( i = 0; i < d->ddata.num_of_regs; i++ )
	{
		ect_print_d_entry_value( e, dnr, &d->ddata.reginfos[i], 1 );
		ect_print_d_entry_value_rec( e, &d->ddata.reginfos[i] );
		printf( "\n" );
	}

	printf( "\n" );

	return OK;
}


static void ect_print_val_sts( ethcat *e, ecnode *d, domain_register *ft )
{
	epicsUInt32 val;
	char sbuf[128] = { 0 };

	int bitlen = ft->bitlen;

	drvGetValue( e, ft->offs, ft->bit, &val, ft->bitlen, ft->bitspec, 0, ft->byteoffs, ft->bytelen );

	if( ft->bitspec > -1 )
		bitlen = 1;

	switch( bitlen )
	{
		case 1:  sprintf( sbuf, "%d", val & 0x00000001 ); break;
		case 8:  sprintf( sbuf, "0x%02x", (unsigned char)val ); break;
		case 16: sprintf( sbuf, "0x%04x", (unsigned short)val ); break;
		case 32: sprintf( sbuf, "0x%08x", (unsigned int)val ); break;
		default:
			sprintf( sbuf, "!0x%02x", (val & ((1 << ft->bitlen) - 1) << ft->bit) >> ft->bit );
			break;
	}
	printf( " = %s", sbuf );
}


static int ect_print_d_entry_value_sts( ethcat *e, domain_reg_info *dreginfo )
{
	sts_entry **se = &e->d->ddata.sts;
	int cnt = 0;
	static const char *space = "                                                        ";
	if( !*se )
		return 0;

	for( ; *se; se = &((*se)->next) )
		if( (*se)->pdo_entry_from == dreginfo->pdo_entry || (*se)->pdo_entry_to == dreginfo->pdo_entry )
			cnt++;
	if( cnt > 1 )
		printf( "\n" );


   	for( se = &e->d->ddata.sts; *se; se = &((*se)->next) )
	{
		if( (*se)->pdo_entry_from == dreginfo->pdo_entry )
		{
			if( cnt > 1 )
				printf( space );
			if( (*se)->from.bitspec > -1 )
				printf( ".b%d", (*se)->from.bitspec );

			ect_print_val_sts( e, e->d, &(*se)->from );
			printf( " ---> " );
			print_pdo_entry( (*se)->pdo_entry_to, (*se)->to.bitspec );

			ect_print_val_sts( e, e->d, &(*se)->to );

			if( cnt > 1 )
				printf( "\n" );
		}
		else if( (*se)->pdo_entry_to == dreginfo->pdo_entry )
		{
			if( cnt > 1 )
				printf( space );
			if( (*se)->to.bitspec > -1 )
				printf( ".b%d", (*se)->to.bitspec );

			ect_print_val_sts( e, e->d, &(*se)->to );
			printf( " <--- " );
			print_pdo_entry( (*se)->pdo_entry_from, (*se)->from.bitspec );


			ect_print_val_sts( e, e->d, &(*se)->from );

			if( cnt > 1 )
				printf( "\n" );
		}


	}

   	return cnt > 1;
}



int ect_print_d_entry_values_sts( int dnr )
{
	int i, retv;
	ecnode *d = ecn_get_domain_nr( 0, dnr );
	ethcat *e = drvFindDomain( dnr );

	if( !d || !e )
	{
		errlogSevPrintf( errlogFatal, "%s: domain %d does not exist\n", __func__, dnr );
		return ERR_BAD_REQUEST;
	}

	for( i = 0; i < d->ddata.num_of_regs; i++ )
	{
		ect_print_d_entry_value( e, dnr, &d->ddata.reginfos[i], 1 );
		retv = ect_print_d_entry_value_sts( e, &d->ddata.reginfos[i] );
		if( !retv )
			printf( "\n" );
	}

	printf( "\n" );

	return OK;
}


long dmap( char *cmd )
{
    if( cmd )
    {
    	if( strlen(cmd) )
    	{

    		if( !strcmp( cmd, "rec" ) )
    			ect_print_d_entry_values_rec( 0 );
    		if( !strcmp( cmd, "sts" ) )
    			ect_print_d_entry_values_sts( 0 );
    		return 0;
    	}
    }


    return ect_print_d_entry_values( 0 );
}


//-------------------------------------------------------------------
//
// Stat
//
//-------------------------------------------------------------------

int ect_print_stats( int dnr )
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


long stat( int dnr )
{
    if( dnr < 0 )
    {
        printf( "----------------------------------------------------------------------------------\n" );
        printf( "Usage: stat [domain_nr]\n\n");
        printf( " Argument        Desc\n");
        printf( " domain_nr       Number of the corresponding EtherCAT domain\n");
        printf( " \nExamples:\n");
        printf( " stat\n");
        printf( " stat 0\n");
        printf( "----------------------------------------------------------------------------------\n" );

        printf( "\n***** ecat2 driver called with stat %d\n", dnr );
        return 0;
    }


    return ect_print_stats( dnr );
}







//-------------------------------------------------------------------
//
// ConfigEL6692
//
//-------------------------------------------------------------------
typedef struct _el6692 {
	struct _el6692 *next;

	int slave_pos;

	ec_pdo_entry_info_t *el6692_rx_entries;
	int no_el6692_rx_entries;
	ec_pdo_entry_info_t *el6692_tx_entries;
	int no_el6692_tx_entries;
} el6692;

el6692 *el6692s = NULL;

// currently not included by default,
// since they have to be defined on both sides of EL6692
ec_pdo_entry_info_t el6692_pdo_default_entries[] = {
    {0x10f4, 0x01, 2}, /* Sync Mode */
    {0x1800, 0x09, 1}, /* TxPDO-Toggle */
    {0x1800, 0x07, 1}, /* TxPDO-State */
    {0x10f4, 0x0e, 1}, /* Control value update toggle */
    {0x10f4, 0x0f, 1}, /* Time stamp update toggle */
    {0x10f4, 0x10, 1}, /* External device not connected */
};


ec_pdo_info_t el6692_pdos[] = {
    { 0x1600, 0, NULL }, // RxPDO-Map
    { 0x1a00, 0, NULL }, // TxPDO-Map
    { 0x1a01, 6, el6692_pdo_default_entries + 0 }, /* TxPDO-Map External Sync Compact */
};

ec_sync_info_t el6692_syncs[] = {
    { 0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE },
    { 1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE },
    { 2, EC_DIR_OUTPUT, 1, el6692_pdos + 0, EC_WD_DISABLE }, // --> RxPDO
    { 3, EC_DIR_INPUT, 1, el6692_pdos + 1, EC_WD_DISABLE }, // --> TxPDO
    { 0xff }
};

#define IN_ENTRY 1
#define OUT_ENTRY 2
#define Beckhoff_EL6692 0x00000002, 0x1a243052

void configure_el6692_entries(  ec_master_t *master )
{
	el6692 **mod;
	int i;
	ec_pdo_entry_info_t *info;
	ec_slave_config_t *sc;


	printf( PPREFIX "EL6692 entry list:\n" );
	for( mod = &el6692s; *mod; mod = &((*mod)->next) )
	{
		printf( "   EL6692 @ position %d:  OUT/Rx %d entries, IN/Tx %d entries\n",
				(*mod)->slave_pos,
				(*mod)->no_el6692_rx_entries,
				(*mod)->no_el6692_tx_entries
				);
		for( i = 0, info = (*mod)->el6692_rx_entries; i < (*mod)->no_el6692_rx_entries; i++ )
		{
			printf( "      OUT (Rx) entry %d: 0x%04x:%02x, %d bit%s\n",
					i,
					info[i].index,
					info[i].subindex,
					info[i].bit_length,
					info[i].bit_length == 1 ? "" : "s"
					);
		}

		for( i = 0, info = (*mod)->el6692_tx_entries; i < (*mod)->no_el6692_tx_entries; i++ )
		{
			printf( "       IN (Tx) entry %d: 0x%04x:%02x, %d bit%s\n",
					i,
					info[i].index,
					info[i].subindex,
					info[i].bit_length,
					info[i].bit_length == 1 ? "" : "s"
					);
		}

		printf( PPREFIX "Configuring EL6692 @ %d...", (*mod)->slave_pos );

		el6692_pdos[0].entries = (*mod)->el6692_rx_entries;
		el6692_pdos[0].n_entries = (*mod)->no_el6692_rx_entries;

		el6692_pdos[1].entries = (*mod)->el6692_tx_entries;
		el6692_pdos[1].n_entries = (*mod)->no_el6692_tx_entries;

		if (!(sc = ecrt_master_slave_config( master, 0, (*mod)->slave_pos, Beckhoff_EL6692 )))
		{
		    printf( "%s: Failed to get EL6692 slave configuration at position %d.\n", __func__, (*mod)->slave_pos );
		    return;
		}

		if( ecrt_slave_config_pdos(sc, EC_END, el6692_syncs) )
		{
		    printf( "%s: Failed to configure PDOs for EL6692 slave at position %d.\n", __func__, (*mod)->slave_pos );
		    return;
		}

		printf( "...success.\n" );

	}


}

int slave_is_6692( int slave_pos )
{
	el6692 **mod;

    for( mod = &el6692s; *mod; mod = &((*mod)->next) )
    	if( (*mod)->slave_pos == slave_pos )
    		return slave_pos;

    return -1;
}

el6692 *get_6692_at_pos( int slave_pos )
{
	el6692 **mod;

    for( mod = &el6692s; *mod; mod = &((*mod)->next) )
    	if( (*mod)->slave_pos == slave_pos )
    		return *mod;

    return NULL;
}


int get_no_pdos_6692( int sm_num )
{
	// sm 0 and 1
	if( sm_num < 2 )
		return 0;
	// sm 2
	if( sm_num == 2 )
		return 1;

	return 1; // sm3

}

int get_pdo_info_6692( ec_master_t *ecm, int i, int j, int k, ec_pdo_info_t *pdo_t )
{
	el6692 *mod = get_6692_at_pos( i );
	if( !mod )
	{
		printf( "Cannot find EL6692 module at position %d!\n", i );
		return 0;
	}

	el6692_pdos[0].entries = mod->el6692_rx_entries;
	el6692_pdos[0].n_entries = mod->no_el6692_rx_entries;

	el6692_pdos[1].entries = mod->el6692_tx_entries;
	el6692_pdos[1].n_entries = mod->no_el6692_tx_entries;

	switch( j )
	{
		case 0:
		case 1:
				memset( pdo_t, 0, sizeof(ec_pdo_info_t) );
				break;
		case 2: if( el6692_pdos[0].entries )
					memcpy( pdo_t, &el6692_pdos[0], sizeof(ec_pdo_info_t) );
				break;

		case 3:	if( el6692_pdos[1+k].entries )
					memcpy( pdo_t, &el6692_pdos[1+k], sizeof(ec_pdo_info_t) );
				break;

		default:
				printf( "%s: SM %d > 3 !\n", __func__, i );
				return 0;
	}

	return 1;
}


int get_pdo_entry_info_6692( ec_master_t *ecm, int i, int j, int k, int l, ec_pdo_entry_info_t *pdo_entry_t )
{
	el6692 *mod = get_6692_at_pos( i );
	if( !mod )
	{
		printf( "Cannot find EL6692 module at position %d!\n", i );
		return 0;
	}

	el6692_pdos[0].entries = mod->el6692_rx_entries;
	el6692_pdos[0].n_entries = mod->no_el6692_rx_entries;

	el6692_pdos[1].entries = mod->el6692_tx_entries;
	el6692_pdos[1].n_entries = mod->no_el6692_tx_entries;

	switch( j )
	{
		case 0:
		case 1:
				break;
		case 2: if( el6692_pdos[0].entries )
					memcpy( pdo_entry_t, &el6692_pdos[0].entries[l], sizeof(ec_pdo_entry_info_t) );
				break;

		case 3:if( el6692_pdos[1+k].entries )
					memcpy( pdo_entry_t, &el6692_pdos[1+k].entries[l], sizeof(ec_pdo_entry_info_t) );
				break;

		default:
				printf( "%s: SM %d > 3 !\n", __func__, i );
				return 0;
	}

	return 1;

}


static el6692 *add_new_6692( int slave_pos )
{
	el6692 **mod;

    // is this EL6692 already registered?
    for( mod = &el6692s; *mod; mod = &((*mod)->next) )
    	if( (*mod)->slave_pos == slave_pos )
    		return (*mod);

    if( !(*mod = calloc( 1, sizeof(el6692)) ) )
    		return NULL;
	(*mod)->slave_pos = slave_pos;

    return *mod;
}

long ConfigEL6692( int slave_pos, char *io, int bitlen )
{
	int i, len, iodir = 0;
	ec_pdo_entry_info_t *pei, *newp, **entries;
	int index, subindex, *no_entries;
	el6692 *module;

    if( !io || bitlen <= 0 || slave_pos < 0 )
    	goto getout;

    len = strlen(io);
    if( len < 2 || len > 3 )
    	goto getout;

    if( bitlen > 8 && bitlen % 8 )
	{
		errlogSevPrintf( errlogFatal, "%s: Bit length of %d is not allowed for a EL6692 module entry\n", __func__, bitlen );
		return ERR_BAD_ARGUMENT;
	}



    for( i = 0; i < len; i++ )
    	io[i] = tolower(io[i]);

    module = add_new_6692( slave_pos );
    if( !module )
	{
		errlogSevPrintf( errlogFatal, "%s: allocating memory for a EL6692 module failed\n", __func__ );
		return ERR_OUT_OF_MEMORY;
	}


	if( !strcmp(io, "in") )
    {
    	no_entries = &module->no_el6692_tx_entries;
    	entries = &module->el6692_tx_entries;
    	iodir = IN_ENTRY;
    	index = 0x7000;
    }
    else if( !strcmp(io, "out") )
    {
    	no_entries = &module->no_el6692_rx_entries;
    	entries = &module->el6692_rx_entries;
    	iodir = OUT_ENTRY;
    	index = 0x6000;
    }
    else
    	goto getout;

	subindex = *no_entries;

    if( !*no_entries )
    {
		newp = pei = calloc( 1, sizeof(ec_pdo_entry_info_t) );
		(*no_entries)++;
    }
    else
    {
    	pei = realloc( (*entries), (++(*no_entries))*sizeof(ec_pdo_entry_info_t) );
    	newp = pei + (*no_entries) - 1;

    }

	if( !pei || !newp )
	{
		errlogSevPrintf( errlogFatal, "%s: allocating memory failed\n", __func__ );
		return ERR_OUT_OF_MEMORY;
	}

	*entries = pei;

	newp->index = index;
   	newp->subindex = subindex;
   	newp->bit_length = bitlen;


   	printf( "%s: config EL6692 as slave %d, prepared entry %d, direction %s: 0x%04x:%02x %d bit%s\n", __func__,
				slave_pos,
				(*no_entries) - 1,
				iodir == IN_ENTRY ? "IN" : "OUT",
				index,
				subindex,
				bitlen,
				bitlen == 1 ? "" : "s"
   			);

   	return 0;

getout:
    printf( "----------------------------------------------------------------------------------\n" );
    printf( "Usage: ecatConfigEL6692 io bitlen\n\n");
    printf( " Argument        Desc\n");
    printf( " slave_pos       slave number, from 0\n" );
    printf( " io              in or out\n");
    printf( " bitlen          Bitlen of the entry to create (> 0)\n");
    printf( " \nExamples:\n");
    printf( " ecatcfgEL6692 1 in 8\n");
    printf( " ecatcfgEL6692 1 in 1\n");
    printf( " ecatcfgEL6692 1 out 32\n");
    printf( "----------------------------------------------------------------------------------\n" );

    printf( "\n***** ecat2 driver called with stat %d %s %d\n", slave_pos, io, bitlen );
    return 0;
}







//-------------------------------------------------------------------
//
// StS
//
//-------------------------------------------------------------------






static sts_entry *add_new_sts_entry( ecnode *d, ecnode *pe_from, ecnode *pe_to, domain_register *from, domain_register *to )
{
	sts_entry **se;


    // is this entry already registered?
    for( se = &d->ddata.sts; *se; se = &((*se)->next) )
    	if( ((*se)->from.offs == from->offs && (*se)->from.bit == from->bit &&
    			(*se)->from.bitlen == from->bitlen && (*se)->from.bitspec == from->bitspec) ||
			 ((*se)->to.offs == to->offs && (*se)->to.bit == to->bit &&
							(*se)->to.bitlen == to->bitlen && (*se)->to.bitspec == to->bitspec)
    			)
    		return NULL;

    epicsMutexMustLock( d->ddata.sts_lock );
    if( !(*se = calloc( 1, sizeof(sts_entry)) ) )
    		return NULL;

    (*se)->pdo_entry_from = pe_from;
    (*se)->from.offs 	= from->offs;
    (*se)->from.bit 	= from->bit;
    (*se)->from.bitlen 	= from->bitlen;
    (*se)->from.bitspec = from->bitspec;

    (*se)->pdo_entry_to = pe_to;
    (*se)->to.offs 		= to->offs;
    (*se)->to.bit 		= to->bit;
    (*se)->to.bitlen 	= to->bitlen;
    (*se)->to.bitspec 	= to->bitspec;

    d->ddata.num_of_sts_entries++;
    epicsMutexUnlock( d->ddata.sts_lock );

    return *se;
}



void print_pdo_entry( ecnode *pe, int bitspec )
{
	if( !pe )
	{
		errlogSevPrintf( errlogFatal, "%s: pe NULL\n", __func__ );
		return;
	}
	if( !pe->parent )
	{
		errlogSevPrintf( errlogFatal, "%s: pe->parent NULL\n", __func__ );
		return;
	}

   	printf( "s%d.sm%d.p%d.e%d",
				pe->parent->parent->parent->nr,
				pe->parent->parent->nr,
				pe->parent->nr,
				pe->nr
   			);
	if( bitspec > -1 )
		printf( ".b%d", bitspec );


}

long sts( char *from, char *to )
{
	int len_from, len_to, dreg_nr_from, dreg_nr_to, bitlen;
	ethcat *e_from, *e_to;
	ecnode *pe_from, *pe_to;
	domain_register dreg_from, dreg_to;

    if( !from || !to )
    	goto getout;

    len_from = strlen( from );
    len_to = strlen( to );
    if( len_from < 2 || len_to < 2 )
    	goto getout;


    if( parse_str( from, &e_from, &pe_from, &dreg_nr_from, &dreg_from, NULL ) != OK )
        return ERR_BAD_ARGUMENT;

    if( parse_str( to, &e_to, &pe_to, &dreg_nr_to, &dreg_to, NULL ) != OK )
        return ERR_BAD_ARGUMENT;

	add_new_sts_entry( e_from->d, pe_from, pe_to, &dreg_from, &dreg_to );


	printf( PPREFIX "StS entry added: " );

	print_pdo_entry( pe_from, dreg_from.bitspec );
	printf( " --> " );
	print_pdo_entry( pe_to, dreg_to.bitspec );

	if( dreg_from.bitspec > -1 || dreg_to.bitspec > -1 )
		bitlen = 1;
	else
		bitlen = dreg_from.bitlen;
	if( bitlen > 0 )
		printf( " (%d bit%s)", bitlen, bitlen == 1 ? "" : "s" );

	printf( "\n" );
   	return 0;

getout:
    printf( "----------------------------------------------------------------------------------\n" );
    printf( "Usage: ecatsts from to\n\n");
    printf( " Argument        Desc\n");
    printf( " from            pdo entry (eg s0.sm0.p0.e0[.b0]) or domain register (eg [d0.]r0)\n" );
    printf( " to              pdo entry (eg s0.sm0.p0.e0[.b0]) or domain register (eg [d0.]r0)\n" );
    printf( " \nNote: if domain number is omitted, d0 will be assumed\n");
    printf( " \nExamples:\n");
    printf( " ecatsts s3.sm2.p0.e1 s4.sm0.p1.e120\n" );
    printf( " ecatsts s3.sm2.p0.e1.b6 s4.sm0.p1.e120.b3\n" );
    printf( " ecatsts d0.r31 d0.r144\n");
    printf( " ecatsts r12 r0\n");
    printf( "----------------------------------------------------------------------------------\n" );

    return 0;
}











//-------------------------------------------------------------------
//
// cfg slave
//
//-------------------------------------------------------------------


#define MAX_PDOS			32
#define MAX_SYNC_MANAGERS	16
typedef enum {
	CSC_SM = 0,
	CSC_SM_CLEAR_PDOS,
	CSC_SM_ADD_PDO,
	CSC_PDO_CLEAR_ENTRIES,
	CSC_PDO_ADD_ENTRY,

	CSC_ERROR = 0xffff
} cfgslave_cmd;

static char *csc_cmd_str[] = {
	"sm",
	"sm_clear_pdos",
	"sm_add_pdo",
	"pdo_clear_entries",
	"pdo_add_entry",

	NULL
};


typedef struct {
	cfgslave_cmd cmd;
	int args[6];
} cfgslave_prgstep;

static int cfg_prg_no_of_steps = 0;
static cfgslave_prgstep *cfg_prg = NULL;
#define EXTRA_DUPLICATE_CHECK 0
#define CFG2


int slave_has_static_config( int slave_nr )
{
	int i;
	for( i = 0; i < cfg_prg_no_of_steps; i++ )
		if( cfg_prg[i].args[0] == slave_nr )
			return 1;
	return 0;
}




#ifdef CFG2
ecnode *ecn_get_child_pdo_t_index_type( ecnode *parent, int pdoindex, ECN_TYPE type )
{
	ecnode *n;
	if( !parent )
		return NULL;
	n = parent->child;
	if( !n )
		return NULL;

	do {
		if( n->type == type )
			if( n->pdo_t.index == pdoindex )
				return n;
	}
	while( (n = n->next) );

	return NULL;
}

ecnode *ecn_get_child_ix_subix_type( ecnode *parent, int index, int subindex, ECN_TYPE type )
{
	ecnode *n;
	if( !parent )
		return NULL;
	n = parent->child;
	if( !n )
		return NULL;

	do {
		if( n->type == type )
			if( n->pdo_entry_t.index == index &&
					n->pdo_entry_t.subindex == subindex )
				return n;
	}
	while( (n = n->next) );

	return NULL;
}

static ecnode *cfg_add_slave( ecnode *m, int slave_nr, ec_slave_info_t *slave_t )
{
	ecnode *slave;
	if( (slave = ecn_get_child_nr_type( m, slave_nr, ECNT_SLAVE )) )
		return slave;

	if( !(slave = ecn_add_child_type( m, ECNT_SLAVE )) )
        perrret( "%s: adding slave %d failed\n", __func__, slave_nr );
	slave->nr = slave_nr;

	memcpy( &slave->slave_t, slave_t, sizeof(ec_slave_info_t) );
	memcpy( &slave->sdata.config_slave_info, &slave_t, sizeof(ec_slave_info_t) );

	slave->sdata.check = 1;

	return slave;
}

static ecnode *cfg_add_sm( ecnode *slave, int sm_nr, int dir, int wd_mode )
{
	ecnode *sm;
	if( (sm = ecn_get_child_nr_type( slave, sm_nr, ECNT_SYNC )) )
		return sm;

	if( !(sm = ecn_add_child_type( slave, ECNT_SYNC )) )
        perrret( "%s: adding slave %d, sync %d failed\n", __func__, slave->nr, sm_nr );
	sm->nr = sm_nr;

	sm->sync_t.index = sm_nr;
	sm->sync_t.dir = dir;
	sm->sync_t.watchdog_mode = wd_mode;
	sm->sync_t.n_pdos = 0;

	slave->slave_t.sync_count++;
	return sm;
}

static ecnode *cfg_add_pdo( ecnode *sm, int pdoindex )
{
	ecnode *pdo = NULL;

	if( (pdo = ecn_get_child_pdo_t_index_type( sm, pdoindex, ECNT_PDO )) )
		return pdo;

	if( !(pdo = ecn_add_child_type( sm, ECNT_PDO )) )
        perrret( "%s: adding slave %d, sync %d, pdo index 0x%04x failed\n", __func__,
        					sm->parent->nr, sm->nr, pdoindex );

	pdo->nr = sm->sync_t.n_pdos++;
	pdo->pdo_t.index = pdoindex;
	pdo->pdo_t.n_entries = 0;

	return pdo;
}

static ecnode *cfg_add_pdo_entry( ecnode *pdo, int index, int subindex, int bitlen )
{
	ecnode *pdo_entry = NULL;

	if( (pdo_entry = ecn_get_child_ix_subix_type( pdo, index, subindex, ECNT_PDO_ENTRY )) )
		return pdo_entry;

	if( !(pdo_entry = ecn_add_child_type( pdo, ECNT_PDO_ENTRY )) )
        perrret( "%s: adding slave %d, sync %d, pdo index 0x%04x, entry 0x%04x:%02x failed\n", __func__,
        				pdo->parent->parent->nr, pdo->parent->nr, pdo->pdo_t.index, index, subindex );

	pdo_entry->nr = pdo->pdo_t.n_entries++;
	pdo_entry->pdo_entry_t.index = index;
	pdo_entry->pdo_entry_t.subindex = subindex;
	pdo_entry->pdo_entry_t.bit_length = bitlen;

	return pdo_entry;
}

#endif



#define PRINT_CFG_INFO 0

EC_ERR execute_configuration_prg( void )
{
	int retv, ix = 0,
			slave_nr, sm_nr, pdo_ix_dir,
			entry_ix_wd_mode, entry_sub_ix, entry_bitlen;
	ec_slave_config_t *sc;
	ecnode *m = ecroot->child;
	//ecnode *slave, *sm, *pdo, *pdoe;
	ec_slave_info_t slave_t;
	ec_sync_info_t sync_t;
#if EXTRA_DUPLICATE_CHECK
	ec_pdo_info_t pdo_t;
	ec_pdo_entry_info_t entry;
	int i, j, found;
#endif
	cfgslave_prgstep *step = cfg_prg;
	cfgslave_cmd cscmd;
	char *cmd;
#ifdef CFG2
    ecnode *slave, *sm, *pdo;
#endif

	if( !cfg_prg )
		return 1;

	/*

	ecat2cfgslave sm                   slavenr   smnr     dir        wd_mode
	ecat2cfgslave sm_clear_pdos        slavenr   smnr
	ecat2cfgslave sm_add_pdo           slavenr   smnr     pdoindex
	ecat2cfgslave pdo_clear_entries    slavenr   smnr     pdoindex
	ecat2cfgslave pdo_add_entry        slavenr   smnr     pdoindex   entryindex    entrysubindex   entrybitlen

	*/
	printf( PPREFIX "Executing slave configuration program, %d step(s)\n", cfg_prg_no_of_steps );
	do
	{
		printf( PPREFIX "Step %d from %d: ", ix, cfg_prg_no_of_steps-1 );
		step = cfg_prg + ix;
		cscmd             = step->cmd;
		cmd = csc_cmd_str[(int)cscmd];
		slave_nr          = step->args[0];
		sm_nr             = step->args[1];
		pdo_ix_dir        = step->args[2];
		entry_ix_wd_mode  = step->args[3];
		entry_sub_ix      = step->args[4];
		entry_bitlen      = step->args[5];


        if( ecrt_master_get_slave( m->mdata.master, slave_nr, &slave_t ) )
		{
			errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s', slave nr %d not found\n", __func__, cmd, slave_nr );
			return ERR_BAD_ARGUMENT;
		}

		if (!(sc = ecrt_master_slave_config( m->mdata.master, slave_t.alias, slave_nr, slave_t.vendor_id, slave_t.product_code )))
		{
			errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s': failed to get slave configuration at position %d.\n", __func__, cmd, slave_nr );
			return ERR_OPERATION_FAILED;
		}
		if( ecrt_master_get_sync_manager( m->mdata.master, slave_nr, sm_nr, &sync_t ) )
		{
			errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s': failed to get slave %d sync manager configuration at position %d.\n", __func__, cmd, slave_nr, sm_nr );
			return ERR_OPERATION_FAILED;
		}
#ifdef CFG2

		slave = cfg_add_slave( m, slave_nr, &slave_t );

		if( !slave )
			return ERR_OPERATION_FAILED;

		sm = cfg_add_sm( slave, sm_nr, pdo_ix_dir, entry_ix_wd_mode );

		if( !sm )
			return ERR_OPERATION_FAILED;
#endif

		switch( cscmd )
		{
			case CSC_SM:
						// ecatcfg sm                   slavenr   smnr     dir        wd_mode

						if( pdo_ix_dir != EC_DIR_OUTPUT && pdo_ix_dir != EC_DIR_INPUT )
						{
							errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s': slave %d, sm %d, direction %d is not valid (valid: %d (output) or %d (input))\n",
																			__func__, cmd, slave_nr, sm_nr, pdo_ix_dir,
																			EC_DIR_OUTPUT /* 1 */, EC_DIR_INPUT /* 2 */ );
							return ERR_BAD_ARGUMENT;
						}

						if( entry_ix_wd_mode != EC_WD_DEFAULT &&
								entry_ix_wd_mode != EC_WD_ENABLE &&
								entry_ix_wd_mode != EC_WD_DISABLE )
						{
							errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s': slave %d, sm %d, wd_mode %d is not valid (valid: %d (default), %d (enable) or %d (disable))\n",
																			__func__, cmd, slave_nr, sm_nr, pdo_ix_dir,
																			EC_WD_DEFAULT /* 0 */, EC_WD_ENABLE /* 1 */, EC_WD_DISABLE /* 2 */ );
							return ERR_BAD_ARGUMENT;
						}
						if( (retv = ecrt_slave_config_sync_manager( sc, sm_nr, pdo_ix_dir, entry_ix_wd_mode )) )
						{
							errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s': slave %d, sm %d, dir %d, wd_mode %d has failed (retv %d)\n",
																			__func__, cmd, slave_nr, sm_nr, pdo_ix_dir, entry_ix_wd_mode, retv );
							return ERR_OPERATION_FAILED;
						}

						// refresh sm info
#ifdef CFG2
			        	if( ecrt_master_get_sync_manager( m->mdata.master, slave_nr, sm_nr, &sm->sync_t ) )
			            	perrret( "%s: (EL6692) cannot get slave %d, sync mgr %d info\n", __func__, slave_nr, sm_nr );
#endif
			        	sm->sync_t.dir = pdo_ix_dir;
			        	sm->sync_t.watchdog_mode = entry_ix_wd_mode;

#if PRINT_CFG_INFO
						printf( PPREFIX "cfgslave cmd '%s': slave %d, sm %d, dir %d, wd_mode %d has been executed.\n",
													cmd, slave_nr, sm_nr, pdo_ix_dir, entry_ix_wd_mode );
#endif
						break;

			case CSC_SM_CLEAR_PDOS:
						// ecatcfg sm_clear_pdos        slavenr   smnr
						ecrt_slave_config_pdo_assign_clear( sc, sm_nr );

//						ecn_delete_children( sm );
						sm->sync_t.n_pdos = 0;

#if PRINT_CFG_INFO
						printf( PPREFIX "cfgslave cmd '%s': slave %d, sm %d has been executed.\n",
													cmd, slave_nr, sm_nr );
#endif
						break;

			case CSC_SM_ADD_PDO:
						// ecatcfg sm_add_pdo           slavenr   smnr     pdoindex
#if EXTRA_DUPLICATE_CHECK
				ecrt_master_get_sync_manager( m->mdata.master, slave_nr, sm_nr, &sync_t );
						found = 0;
						for( i = 0; i < sync_t.n_pdos && !found; i++ )
						{
							ecrt_master_get_pdo( m->mdata.master, slave_nr, sm_nr, i, &pdo_t );
							if( pdo_t.index == pdo_ix_dir )
								found = 1;
						}
						if( found )
						{
							printf( "slave %d, sm %d, pdo 0x%04x already mapped\n",
									slave_nr, sm_nr, pdo_ix_dir
									);
							break;
						}
#endif
						if( (retv = ecrt_slave_config_pdo_assign_add( sc, sm_nr, pdo_ix_dir ) ))
						{
							errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s': slave %d, sm %d, pdo 0x%04x has failed (retv %d)\n",
																			__func__, cmd, slave_nr, sm_nr, pdo_ix_dir, retv );
							return ERR_OPERATION_FAILED;
						}

#ifdef CFG2
						pdo = cfg_add_pdo( sm, pdo_ix_dir );
#endif
#if PRINT_CFG_INFO
						printf( PPREFIX "cfgslave cmd '%s': slave %d, sm %d, pdo 0x%04x has been executed.\n",
													cmd, slave_nr, sm_nr, pdo_ix_dir );
#endif
						break;

			case CSC_PDO_CLEAR_ENTRIES:
						// ecatcfg pdo_clear_entries    slavenr   smnr     pdoindex

						ecrt_slave_config_pdo_mapping_clear( sc, pdo_ix_dir );

						pdo = ecn_get_child_pdo_t_index_type( sm, pdo_ix_dir, ECNT_PDO );
						pdo->pdo_t.n_entries = 0;

#if PRINT_CFG_INFO
						printf( PPREFIX "cfgslave cmd '%s': slave %d, sm %d, pdo 0x%04x has been executed.\n",
													cmd, slave_nr, sm_nr, pdo_ix_dir );
#endif
						break;

			case CSC_PDO_ADD_ENTRY:
						// ecatcfg pdo_add_entry        slavenr   smnr     pdoindex   entryindex    entrysubindex   entrybitlen
#if EXTRA_DUPLICATE_CHECK
				 		ecrt_master_get_sync_manager( m->mdata.master, slave_nr, sm_nr, &sync_t );
				 		found = 0;
						for( i = 0; i < sync_t.n_pdos && !found; i++ )
						{
							ecrt_master_get_pdo( m->mdata.master, slave_nr, sm_nr, i, &pdo_t );
							if( pdo_t.index == pdo_ix_dir )
							{
								for( j = 0; j < pdo_t.n_entries && !found; j++ )
								{
									ecrt_master_get_pdo_entry( m->mdata.master, slave_nr, sm_nr, i, j, &entry );
									if( entry.index == entry_ix_wd_mode &&
											entry.subindex == entry_sub_ix )
										found = 1;
								}
							}
						}
						if( found )
						{
							printf( "slave %d, sm %d, pdo 0x%04x, entry 0x%04x:%02x already mapped\n",
									slave_nr, sm_nr, pdo_ix_dir,
									entry_ix_wd_mode, entry_sub_ix
									);
							break;
						}
#endif
						if( (retv = ecrt_slave_config_pdo_mapping_add( sc, pdo_ix_dir, entry_ix_wd_mode, entry_sub_ix, entry_bitlen ) ))
						{
							errlogSevPrintf( errlogFatal, "%s: cfgslave cmd '%s': slave %d, sm %d, pdo 0x%04x, 0x%04x:%02x (%d bit%s) has failed (retv %d)\n",
																			__func__, cmd, slave_nr, sm_nr, pdo_ix_dir,
																			entry_ix_wd_mode, entry_sub_ix, entry_bitlen,
																			entry_bitlen == 1 ? "" : "s", retv
																			);
							return ERR_OPERATION_FAILED;
						}

#ifdef CFG2
						pdo = ecn_get_child_pdo_t_index_type( sm, pdo_ix_dir, ECNT_PDO );
						cfg_add_pdo_entry( pdo, entry_ix_wd_mode, entry_sub_ix, entry_bitlen );
#endif

#if PRINT_CFG_INFO

						printf( PPREFIX "cfgslave cmd '%s': slave %d, sm %d, pdo 0x%04x, 0x%04x:%02x (%d bit%s) has been executed.\n",
													cmd, slave_nr, sm_nr, pdo_ix_dir,
													entry_ix_wd_mode, entry_sub_ix, entry_bitlen,
													entry_bitlen == 1 ? "" : "s"
						);
#endif
						break;

			default:	// ...to satisfy gcc
						break;
		}

#ifdef CFG2_UPDATE
		cfg_update_info( slave );
#endif
	} while( ++ix < cfg_prg_no_of_steps );

	cfg_prg_no_of_steps = 0;
	free( cfg_prg );

	return ERR_NO_ERROR;
}



long cfgslave( char *cmd, int slave_nr, int sm_nr, int pdo_ix_dir, int entry_ix_wd_mode, int entry_sub_ix, int entry_bitlen )
{
	int cmd_ix = 0;
	cfgslave_cmd cscmd = CSC_ERROR;
	cfgslave_prgstep *step;

    if( !cmd )
	{
		errlogSevPrintf( errlogFatal, "%s: ecat2slave command not found\n", __func__ );
		return ERR_BAD_ARGUMENT;
	}

    if( !strlen(cmd) )
	{
		errlogSevPrintf( errlogFatal, "%s: ecat2slave command not found\n", __func__ );
		return ERR_BAD_ARGUMENT;
	}

    do
	{
		if( !strcmp( strtolower(cmd), csc_cmd_str[cmd_ix] ) )
		{
			cscmd = (cfgslave_cmd)cmd_ix;
			break;
		}
	} while( csc_cmd_str[++cmd_ix] );

	if( cscmd == CSC_ERROR )
	{
		errlogSevPrintf( errlogFatal, "%s: cfgslave command '%s' is not valid\n", __func__, cmd );
		return ERR_BAD_ARGUMENT;
	}


    if( !cfg_prg )
    {
		step = cfg_prg = calloc( 1, sizeof(cfgslave_prgstep) );
		cfg_prg_no_of_steps++;
    }
    else
    {
    	cfg_prg = realloc( cfg_prg, (++cfg_prg_no_of_steps)*sizeof(cfgslave_prgstep) );
    	step = cfg_prg + cfg_prg_no_of_steps - 1;
    }

	if( !step || !cfg_prg )
	{
		errlogSevPrintf( errlogFatal, "%s: out of memory\n", __func__ );
		return ERR_OUT_OF_MEMORY;
	}


	step->cmd = cscmd;
	step->args[0] = slave_nr;
	step->args[1] = sm_nr;
	step->args[2] = pdo_ix_dir;
	step->args[3] = entry_ix_wd_mode;
	step->args[4] = entry_sub_ix;
	step->args[5] = entry_bitlen;

	return ERR_NO_ERROR;
}






//-------------------------------------------------------------------
//
// ecat2slave / genslave
//
//-------------------------------------------------------------------

typedef enum {
	GSC_CHK_ON_BOOT = 0,
	GSC_CHK_EVERY,
	GSC_CHK_ALL_EVERY,

	GSC_ERROR = 0xffff
} genslave_cmd;

static char *gsc_cmd_str[] = {
	"check_on_boot",
	"check_every",

	NULL
};


typedef struct {
	genslave_cmd cmd;
	int args[6];
} genslave_cmdstep;

static int genslave_cmd_no_of_steps = 0;
static genslave_cmdstep *genslave_cmds = NULL;

// ecat2slave {check_on_boot_only/check_always}  slavenr  vendor_id  product_code  revision_number  serial_number


long genslave( char *cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 )
{
	int cmd_ix = 0;
	genslave_cmd gscmd = GSC_ERROR;
	genslave_cmdstep *step;

    if( !cmd )
	{
		errlogSevPrintf( errlogFatal, "%s: cfgslave command not found\n", __func__ );
		return ERR_BAD_ARGUMENT;
	}

    if( !strlen(cmd) )
	{
		errlogSevPrintf( errlogFatal, "%s: cfgslave command not found\n", __func__ );
		return ERR_BAD_ARGUMENT;
	}

    do
	{
		if( !strcmp( strtolower(cmd), gsc_cmd_str[cmd_ix] ) )
		{
			gscmd = (genslave_cmd)cmd_ix;
			break;
		}
	} while( gsc_cmd_str[++cmd_ix] );

	if( gscmd == GSC_ERROR )
	{
		errlogSevPrintf( errlogFatal, "%s: ecat2slave command '%s' is not valid\n", __func__, cmd );
		return ERR_BAD_ARGUMENT;
	}


    if( !genslave_cmds )
    {
		step = genslave_cmds = calloc( 1, sizeof(genslave_cmdstep) );
		genslave_cmd_no_of_steps++;
    }
    else
    {
    	genslave_cmds = realloc( genslave_cmds, (++genslave_cmd_no_of_steps)*sizeof(genslave_cmdstep) );
    	step = genslave_cmds + genslave_cmd_no_of_steps - 1;
    }

	if( !step || !genslave_cmds )
	{
		errlogSevPrintf( errlogFatal, "%s: out of memory\n", __func__ );
		return ERR_OUT_OF_MEMORY;
	}


	step->cmd = gscmd;
	step->args[0] = arg1;
	step->args[1] = arg2;
	step->args[2] = arg3;
	step->args[3] = arg4;
	step->args[4] = arg5;
	step->args[5] = arg6;



	return ERR_NO_ERROR;
}


EC_ERR genslave_prepare_cmds( void )
{
/*
	int ix = 0;
	genslave_cmd gscmd;
	genslave_cmdstep *step;
	char *strcmd;

	if( !genslave_cmds || !genslave_cmd_no_of_steps )
		return ERR_NO_ERROR;

	(*ec)->dthread = epicsThreadMustCreate( ECAT_TNAME_D, 60, // epicsThreadPriorityLow,
						epicsThreadGetStackSize(epicsThreadStackSmall), &ec_worker_thread, *ec );


	do
	{
		step = genslave_cmds + ix;
		gscmd = step->cmd;
		strcmd = gsc_cmd_str[(int)gscmd];

		switch( gscmd )
		{
			case GSC_CHK_ON_BOOT:
				break;
			case GSC_CHK_EVERY:
				break;
			case GSC_CHK_ALL_EVERY:
				break;

			default:
				break;

		}
	} while( ++ix < genslave_cmd_no_of_steps );
*/

	return ERR_NO_ERROR;
}




