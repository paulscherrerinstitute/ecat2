#include "ec.h"

//----------------------------------------------------------------------------------------------


typedef struct _dev_ethercat_private {
	ethcat *e;

	int dreg_nr;
	domain_register dreg_info;

	ecnode *pe;

} devethercat_private;


_rectype rectypes[] = {
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
	FN_CALLED;

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
	FN_CALLED;

	return 0;
}


static long dev_init( int phase )
{
	FN_CALLED;

	if( phase == 0 )
		initHookRegister( &process_hooks );
	return 0;
}


static long dev_get_ioint_info( int dir, dbCommon *prec, IOSCANPVT *io)
{
	ethcat *e;
	int ix;
	RECTYPE retv = dev_get_record_type( prec, &ix );
	FN_CALLED;

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
	FN_CALLED;

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


int dev_tokenize( char *s, const char *delims, char **tokens )
{
	char *p, *r;
	int i, ix = 0;
	FN_CALLED;

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
	FN_CALLED;

	if( !token )
		return -1;

	if( !isdigit( token[2] ) )
		return -1;

	s = token+2;
	while( s[0] )
	{
		tail = NULL;
		num = num + mul*(int)strtol( s, &tail, 10 );

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



int parse_str( char *s, ethcat **e, ecnode **pe, int *dreg_nr, domain_register *dreg )
{
	char iotext[128], *tail, *tokens[EPT_MAX_TOKENS] = { NULL }, *delims = " ._\t";
	int i, ntokens, retv = NOTOK, num, token_num[EPT_MAX_TOKENS];

	FN_CALLED;
	strcpy( iotext, s );

	if( !(ntokens = dev_tokenize( iotext, delims, tokens )) )
	{
		errlogSevPrintf( errlogFatal, "%s: INP/OUT string invalid\n", __func__ );
		return S_dev_badArgument;
	}

	// Possible INP/OUT links:
	//
	// dDaa.Rbb[.Bcc]              : domain register nn, optional bit cc
	// Saa.SMbb.Pcc.Edd            : pdo entry a.b.c.d
	// Saa.SMbb.Pcc.Edd.Bee        : pdo entry a.b.c.d, optional bit ee
	// Saa.SMbb.Pcc.Edd.Lnn        : pdo entry a.b.c.d, optional string len nn
	// Saa.SMbb.Pcc.Edd.Oee[.Lnn]  : pdo entry a.b.c.d, optional rel. offset ee, optional string len nn
	//
	for( i = 0; i < EPT_MAX_TOKENS; i++ )
		token_num[i] = -1;
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
			case 'D':	token_num[D_NUM] = num; break;
			case 'R':	token_num[R_NUM] = num; break;
			case 'S':	if( toupper(tokens[i][1]) == 'M' )
							token_num[SM_NUM] = (int)strtol( tokens[i]+2, &tail, 10 );
						else
							token_num[S_NUM] = num; break;
						break;
			case 'P':	token_num[P_NUM] = num; break;
			case 'E':	token_num[E_NUM] = num; break;
			case 'B':	token_num[B_NUM] = num; break;
			case 'L':   token_num[L_NUM] = num; break;
			case 'O':   token_num[O_NUM] = num; break;

		}


	}


	if( token_num[D_NUM] < 0 )
		token_num[D_NUM] = 0;

    *e = ethercatOpen( token_num[D_NUM] );
    if( *e == NULL )
    {
        errlogSevPrintf( errlogFatal, "%s: cannot open domain %d\n", __func__, token_num[D_NUM] );
        return ERR_BAD_ARGUMENT;
    }

	// check to see what kind of INP/OUT info was discovered
    if( token_num[R_NUM] >= 0 )
    {
    	if( drvGetRegisterDesc( *e, dreg, *dreg_nr = token_num[R_NUM], pe, token_num) != OK )
    	{
    		errlogSevPrintf( errlogFatal, "%s: INP/OUT string error (%s), domain %d, register number %d not found\n",
    												__func__, iotext, token_num[D_NUM], token_num[R_NUM] );
    		return ERR_BAD_ARGUMENT;
    	}

    	retv = OK;
    }
    else if( token_num[S_NUM] >= 0 && token_num[SM_NUM] >= 0 && token_num[P_NUM] >= 0 && token_num[E_NUM] >=0 )
    {
    	if( drvGetEntryDesc( *e, dreg, dreg_nr, pe, token_num ) != OK )
    	{

			errlogSevPrintf( errlogFatal, "%s: INP/OUT string error (%s) in domain %d\n", __func__, iotext, token_num[D_NUM] );
			return ERR_BAD_ARGUMENT;
	    }
    	retv = OK;
    }
    else
	{
		errlogSevPrintf( errlogFatal, "%s: INP/OUT string error (%s), domain %d, entry link not valid or incomplete\n",
												__func__, iotext, token_num[D_NUM] );
		return ERR_BAD_ARGUMENT;
	}

    return retv;
}


int dev_parse_io_string( devethercat_private *priv, dbCommon *record, RECTYPE rectype, DBLINK *reclink )
{
	FN_CALLED;
	int dreg_nr, ix;

	FN_CALLED;
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

	if( !strlen(reclink->text) )
	{
		errlogSevPrintf( errlogFatal, "%s: INP/OUT string empty\n", __func__ );
		return S_dev_badArgument;
	}


	return parse_str( reclink->text, &priv->e, &priv->pe, &dreg_nr, &priv->dreg_info );
}


#define CHECK_RECINIT                                                                                      \
if( !priv )                                                                                                \
{                                                                                                          \
    recGblSetSevr( record, UDF_ALARM, INVALID_ALARM );                                                     \
    errlogSevPrintf(errlogFatal, "%s %s: record not initialized correctly (1)\n", __func__, record->name );    \
    return -1;                                                                                             \
}\
if( !priv->e )                                                                                                \
{                                                                                                          \
    recGblSetSevr( record, UDF_ALARM, INVALID_ALARM );                                                     \
    errlogSevPrintf(errlogFatal, "%s %s: record not initialized correctly (2)\n", __func__, record->name );    \
    return -1;                                                                                             \
}

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
	FN_CALLED;

   	CHECK_RECINIT;


    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, (epicsUInt32 *)&(record->rval),
    												priv->dreg_info.bitlen, priv->dreg_info.bitspec, 0 );
    CHECK_STATUS;

    return status;
}


