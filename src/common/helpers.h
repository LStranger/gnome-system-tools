/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <gnome.h>

#if 0
int ip_first_entry_is_valid(GtkEditable *ip_entry);

int ip_entry_is_valid(GtkEditable *ip_entry);

gboolean check_ip_number(GtkEditable *ip_entry);

gboolean list_add_ip(GtkList *list, GtkWidget *w_ip);

void list_add_word(GtkList *list, GtkWidget *editable);

void clist_add_ip(GtkCList *clist, GtkWidget *w_ip_1, GtkWidget *w_ip_2,
		  GtkWidget *w_ip_3, GtkWidget *w_ip_4);

void clist_add_word(GtkCList *clist, GtkWidget *editable);
#endif

gboolean check_ip_string (const char *ip, gboolean allow_mask);
gboolean check_ip_entry (GtkEntry *entry, gboolean allow_mask);

void set_ctree_checkmark(GtkCTree *ctree, GtkCTreeNode *node,
			 gint column, gboolean state);

gboolean get_ctree_checkmark(GtkCTree *ctree, GtkCTreeNode *node, gint column);

void set_clist_checkmark(GtkCList *clist, gint row, gint column, gboolean state);

gboolean get_clist_checkmark(GtkCList *clist, gint row, gint column);
