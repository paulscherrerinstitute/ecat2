
#include "ec.h"







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


int drvGetValue( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, int bitspec, int wrval )
{
	static long get_speclen_counter = 0;
	char *rw = wrval ? e->w_data : e->r_data;

	FN_CALLED3;
	if( !e || !val )
		return ERR_OPERATION_FAILED;

	if( !e->r_data || !e->w_data )
		return ERR_OPERATION_FAILED;


	epicsMutexMustLock( e->rw_lock );
	switch( bitlen )
	{
		case 1:
				*val = (*(rw + offs) >> bit) & 0x01;
				break;
		case 8:
				if( bitspec >= 0 )
					*val = (*(rw + offs) >> bitspec) & 0x01;
				else
					*val = *(rw + offs);

				break;
		case 16:
				if( bitspec >= 0 )
					*val = (endian_uint16( *(epicsUInt16 *)(rw + offs) ) >> bitspec) & 0x0001;
				else
					*val = endian_uint16( *(epicsUInt16 *)(rw + offs) );
				break;
		case 32:
				if( bitspec >= 0 )
					*val = (endian_uint32( *(epicsUInt32 *)(rw + offs) ) >> bitspec) & 0x00000001;
				else
					*val = endian_uint32( *(epicsUInt32 *)(rw + offs) );
				break;
		default:
				get_speclen_counter++;
				break;
	}
	epicsMutexUnlock( e->rw_lock );


    return 0;
}

int drvSetValue( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, int bitspec )
{
	int retv = 0;

	FN_CALLED;

	if( !e || !val )
		return ERR_OPERATION_FAILED;

	if( !e->w_data )
		return ERR_OPERATION_FAILED;

	epicsMutexMustLock( e->rw_lock );
	switch( bitlen )
	{
		case 1:
				if( *val & 0x01 )
					*(e->w_data + offs) |= (0x01 << bit);
				else
					*(e->w_data + offs) &= ~(0x01 << bit);

				*(e->w_mask + offs) |= (0x01 << bit);
				break;
		case 8:
				if( bitspec >= 0 )
				{
					*(e->w_data + offs) &= ~(1 << bitspec);
					*(e->w_data + offs) |= ((*val & 0x01) << bitspec);
					*(e->w_mask + offs) |= (0x01 << bitspec);
				}
				else
				{
					*(e->w_data + offs) = *val;
					*(e->w_mask + offs) = 0xff;
				}

				break;
		case 16:
				if( bitspec >= 0 )
				{
					*(epicsUInt16 *)(e->w_data + offs) &= ~(1 << bitspec);
					*(epicsUInt16 *)(e->w_data + offs) |= ((endian_uint16(*val) & 0x0001) << bitspec);
					*(epicsUInt16 *)(e->w_mask + offs) |= (1 << bitspec);
				}
				else
				{
					*(epicsUInt16 *)(e->w_data + offs) 	= endian_uint16(*val);
					*(epicsUInt16 *)(e->w_mask + offs) 	= 0xffff;
				}
				break;
		case 32:
				if( bitspec >= 0 )
				{
					*(epicsUInt32 *)(e->w_data + offs) &= ~(1 << bitspec);
					*(epicsUInt32 *)(e->w_data + offs) |= ((endian_uint32(*val) & 0x00000001) << bitspec);
					*(epicsUInt32 *)(e->w_mask + offs) |= (1 << bitspec);
				}
				else
				{
					*(epicsUInt32 *)(e->w_data + offs) 	= endian_uint32(*val);
					*(epicsUInt32 *)(e->w_mask + offs) 	= 0xffffffff;
				}
				break;
		default:
				if( bitlen < 1 || bitlen > 8 )
				{
					errlogSevPrintf( errlogFatal, "%s: bitlen %d not allowed\n", __func__, bitlen );
					retv = 1; // error, unaligned field
					break;
				}
				break;
	}
	epicsMutexUnlock( e->rw_lock );

    return retv;
}

