#include <gnome.h>
#include <gal/e-table/e-tree-simple.h>
#include "xml.h"

extern void network_create (void);
extern void network_populate (xmlNodePtr xml_root);
extern xmlNodePtr network_current_node (void);
extern void network_current_delete (void);
ETreePath *
network_insert_user (ETreeModel *etree, ETreePath *parent, int position, gpointer node_data);

