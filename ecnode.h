#ifndef ECNODE_H
#define ECNODE_H

ecnode *ecn_add_child( ecnode *parent );
ecnode *ecn_add_child_type( ecnode *parent, ECN_TYPE type );
void ecn_remove_node( ecnode *node );
void ecn_delete_branch( ecnode *n );

ecnode *ecn_get_child_nr( ecnode *parent, int nr );
ecnode *ecn_get_next_child_nr( ecnode *parent, int *last_nr );
ecnode *ecn_get_child_nr_type( ecnode *parent, int nr, ECN_TYPE type );
int ecn_get_first_free_child_nr_type( ecnode *parent, ECN_TYPE type );



ecnode *ecn_get_domain_nr( int m_nr, int d_nr );
ecnode *ecn_get_pdo_entry_nr( int m_nr, int s_nr, int sm_nr, int p_nr, int pe_nr );

#endif


















