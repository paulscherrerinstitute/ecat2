#ifndef ECTOOLS_H
#define ECTOOLS_H


//---------------------------------
long ConfigEL6692( int slave_pos, char *io, int bitlen );
void configure_el6692_entries( ec_master_t *master );
int slave_is_6692( int slave_pos );
int get_no_pdos_6692( int sm_num );
int get_pdo_info_6692( ec_master_t *ecm, int i, int j, int k, ec_pdo_info_t *pdo_t );
int get_pdo_entry_info_6692( ec_master_t *ecm, int i, int j, int k, int l, ec_pdo_entry_info_t *pdo_entry_t );

long dmap( char *cmd );
long stat( int dnr );

long sts( char *from, char *to );

long cfgslave( char *cmd, int slave_nr, int sm_nr, int pdo_ix_dir, int entry_ix_wd_mode, int entry_sub_ix, int entry_bitlen );
EC_ERR execute_configuration_prg( void );

//---------------------------------

#endif


