long dev_special_linconv_ai( aiRecord *record, int after )
{
	FN_CALLED;

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
	FN_CALLED;

  	CHECK_RECINIT;
    status = drvSetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit,
    							(epicsUInt32 *)(&record->rval), priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}


long dev_special_linconv_ao( aoRecord* record, int after )
{
	FN_CALLED;

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
	FN_CALLED;

   	CHECK_RECINIT;

    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, &(record->rval),
    													priv->dreg_info.bitlen, priv->dreg_info.bitspec, 0 );
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
	FN_CALLED;

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
	FN_CALLED;

   	CHECK_RECINIT;

    status = drvGetValueMasked( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, &record->rval, priv->dreg_info.bitlen,
    							priv->dreg_info.nobt, priv->dreg_info.shft, priv->dreg_info.mask );
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
	FN_CALLED;

   	CHECK_RECINIT;

    status = drvSetValueMasked( priv->e, priv->dreg_info.offs, priv->dreg_info.bit, &record->rval, priv->dreg_info.bitlen,
    							priv->dreg_info.nobt, priv->dreg_info.shft, priv->dreg_info.mask );
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
	FN_CALLED;

   	CHECK_RECINIT;

    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit,
    								(epicsUInt32 *)&(record->val), priv->dreg_info.bitlen, priv->dreg_info.bitspec, 0 );
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
	FN_CALLED;

   	CHECK_RECINIT;

    status = drvSetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit,
    								(epicsUInt32 *)&(record->val), priv->dreg_info.bitlen, priv->dreg_info.bitspec );
    CHECK_STATUS;

    return status;
}




