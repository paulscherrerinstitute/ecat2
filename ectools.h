#ifndef ECTOOLS_H
#define ECTOOLS_H


//---------------------------------
long ConfigEL6692( int slave_pos, char *io, int bitlen );
void configure_el6692_entries( ec_master_t *master );

long dmap( char *cmd );
long stat( int level, int dnr );

long sts( char *from, char *to );
//---------------------------------

#endif


















