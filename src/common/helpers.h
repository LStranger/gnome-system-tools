#include <gnome.h>


int ip_first_entry_is_valid(GtkEditable *ip_entry);

int ip_entry_is_valid(GtkEditable *ip_entry);

gboolean check_ip_number(GtkEditable *ip_entry);

void list_add_ip(GtkList *list, GtkWidget *w_ip_1, GtkWidget *w_ip_2,
		 GtkWidget *w_ip_3, GtkWidget *w_ip_4);

void list_add_word(GtkList *list, GtkWidget *editable);

void clist_add_ip(GtkCList *clist, GtkWidget *w_ip_1, GtkWidget *w_ip_2,
		  GtkWidget *w_ip_3, GtkWidget *w_ip_4);

void set_ctree_checkmark(GtkCTree *ctree, GtkCTreeNode *node,
			 gint column, gboolean state);

gboolean get_ctree_checkmark(GtkCTree *ctree, GtkCTreeNode *node, gint column);

void set_clist_checkmark(GtkCList *clist, gint row, gint column, gboolean state);

gboolean get_clist_checkmark(GtkCList *clist, gint row, gint column);