//----------------------------------------------------------
//
// stringin
//
//----------------------------------------------------------

long dev_rw_stringin( stringinRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;
	FN_CALLED;

   	CHECK_RECINIT;

    status = drvGetValue( priv->e, priv->dreg_info.offs, priv->dreg_info.bit,
    								(epicsUInt32 *)&(record->val), priv->dreg_info.bitlen, priv->dreg_info.bitspec, 0 );
    CHECK_STATUS;

    return status;
}



//----------------------------------------------------------
//
// stringout
//
//----------------------------------------------------------

long dev_rw_stringout( stringoutRecord *record )
{
   	int status = 0;
   	devethercat_private  *priv = (devethercat_private *)record->dpvt;
	FN_CALLED;

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

static void add_record( int ix, devethercat_private *priv, dbCommon *record )
{
	conn_rec **cr;
	FN_CALLED;

	if( priv->pe == NULL )
	{
		errlogSevPrintf( errlogFatal, "%s: Cannot add record, PDO entry undefined.\n", __func__ );
		return;
	}
    // is this rec already registered?
    for( cr = &priv->pe->cr; *cr; cr = &(*cr)->next )
    	if( (*cr)->rec == record )
    	{
    		errlogSevPrintf( errlogFatal, "%s: Somehow, this same record has already been registered\n", __func__ );
    		return;
    	}


    *cr = calloc( 1, sizeof(conn_rec) );
	if( *cr == NULL )
	{
		errlogSevPrintf( errlogFatal, "%s: Memory allocation failed.\n", __func__ );
		return;
	}

	(*cr)->ix = ix;
    (*cr)->rectype = rectypes[ix].rtype;
    (*cr)->rec = record;
	(*cr)->dreg_info = &priv->dreg_info;

}

#define PARSE_IO_STR( rtype, inout )                                                                         \
reclink = &((rtype##Record *)record)->inout;                                                               \
if( dev_parse_io_string( priv, record, rectype, reclink ) != OK )                                            \
{                                                                                                            \
	errlogSevPrintf( errlogFatal, "%s: Parsing INP/OUT link '%s' failed (record %s, type %s)\n", __func__,   \
			reclink->text, record->name, rectypes[ix].recname );                                             \
	return ERR_BAD_ARGUMENT;                                                                                \
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
	FN_CALLED;

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
						priv->dreg_info.nobt = ((mbbiRecord *)record)->nobt;
						priv->dreg_info.shft = ((mbbiRecord *)record)->shft;
						priv->dreg_info.mask = ((mbbiRecord *)record)->mask << ((mbbiRecord *)record)->shft;
    					break;

    	case REC_MBBO:
						PARSE_IO_STR( mbbo, out );
						priv->dreg_info.nobt = ((mbboRecord *)record)->nobt;
						priv->dreg_info.shft = ((mbboRecord *)record)->shft;
						priv->dreg_info.mask = ((mbboRecord *)record)->mask << ((mbboRecord *)record)->shft;
    					break;

    	case REC_LONGIN:
						PARSE_IO_STR( longin, inp );
    					break;

    	case REC_LONGOUT:
						PARSE_IO_STR( longout, out );
						break;

    	case REC_STRINGIN:
						PARSE_IO_STR( stringin, inp );
						break;

    	case REC_STRINGOUT:
						PARSE_IO_STR( stringout, out );
						break;

    	default:
						errlogSevPrintf( errlogFatal, "%s: record %s currently not supported\n", __func__, record->name );
						return ERR_BAD_ARGUMENT;
    }

	add_record( ix, priv, record );

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
devsupport( stringin );
devsupport( stringout );



