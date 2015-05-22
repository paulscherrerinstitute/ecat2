
#include "ec.h"


#define pe_name(node) 	(d->ddata.reginfos[i].pdo_entry->pdo_entry_t->name ? get_pdo_entry_t(node)->name : "<no name>")
void print_pdo_entry( ecnode *pe, int bitspec );


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
	printf( "%4d.%d  ", dreginfo->byte, dreginfo->bit );


	sprintf( sbuf, "%d bit%s ",
			len,
			dreginfo->pdo_entry->pdo_entry_t.bit_length == 1 ? " " : "s"
			);
	printf( "%10s", sbuf );

	sprintf( sbuf, "r%d = ",
						//dnr,
						dreginfo->domain_entry->nr
				);
	printf( "%8s", sbuf );


	drvGetValue( e, byte, bit, &val, len, -1, 0 );

	switch( len )
	{
		case 1:  sprintf( sbuf, "%d", val & (1 << bit) ? 1 : 0 ); break;
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

	printf( " ---> " );

	printf( "s%d.sm%d.p%d.e%d ",
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
static void ect_print_val( ethcat *e, RECTYPE rectype, int byte, int bit, int bitlen, int bitspec, int nobt, int shift, int mask )
{
	epicsUInt32 val;
	char sbuf[128] = { 0 };

	if( rectype == REC_MBBI || rectype == REC_MBBO )
	{
	    drvGetValueMasked( e, byte, bit, &val, bitlen, nobt, shift, mask );
	    val >>= shift;
	}
	else
		drvGetValue( e, byte, bit, &val, bitlen, bitspec, 0 );

	if( bitspec > -1 )
		bitlen = 1;



	switch( bitlen )
	{
		case 1:  sprintf( sbuf, "%d", val & (1 << bit) ? 1 : 0 ); break;
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
	int morethan1 = 0;

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
			case REC_LONGIN:
			case REC_MBBI:
							printf( " ---> " );
							break;
			case REC_AO:
			case REC_BO:
			case REC_MBBO:
			case REC_LONGOUT:
			default:
							printf( " <--- " );
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
				default:
								break;
			}


		printf( "%s %s",
				rectypes[(*cr)->ix].recname,
				(*cr)->rec->name
				);

#if PRINT_RECVAL
		ect_print_val( e, (*cr)->rectype, (*cr)->dreg_info->offs, (*cr)->dreg_info->bit, (*cr)->dreg_info->bitlen,
						(*cr)->dreg_info->bitspec, (*cr)->dreg_info->nobt, (*cr)->dreg_info->shft, (*cr)->dreg_info->mask );
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
				printf( ".b%d ", (*se)->from.bitspec );
			printf( " ---> " );
			print_pdo_entry( (*se)->pdo_entry_to, (*se)->to.bitspec );
			if( cnt > 1 )
				printf( "\n" );
		}
		else if( (*se)->pdo_entry_to == dreginfo->pdo_entry )
		{
			if( cnt > 1 )
				printf( space );
			if( (*se)->to.bitspec > -1 )
				printf( ".b%d ", (*se)->to.bitspec );
			printf( " <--- " );
			print_pdo_entry( (*se)->pdo_entry_from, (*se)->from.bitspec );
			if( cnt > 1 )
				printf( "\n" );
		}


	}

   	return cnt > 1;
}


int ect_print_d_entry_values_sts( int dnr )
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
		if( !ect_print_d_entry_value_sts( e, &d->ddata.reginfos[i] ) )
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
    { 3, EC_DIR_INPUT, 2, el6692_pdos + 1, EC_WD_DISABLE }, // --> TxPDO
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


static el6692 *add_new_6692( slave_pos )
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



    for( i = 0; i < strlen(io); i++ )
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
    	newp = *entries + *no_entries;
    	pei = realloc( (*entries), ++(*no_entries) );

    }
	if( !pei )
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
    printf( " ecatEL6692 in 8\n");
    printf( " ecatEL6692 in 1\n");
    printf( " ecatEL6692 out 32\n");
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


