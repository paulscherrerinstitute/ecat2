#include "ec.h"

//----------------------------------------------------------------------------------------------



typedef struct _dev_ethercat_private {
	ethcat *e;

	int dreg_nr;
	domain_register dreg_info;

	ecnode *pe;

} devethercat_private;


static _rectype rectypes[] = {
		{ "ai", 		REC_AI, 			RIO_READ },
		{ "bi", 		REC_BI, 			RIO_READ },
		{ "mbbi", 		REC_MBBI, 			RIO_READ },
		{ "mbbiDirect", REC_MBBIDIRECT, 	RIO_READ },
		{ "longin", 	REC_LONGIN, 		RIO_READ },
		{ "stringin", 	REC_STRINGIN, 		RIO_READ },
		{ "ao", 		REC_AO, 			RIO_WRITE },
		{ "bo", 		REC_BO, 			RIO_WRITE },
		{ "mbbo", 		REC_MBBO, 			RIO_WRITE },
		{ "mbboDirect", REC_MBBODIRECT, 	RIO_WRITE },
		{ "longout", 	REC_LONGOUT, 		RIO_WRITE },
		{ "stringout", 	REC_STRINGOUT, 		RIO_WRITE },

		{ NULL }
};
static RECTYPE dev_get_record_type( dbCommon *prec, int *ix )
{
	if( !prec->rdes->name )
		return REC_ERROR;

	for( *ix = 0; rectypes[*ix].recname; (*ix)++ )
		if( !strcmp( rectypes[*ix].recname, prec->rdes->name ) )
			return rectypes[*ix].rtype;

	return REC_ERROR;
}


/*-------------------------------------------------------------------------------------*/
/*                                                                                     */
/* devethercat device support                                                          */
/*                                                                                     */
/*-------------------------------------------------------------------------------------*/

long dev_report( int interest )
{

	return 0;
}


static long dev_init( int phase )
{
  	if( phase == 0 )
		initHookRegister( &process_hooks );
	return 0;
}


static long dev_get_ioint_info( int dir, dbCommon *prec, IOSCANPVT *io)
{
	ethcat *e;
	int ix;
	RECTYPE retv = dev_get_record_type( prec, &ix );

	if( !prec->dpvt )
	{
		errlogSevPrintf( errlogFatal, "%s: record not initialized properly (1)\n", __func__ );
		return S_dev_badArgument;
	}

	e = ((devethercat_private *)prec->dpvt)->e;
	if( !e )
	{
		errlogSevPrintf( errlogFatal, "%s: record not initialized properly (2)\n", __func__ );
		return S_dev_badArgument;
	}

	if( retv == REC_ERROR )
		return S_dev_badArgument;

	switch( rectypes[ix].riotype )
	{
		case RIO_READ: 	*io = e->r_scan; break;
		case RIO_WRITE:	*io = e->w_scan; break;
		default:		return S_dev_success; // to supress gcc warning
	}

 	return S_dev_success;
}



/*----------------------------------------------------------*/
/*                                                          */
/* init_record                                              */
/*                                                          */
/*----------------------------------------------------------*/
int dev_trim_whitespaces( char *out, int len, const char *str )
{
	const char *end;
	int out_size;

	if( len == 0 )
		return FAIL;

	while( isspace(*str) )str++;

	if( !*str == 0 )
	{
		*out = 0;
		return OK;
	}

	end = str + strlen(str) - 1;
	while( end > str && isspace(*end) )
		end--;
	end++;

	out_size = (end - str) < len-1 ? (end - str) : len-1;

	memcpy( out, str, out_size );
	out[out_size] = 0;

	return OK;
}


static int dev_tokenize( char *s, const char *delims, char **tokens )
{
	char *p, *r;
	int i, ix = 0;

	r = p = s;
	for( i = 0, p = strsep( &r, delims ); p && ix < EPT_MAX_TOKENS-1; i++, (p = strsep( &r, delims)) )
		if( p[0] )
			tokens[ix++] = p;
	tokens[ix] = NULL;

	return ix;
}

