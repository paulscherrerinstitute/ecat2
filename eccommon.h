#ifndef ECCOMMON_H
#define ECCOMMON_H


#define FREQ_1Hz	1000000
#define FREQ_2Hz	500000
#define FREQ_3Hz	333333
#define FREQ_4Hz	250000
#define FREQ_5Hz	200000

#define FREQ_10Hz	100000
#define FREQ_100Hz	10000
#define FREQ_1000Hz	1000

#define FREQ_2000Hz	500
#define FREQ_3000Hz	333
#define FREQ_4000Hz	250
#define FREQ_5000Hz	200

#define FREQ_10000Hz 100

#define FREQ_1KHz	FREQ_1000Hz
#define FREQ_2KHz	FREQ_2000Hz
#define FREQ_3KHz	FREQ_3000Hz
#define FREQ_4KHz	FREQ_4000Hz
#define FREQ_5KHz	FREQ_5000Hz
#define FREQ_10KHz	FREQ_10000Hz



#define FREQ_MIN		10
#define FREQ_MAX		2000000001
//#define FREQ_DEFAULT	FREQ_1Hz
//#define FREQ_DEFAULT	FREQ_100Hz
#define FREQ_DEFAULT	FREQ_1000Hz
//#define FREQ_DEFAULT	FREQ_2000Hz

#define DEVICE_NAME "psi_ethercat"
#define BUF_LEN 80
#define LBUF_LEN 256
#define MAX_SBUF	1024
#define SBUF_SAFETY	(MAX_SBUF-64)

#define M_LOCK              down( &ecatm->master->master_sem )
#define M_UNLOCK            up( &ecatm->master->master_sem )

#define PSI_ECAT_VERSION_MAGIC        0xdd

#define IOCTL_MSG_R  _IOWR(PSI_ECAT_VERSION_MAGIC, 1, int)
#define IOCTL_MSG_W  _IOWR(PSI_ECAT_VERSION_MAGIC, 2, int)
#define IOCTL_MSG_RW  _IOWR(PSI_ECAT_VERSION_MAGIC, 10, int)
#define IOCTL_MSG_DREG  _IOWR(PSI_ECAT_VERSION_MAGIC, 20, int)
#define IOCTL_MSG_DOMEXISTS _IOWR(PSI_ECAT_VERSION_MAGIC, 21, int)

//------------------------------------------


//-------------------------------
typedef struct _ecd_master ecd_master;
typedef struct _ecd_domain ecd_domain;
typedef struct _ecnode ecnode;
//-------------------------------

//
//
typedef enum  {
	DCT_NOT_CONFIGURED = 0,
	DCT_CONFIGURED = 0x01,
	DCT_AUTOCONFIG = 0x10
} DOMAIN_CONFIG_TYPE;

//------------------------------------------


typedef struct _domain_reginfo {
	ecnode *master;
	ecnode *slave;
	ecnode *sync;
	ecnode *pdo;
	ecnode *pdo_entry;

	ecnode *domain;
	ecnode *domain_entry;

	unsigned int byte;
	unsigned int bit;
	unsigned int bit_length;
} domain_reg_info;

typedef struct {
	ecnode *from;
	ecnode *to;
} sts_entry;


typedef struct _ecd_domain {

    ecnode *nmaster;
    struct task_struct *domain_thread;
    struct dentry *mmfile;


    int is_running;
    int rate; // in microseconds

   // struct hrtimer_sleeper ts;

//    struct mutex rwmut;

//    struct semaphore rsem;
//    struct semaphore wsem;
//    struct semaphore rwsem;
    char *dmem;    // domain mem
    char *rmem;    // R mem
    char *wmem;    // W mem
    int dallocated;
    int dsize;

    // sts
    int num_of_sts_entries;
    sts_entry *sts;

	// cfg
    DOMAIN_CONFIG_TYPE dcfgtype;
	int num_of_regs;
	ec_pdo_entry_reg_t *regs;
	domain_reg_info *reginfos;

} ecd_domain;

//------------------------------------------


typedef struct _ecd_master {
	ec_master_t *master;

	epicsMutexId io_lock;

	ec_master_info_t master_info;
    ec_master_state_t master_state;

    struct task_struct *master_thread;

} ecd_master;


//-------------------------------------
typedef enum {
	ECNT_ERROR = 0,
	ECNT_ROOT,            // 1
	ECNT_MASTER,          // 2
	ECNT_SLAVE,           // 3
	ECNT_SYNC,            // 4
	ECNT_PDO,             // 5
	ECNT_PDO_ENTRY,       // 6

	ECNT_DOMAIN,          // 7
	ECNT_DOMAIN_ENTRY     // 8
} ECN_TYPE;

// very flat node structure for
// easier access to members in code
typedef struct _ecnode {
	ECN_TYPE	type;
	struct _ecnode *parent;
	struct _ecnode *next;
	struct _ecnode *child;

	// common
	int nr;
	struct proc_dir_entry *pdire;
	char *path;


	ec_pdo_entry_info_t pdo_entry_t;
	ec_pdo_info_t 		pdo_t;
	ec_sync_info_t 		sync_t;
	ec_slave_info_t 	slave_t;

	union
	{
		ec_master_t *master_t;
		ec_domain_t *domain_t;
	};

	union
	{
		ecd_master mdata;
		ecd_domain ddata;
	};


	// if pdo entry belongs to a domain
	ecnode *de_master;
	ecnode *de_slave;
	ecnode *de_sync;
	ecnode *de_pdo;
	ecnode *de_pdo_entry;
	ecnode *de_domain;

	ecnode *domain_entry;

	int id;
	//ec_pdo_entry_t pe;
} ecnode;



//-----------------------------------------

typedef enum {
	ERR_NO_ERROR = 0,

	ERR_OUT_OF_MEMORY 		= 100,
	ERR_BAD_ARGUMENT 		= 101,
	ERR_BAD_REQUEST 		= 102,
	ERR_OPERATION_FAILED 	= 103,
	ERR_DOES_NOT_EXIST		= 104,
	ERR_ALREADY_EXISTS 		= 105,

/*
	ERR_ = 10,
	ERR_ = 10,
	ERR_ = 10,
	ERR_ = 10,
	ERR_ = 10,
	ERR_ = 10,
	ERR_ = 10,
	ERR_ = 10,
*/



} EC_ERR;


#define PPREFIX "***** ecat2: "
#define EPT_MAX_TOKENS 16


#endif


















