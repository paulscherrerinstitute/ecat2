#ifndef ECENGINE_H
#define ECENGINE_H


//---------------------------------
void ec_worker_thread( void *data );
void ec_irq_thread( void *data );


//---------------------------------

#define IOCTL_MSG_DREC _IOWR(PSI_ECAT_VERSION_MAGIC, 100, int)




#endif


















