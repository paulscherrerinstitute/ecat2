#ifndef ECENGINE_H
#define ECENGINE_H


//---------------------------------
void ec_worker_thread( void *data );
void ec_irq_thread( void *data );

void ec_shc_thread( void *data );
void set_master_health( ethcat *ec, int health );
int get_master_health( ethcat *ec );
void set_master_link_up( ethcat *ec, int link_up );
int get_master_link_up( ethcat *ec );
void set_slave_health( ethcat *ec, ecnode *s, int health );
int get_slave_health( ethcat *ec, ecnode *s );
void set_slave_aggregate_health( ethcat *ec, int health );
int get_slave_aggregate_health( ethcat *ec );

EC_ERR drvGetSysRecData( ethcat *e, system_rec_data *sysrecdata, dbCommon *record, epicsUInt32 *val );
int drvGetBlock(
		ethcat *e,
		char *buf,
		int offs,
		int len
);
//---------------------------------



#define IOCTL_MSG_DREC _IOWR(PSI_ECAT_VERSION_MAGIC, 100, int)


#define S_INIT 		0x01
#define S_PREOP		0x02
#define S_SAFEOP 	0x04
#define S_OP 		0x08

#endif


