int dev_parse_expression( char *token )
{
	int num = 0, mul = 1;
	char *tail, *s;

	if( !token )
		return -1;

	if( !isdigit( token[2] ) )
		return -1;

	s = token+2;
	while( s[0] )
	{
		tail = NULL;
		num = num + mul*(int)strtol( s, &tail, 10 );

//		printf( "token='%s', s='%s', tail='%s', mul=%d, num=%d\n", token, s, tail ? tail : "NULL", mul, num );

		s = tail;
		if( s )
		{
			if( s[0] == ')' )
				break;
		}
		else
			break;
		if( s[0] == '+' )
			mul = 1;
		else if( s[0] == '-' )
			mul = -1;
		else if( !isspace(s[0]) )
			return -1;
		s++;
	}

	return num;
}


int dev_parse_io_string( devethercat_private *priv, dbCommon *record, RECTYPE rectype, DBLINK *reclink )
{
	char *iotext, *tail;
	char *tokens[EPT_MAX_TOKENS] = { NULL }, *delims = " ._\t";
	int i, ntokens, ix = 0, len, retv = NOTOK, num, s_num, sm_num, p_num, e_num, b_num, d_num, r_num;

	dev_get_record_type( record, &ix );

	if( !record || !reclink || rectype == REC_ERROR )
	{
		errlogSevPrintf( errlogFatal, "%s: record, reclink or rectype invalid\n", __func__ );
		return S_dev_badArgument;
	}

	if( !reclink->text )
	{
		errlogSevPrintf( errlogFatal, "%s: INP/OUT string NULL\n", __func__ );
		return S_dev_badArgument;
	}

	if( !(len = strlen(reclink->text)) )
	{
		errlogSevPrintf( errlogFatal, "%s: INP/OUT string empty\n", __func__ );
		return S_dev_badArgument;
	}

	if( !(iotext = calloc(sizeof(char), len)) )
    {
        errlogSevPrintf( errlogFatal, "%s: %s %s: out of memory\n", __func__, rectypes[ix].recname, record->name );
		return S_dev_noMemory;
    }
	strcpy( iotext, reclink->text );

#if 0
	if( dev_trim_whitespaces( iotext, len, reclink->text ) == NOTOK )
    {
		free( iotext );
        errlogSevPrintf( errlogFatal, "%s: %s %s: INP/OUT string error (%s)\n", __func__, rectypes[ix].recname, record->name, reclink->text );
		return S_dev_badArgument;
    }
#endif

	if( !(ntokens = dev_tokenize( iotext, delims, tokens )) )
	{
		errlogSevPrintf( errlogFatal, "%s: INP/OUT string invalid\n", __func__ );
		return S_dev_badArgument;
	}

	// Possible INP/OUT links:
	//
	// Daa.Rbb            : domain register nn
	// Saa.SMbb.Pcc.Edd   : pdo entry a.b.c.d
	//
	s_num = sm_num = p_num = e_num = b_num = d_num = r_num = -1;
	for( i = 0; i < ntokens; i++ )
	{
		if( !tokens[i] )
			continue;

		if( isdigit(tokens[i][1]) )
			num = (int)strtol( tokens[i]+1, &tail, 10 );
		else if( strlen(tokens[i]) > 3 && tokens[i][1] == '(' && tokens[i][strlen(tokens[i])-1] == ')')
		{
			num = dev_parse_expression( tokens[i] );
//			printf( "parsed %s as %d\n", tokens[i], num );
		}
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

	free( iotext );


/*
	if( d_num < 0 )
    {
		free( iotext );
        errlogSevPrintf( errlogFatal, "%s: domain missing (%s) for record %s (type %s)\n", __func__, reclink->text, record->name, rectypes[ix].recname );
        return ERR_BAD_ARGUMENT;
    }
*/
	if( d_num < 0 )
		d_num = 0;

    priv->e = ethercatOpen( d_num );
    if( priv->e == NULL )
    {
        errlogSevPrintf( errlogFatal, "%s: cannot open domain %d for record %s (type %s)\n", __func__, d_num, record->name, rectypes[ix].recname );
        return ERR_BAD_ARGUMENT;
    }

	// check to see what kind of INP/OUT info was discovered
    if( r_num >= 0 )
    {
    	if( drvGetRegisterDesc( priv->e, &priv->dreg_info, priv->dreg_nr = r_num, &priv->pe, b_num) != OK )
    	{
    		errlogSevPrintf( errlogFatal, "%s: %s (type %s): INP/OUT string error (%s), domain %d register number %d not found\n",
    												__func__, record->name, rectypes[ix].recname, reclink->text, d_num, r_num );
    		return ERR_BAD_ARGUMENT;
    	}

    	retv = OK;
    }
    else if( s_num >=0 && sm_num >= 0 && p_num >= 0 && e_num >=0 )
    {
    	if( drvGetEntryDesc( priv->e, &priv->dreg_info, &priv->dreg_nr, &priv->pe, s_num, sm_num, p_num, e_num, b_num ) != OK )
    	{

			errlogSevPrintf( errlogFatal, "%s: %s (type %s): INP/OUT string error (%s) in domain %d\n",
													__func__, record->name, rectypes[ix].recname, reclink->text, d_num );
			return ERR_BAD_ARGUMENT;
	    }
    	retv = OK;
    }
    else
	{
		errlogSevPrintf( errlogFatal, "%s: %s (type %s): INP/OUT string error (%s), domain %d, entry link not valid or incomplete\n",
												__func__, record->name, rectypes[ix].recname, reclink->text, d_num );
		return ERR_BAD_ARGUMENT;
	}

	return retv;
}




#define CHECK_RECINIT                                                                                      \
if( !priv )                                                                                                \
{                                                                                                          \
    recGblSetSevr( record, UDF_ALARM, INVALID_ALARM );                                                     \
    errlogSevPrintf(errlogFatal, "%s %s: record not initialized correctly\n", __func__, record->name );    \
    return -1;                                                                                             \
}                                                                                                          \

#define CHECK_STATUS                                                                                                 \
if( status )                                                                                                         \
{                                                                                                                    \
    errlogSevPrintf(errlogFatal, "%s failed for record %s: error code 0x%x\n", __func__, record->name, status );     \
    recGblSetSevr(record, READ_ALARM, INVALID_ALARM );                                                               \
}                                                                                                                    \



//----------------------------------------------------------
//
// ai
//
//----------------------------------------------------------

long dev_rw_ai( aiRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, (epicsUInt32 *)&(record->rval),
    												priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}


long dev_special_linconv_ai( aiRecord *record, int after )
{
    if( after )
    {
        record->eslo = (record->eguf - record->egul)/(epicsFloat64)0xffffff;
        record->eoff = record->egul;
    }

#if 0
    printf( "---------------------------------------\n");
    printf( "eslo=%.10f, eoff=%.10f, eguf=%.10f, egul=%16.10f\n",
                record->eslo, record->eoff, record->eguf, record->egul );
    printf( "---------------------------------------\n");
#endif
    return 0;
}




//----------------------------------------------------------
//
// ao
//
//----------------------------------------------------------

long dev_rw_ao( aoRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit,
    							(epicsUInt32 *)&(record->rval), priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}


long dev_special_linconv_ao( aoRecord* record, int after )
{

	if( after )
    {
        record->eslo = (record->eguf - record->egul)/(epicsFloat64)0xffffff;
        record->eoff = record->egul;
    }

#if 0
    printf( "---------------------------------------\n");
    printf( "eslo=%.10f, eoff=%.10f, eguf=%.10f, egul=%16.10f\n",
                record->eslo, record->eoff, record->eguf, record->egul );
    printf( "---------------------------------------\n");
#endif

    return 0;
}



//----------------------------------------------------------
//
// bi
//
//----------------------------------------------------------

long dev_rw_bi( biRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, &(record->rval),
    													priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}



//----------------------------------------------------------
//
// bo
//
//----------------------------------------------------------

long dev_rw_bo( boRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvSetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, &(record->rval),
    													priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}


//----------------------------------------------------------
//
// mbbi
//
//----------------------------------------------------------

long dev_rw_mbbi( mbbiRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvGetValueMasked( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, &record->rval, priv->dreg_info.bitlen,
    									record->nobt, record->shft, record->mask );
    CHECK_STATUS;

    return status;
}


//----------------------------------------------------------
//
// mbbo
//
//----------------------------------------------------------

long dev_rw_mbbo( mbboRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvSetValueMasked( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, &record->rval, priv->dreg_info.bitlen,
			record->nobt, record->shft, record->mask );
    CHECK_STATUS;

    return status;
}



//----------------------------------------------------------
//
// longin
//
//----------------------------------------------------------

long dev_rw_longin( longinRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit,
    								(epicsUInt32 *)&(record->val), priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}



//----------------------------------------------------------
//
// longout
//
//----------------------------------------------------------

long dev_rw_longout( longoutRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;

   	CHECK_RECINIT;

    status = drvSetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit,
    								(epicsUInt32 *)&(record->val), priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}




//----------------------------------------------------------
//
// INIT
//
//----------------------------------------------------------
#define PARSE_IO_STR( rtype, inout )                                                                         \
reclink = &((rtype##Record *)record)->inout;                                                               \
if( dev_parse_io_string( priv, record, rectype, reclink ) != OK )                                            \
{                                                                                                            \
	errlogSevPrintf( errlogFatal, "%s: Parsing INP/OUT link '%s' failed (record %s, type %s)\n", __func__,   \
			reclink->text, record->name, rectypes[ix].recname );                                             \
	return S_dev_badArgument;                                                                                \
}

long dev_init_record(
    dbCommon *record
)
{
  	devethercat_private *priv;
  	DBLINK *reclink;
  	int ix = 0;
  	char bitsp[20] = { 0 }, bitsp2[20] = { 0 };
  	RECTYPE rectype = dev_get_record_type( record, &ix );

  	if( rectype == REC_ERROR )
    {
        errlogSevPrintf( errlogFatal, "%s: record %s of type '%s' is not supported \n", __func__, record->name, rectypes[ix].recname );
        return ERR_BAD_REQUEST;
    }
    priv = calloc( sizeof(devethercat_private), 1 );
    if( priv == NULL )
    {
        errlogSevPrintf( errlogFatal, "%s: %s %s: out of memory\n", __func__, rectypes[ix].recname, record->name );
        return ERR_OUT_OF_MEMORY;
    }
    record->dpvt = priv;

    switch( rectype )
    {
    	case REC_AI:
    					PARSE_IO_STR( ai, inp );
						dev_special_linconv_ai( (aiRecord *)record, 1 );
						break;

    	case REC_AO:
    					PARSE_IO_STR( ao, out );
    					dev_special_linconv_ao( (aoRecord *)record, 1 );
						break;

    	case REC_BI:
    					PARSE_IO_STR( bi, inp );
    		    		break;

    	case REC_BO:
						PARSE_IO_STR( bo, out );
    					break;

    	case REC_MBBI:
						PARSE_IO_STR( mbbi, inp );
    					break;

    	case REC_MBBO:
						PARSE_IO_STR( mbbo, out );
    					break;

    	case REC_LONGIN:
						PARSE_IO_STR( longin, inp );
    					break;

    	case REC_LONGOUT:
						PARSE_IO_STR( longout, out );
						break;

    	default:
						errlogSevPrintf( errlogFatal, "%s: record %s currently not supported\n", __func__, record->name );
						return S_dev_badArgument;
    }

    // make a somewhat prettier printout
    if( priv->dreg_info.bitspec >= 0 )
    {
    	sprintf( bitsp, ".b%d", priv->dreg_info.bitspec );
    	sprintf( bitsp2, "(+%d)", priv->dreg_info.bitspec );
    }

    printf( PPREFIX "configured record %s (%s) as d%d.r%d (s%d.sm%d.p%d.e%d%s), offset %d.%d%s (%d bit%s)\n",
    			record->name,
				record->rdes->name,
				priv->e->dnr,
				priv->dreg_nr,
				priv->pe->parent->parent->parent->nr,
				priv->pe->parent->parent->nr,
				priv->pe->parent->nr,
				priv->pe->nr,
				bitsp,
				priv->dreg_info.offs,
				priv->dreg_info.bit,
				bitsp2,
				priv->dreg_info.bitlen,
				priv->dreg_info.bitlen == 1 ? "" : "s"
    		);

    return 0;

}

#undef PARSE_IO_STR


devsupport_linconv( ai );
devsupport_linconv( ao );
devsupport( bi );
devsupport( bo );
devsupport( mbbi );
devsupport( mbbo );
devsupport( longin );
devsupport( longout );




