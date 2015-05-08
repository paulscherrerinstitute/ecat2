
#include "ec.h"


#define pe_name(node) 	(d->ddata.reginfos[i].pdo_entry->pdo_entry_t->name ? get_pdo_entry_t(node)->name : "<no name>")


static int ect_print_d_entry_values( int dnr )
{
	int i, byte, bit, len;
	epicsUInt32 val;
	char sbuf[128];
	ecnode *d = ecn_get_domain_nr( 0, dnr );
	ethcat *e = drvFindDomain( dnr );

	if( !d || !e )
	{
		errlogSevPrintf( errlogFatal, "%s: domain %d does not exist\n", __func__, dnr );
		return S_dev_badRequest;
	}

	for( i = 0; i < d->ddata.num_of_regs; i++ )
	{
		byte = d->ddata.reginfos[i].byte;
		bit = d->ddata.reginfos[i].bit;
		len = d->ddata.reginfos[i].bit_length;
		printf( "d%d.r%d = ",
							d->nr,
							d->ddata.reginfos[i].domain_entry->nr
					);
		memset( sbuf, ' ', 64 );
		sbuf[12] = 0;

		drvGetValue( e, byte, bit, &val, len, -1 );

		switch( len )
		{
			case 1:  sprintf( sbuf, "%d", val & (1 << bit) ? 1 : 0 ); break;
			case 8:  sprintf( sbuf, "0x%02x", (unsigned char)val ); break;
			case 16: sprintf( sbuf, "0x%04x", (unsigned short)val ); break;
			case 32: sprintf( sbuf, "0x%08x", (unsigned int)val ); break;
		}

		sbuf[strlen(sbuf)] = ' ';
		printf( "%s    ---> ", sbuf );

		printf( "s%d.sm%d.p%d.e%d (%d bit%s) 0x%04x:0x%02x",
							d->ddata.reginfos[i].slave->nr,
							d->ddata.reginfos[i].sync->nr,
							d->ddata.reginfos[i].pdo->nr,
							d->ddata.reginfos[i].pdo_entry->nr,
							len, //d->ddata.reginfos[i].pdo_entry->pdo_entry_t.bit_length,
							d->ddata.reginfos[i].pdo_entry->pdo_entry_t.bit_length == 1 ? "" : "s",
							d->ddata.reginfos[i].pdo_entry->pdo_entry_t.index,
							d->ddata.reginfos[i].pdo_entry->pdo_entry_t.subindex

				);
		printf( "\n" );
	}

	printf( "\n" );

	return 1;
}


//-------------------------------------------------------------------
//
// DMap
//
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