#define SHOW_GET_DEBUG 0
#define SHOW_SET_DEBUG 0
int drvGetValueMasked( ethcat *e, int offs, int bit, epicsUInt32 *rval, int bitlen, epicsInt16 nobt, epicsUInt16 shift, epicsUInt32 mask )
{

	FN_CALLED3;

	if( !e || !rval )
		return ERR_OPERATION_FAILED;

	if( !e->r_data )
		return ERR_OPERATION_FAILED;

#if SHOW_SET_DEBUG
	printf( "\n%s:get before: *val=0x%08x, ec*val= 0x%08x, wdata=0x%08x ecwdata=0x%08x (offs.bit:%d.%d, bitlen %d), mask=0x%08x\n", __func__,
			*rval, endian_uint32( *rval ), *(epicsUInt32 *)(e->w_data + offs), endian_uint32(*(epicsUInt32 *)(e->w_data + offs)), offs, bit, bitlen, mask );
#endif
	*rval = (endian_uint32(*(epicsUInt32 *)(e->r_data + offs)) & mask);
	*(epicsUInt32 *)(e->w_mask + offs) |= mask;

#if SHOW_SET_DEBUG
	printf( "%s:get  after: *val=0x%08x, ec*val= 0x%08x, wdata=0x%08x ecwdata=0x%08x (offs.bit:%d.%d, bitlen %d), mask=0x%08x\n", __func__,
			*rval, endian_uint32( *rval ), *(epicsUInt32 *)(e->w_data + offs), endian_uint32(*(epicsUInt32 *)(e->w_data + offs)), offs, bit, bitlen, mask );
#endif

    return 0;
}


int drvSetValueMasked( ethcat *e, int offs, int bit, epicsUInt32 *val, int bitlen, epicsInt16 nobt, epicsUInt16 shift, epicsUInt32 mask )
{
	FN_CALLED;

	if( !e || !val )
		return ERR_OPERATION_FAILED;

	if( !e->w_data || !e->w_mask )
		return ERR_OPERATION_FAILED;

	epicsMutexMustLock( e->rw_lock );

#if SHOW_GET_DEBUG
	printf( "\n%s: set before: *val=0x%08x, ec*val= 0x%08x, wdata=0x%08x ecwdata=0x%08x (offs.bit:%d.%d, bitlen %d), mask=0x%08x\n", __func__,
			*val, endian_uint32( *val ), *(epicsUInt32 *)(e->w_data + offs), endian_uint32(*(epicsUInt32 *)(e->w_data + offs)), offs, bit, bitlen, mask );
#endif

	*(epicsUInt32 *)(e->w_data + offs) &= endian_uint32( ~mask );
	*(epicsUInt32 *)(e->w_data + offs) |= endian_uint32( *val );
	*(epicsUInt32 *)(e->w_mask + offs) |= endian_uint32( mask );

#if SHOW_GET_DEBUG
	printf( "%s:  set after: *val=0x%08x, ec*val= 0x%08x, wdata=0x%08x ecwdata=0x%08x (offs.bit:%d.%d, bitlen %d), mask=0x%08x\n", __func__,
			*val, endian_uint32( *val ), *(epicsUInt32 *)(e->w_data + offs), endian_uint32(*(epicsUInt32 *)(e->w_data + offs)), offs, bit, bitlen, mask );
#endif
	epicsMutexUnlock( e->rw_lock );


	return 0;
}



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


/*-------------------------------------------------------------------*/
/*                                                                   */
/* IRQ thread                                                        */
/*                                                                   */
/*-------------------------------------------------------------------*/
void ec_irq_thread( void *data )
{
	ethcat *ec = (ethcat *)data;

    while(1)
    {
        epicsEventMustWait( ec->irq );

        scanIoRequest( ec->r_scan );
		scanIoRequest( ec->w_scan );

    }

}