static int parse_str( char *s, ethcat **e, ecnode **pe, int *dreg_nr, domain_register *dreg )
{
	char *text, *tail, *tokens[EPT_MAX_TOKENS] = { NULL }, *delims = " ._\t";
	int i, ntokens, retv = NOTOK, num, s_num, sm_num, p_num, e_num, b_num, d_num, r_num;

	FN_CALLED;

	if( !(text = calloc(sizeof(char), strlen(s) )) )
    {
        errlogSevPrintf( errlogFatal, "%s: out of memory\n", __func__ );
		return ERR_OUT_OF_MEMORY;
    }
	strcpy( text, s );


	if( !(ntokens = dev_tokenize( text, delims, tokens )) )
	{
		errlogSevPrintf( errlogFatal, "%s: input string '%s' invalid\n", __func__, s );
		return ERR_BAD_ARGUMENT;
	}

	// Possible INP/OUT links:
	//
	// Daa.Rbb            		: domain register nn
	// Saa.SMbb.Pcc.Edd[.Bee]   : pdo entry a.b.c.d
	//
	s_num = sm_num = p_num = e_num = b_num = d_num = r_num = -1;
	for( i = 0; i < ntokens; i++ )
	{
		if( !tokens[i] )
			continue;

		if( isdigit(tokens[i][1]) )
			num = (int)strtol( tokens[i]+1, &tail, 10 );
		else if( strlen(tokens[i]) > 3 && tokens[i][1] == '(' && tokens[i][strlen(tokens[i])-1] == ')')
			num = dev_parse_expression( tokens[i] );
		else
			num = -1;

		switch( toupper(tokens[i][0]) )
		{
			case 'D':	d_num = num; break;
			case 'R':	r_num = num; break;
			case 'S':	if( toupper(tokens[i][1]) == 'M' )
							sm_num = (int)strtol( tokens[i]+2, &tail, 10 );
						else
							s_num = num; break;
						break;
			case 'P':	p_num = num; break;
			case 'E':	e_num = num; break;
			case 'B':	b_num = num; break;

		}
	}

	if( d_num < 0 )
		d_num = 0;

    *e = ethercatOpen( d_num );
    if( *e == NULL )
    {
        errlogSevPrintf( errlogFatal, "%s: cannot open domain %d\n", __func__, d_num );
        return ERR_BAD_ARGUMENT;
    }

	// check to see what kind of link was discovered
    if( r_num >= 0 )
    {
    	if( drvGetRegisterDesc( *e, dreg, *dreg_nr = r_num, pe, b_num) != OK )
    	{
    		errlogSevPrintf( errlogFatal, "%s: input string error (%s), domain %d register number %d not found\n",
    												__func__, s, d_num, r_num );
    		return ERR_BAD_ARGUMENT;
    	}

    	retv = OK;
    }
    else if( s_num >=0 && sm_num >= 0 && p_num >= 0 && e_num >=0 )
    {
    	if( drvGetEntryDesc( *e, dreg, dreg_nr, pe, s_num, sm_num, p_num, e_num, b_num ) != OK )
    	{

			errlogSevPrintf( errlogFatal, "%s: input string error (%s) in domain %d\n", __func__, s, d_num );
			return ERR_BAD_ARGUMENT;
	    }
    	retv = OK;
    }
    else
	{
		errlogSevPrintf( errlogFatal, "%s: input string error (%s), domain %d, entry link not valid or incomplete\n",
												__func__, s, d_num );
		return ERR_BAD_ARGUMENT;
	}

    return retv;
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
	int len_from, len_to, dreg_nr_from, dreg_nr_to;
	ethcat *e_from, *e_to;
	ecnode *pe_from, *pe_to;
	domain_register dreg_from, dreg_to;

    if( !from || !to )
    	goto getout;

    len_from = strlen( from );
    len_to = strlen( to );
    if( len_from < 2 || len_to < 2 )
    	goto getout;


    if( parse_str( from, &e_from, &pe_from, &dreg_nr_from, &dreg_from ) != OK )
        return ERR_BAD_ARGUMENT;

    if( parse_str( to, &e_to, &pe_to, &dreg_nr_to, &dreg_to ) != OK )
        return ERR_BAD_ARGUMENT;

	add_new_sts_entry( e_from->d, pe_from, pe_to, &dreg_from, &dreg_to );


	printf( PPREFIX "StS entry added: " );

	print_pdo_entry( pe_from, dreg_from.bitspec );
	printf( " --> " );
	print_pdo_entry( pe_to, dreg_to.bitspec );

	if( dreg_from.bitlen > 0 )
		printf( " (%d bit%s)", dreg_from.bitlen, dreg_from.bitlen == 1 ? "" : "s" );

	printf( "\n" );
   	return 0;

getout:
    printf( "----------------------------------------------------------------------------------\n" );
    printf( "Usage: ecatStS from to\n\n");
    printf( " Argument        Desc\n");
    printf( " from            pdo entry (eg s0.sm0.p0.e0[.b0]) or domain register (eg [d0.]r0)\n" );
    printf( " to              pdo entry (eg s0.sm0.p0.e0[.b0]) or domain register (eg [d0.]r0)\n" );
    printf( " \nNote: if domain number is omitted, d0 will be assumed\n");
    printf( " \nExamples:\n");
    printf( " ecatStS s3.sm2.p0.e1 s4.sm0.p1.e120\n" );
    printf( " ecatStS s3.sm2.p0.e1.b6 s4.sm0.p1.e120.b3\n" );
    printf( " ecatStS d0.r31 d0.r144\n");
    printf( " ecatStS r12 r0\n");
    printf( "----------------------------------------------------------------------------------\n" );

    return 0;
}









