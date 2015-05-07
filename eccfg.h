#ifndef ECCFG_H
#define ECCFG_H


#define walk(current, parent) for( current = parent->child; current; current = current->next )
#define walk_t(current, parent, ntype) for( current = parent->child; current; current = current->next ) if( current->type == ntype )

#define EC_PAGE_SIZE	4096
//---------------------------------

int master_create_physical_config( ecnode *m );
int domain_create_autoconfig( ecnode *d );
ecnode *add_domain( ecnode *m, int rate );


//---------------------------------
extern ecnode *ecroot;

#endif


