/*-------------------------------------------------------------------*/
/*                                                                   */
/* Worker thread                                                     */
/*                                                                   */
/*-------------------------------------------------------------------*/

void copy_1bit( char *source, int sbyte_offs, int sbit_offs, char *dest, int dbyte_offs, int dbit_offs )
{
	if( sbit_offs > 7 )
	{
		sbyte_offs += sbit_offs / 8;
		sbit_offs %= 8;
	}
	if( dbit_offs > 7 )
	{
		dbyte_offs += dbit_offs / 8;
		dbit_offs %= 8;
	}

	*(dest+dbyte_offs) &= ~(0x1 << dbit_offs);
	*(dest+dbyte_offs) |= (((*(source+sbyte_offs) >> sbit_offs) & 1) << dbit_offs);

}


void process_sts_entries( ecnode *d )
{
	register int i;
	domain_register *from, *to;
	sts_entry **se;

	epicsMutexMustLock( d->ddata.sts_lock );
    for( se = &(d->ddata.sts); *se; se = &((*se)->next) )
    {
    	from = &((*se)->from);
    	to = &((*se)->to);
    	if( !(from->bitlen % 8) && !from->bit && from->bitlen &&
    			!(to->bitlen % 8) && !to->bit && to->bitlen )
    		memmove( d->ddata.dmem + to->offs, d->ddata.dmem + from->offs, from->bitlen / 8 );
    	else
    		for( i = 0; i < from->bitlen; i++ )
    			copy_1bit( d->ddata.dmem, from->offs, from->bitspec > -1 ? from->bitspec : from->bit,
    						d->ddata.dmem, to->offs, to->bitspec > -1 ? to->bitspec : to->bit );
    }
    epicsMutexUnlock( d->ddata.sts_lock );

}


inline void process_write_values( char *dmem, ecnode *d, char *wmask )
{
	register int i;

	for( i = 0; i < d->ddata.dsize; i++ )
	{
		d->ddata.dmem[i] &= ~wmask[i];
		d->ddata.dmem[i] |= d->ddata.wmem[i];
	}
	memset( wmask, 0, d->ddata.dsize );

}

int wt_counter[EC_MAX_DOMAINS] = { 0 },
		delayed[EC_MAX_DOMAINS] = { 0 },
		recd[EC_MAX_DOMAINS] = { 0 },
		forwarded[EC_MAX_DOMAINS] = { 0 };


void ec_worker_thread( void *data )
{
	int delayctr, dnr;
	struct timespec rec = { .tv_sec = 0, .tv_nsec = 50000 };
	ethcat *ec = (ethcat *)data;
	ec_master_t *ecm;
	ec_domain_t *ecd;

	FN_CALLED;

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

	dnr = ec->d->nr;

	ec->d->ddata.is_running = 1;

	//----------------------
	while( 1 )
	{

		ecrt_master_receive( ecm );
		ecrt_domain_process( ecd );

		epicsMutexMustLock( ec->rw_lock );
		memcpy( ec->d->ddata.rmem, ec->d->ddata.dmem, ec->d->ddata.dsize );
		process_write_values( ec->d->ddata.dmem, ec->d, ec->w_mask );
		process_sts_entries( ec->d );
		epicsMutexUnlock( ec->rw_lock );

		epicsEventSignal( ec->irq );

		ecrt_domain_queue( ecd );
		ecrt_master_send( ecm );

		forwarded[dnr] += tmr_wait( 0 );

		delayctr = 0;
		while( 1 )
		{
            ecrt_master_receive( ecm );
    		if( ec_domain_received( ec->d ) )
    		{
    			recd[dnr]++;
            	break;
    		}
			delayed[dnr]++;
            delayctr++;

			if( delayctr > 10 )
				break;
			clock_nanosleep( CLOCK_MONOTONIC, 0, &rec, NULL );
		}

		wt_counter[dnr]++;

	}

	ec->d->ddata.is_running = 0;

}

