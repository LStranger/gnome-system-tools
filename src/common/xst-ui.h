/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <gnome.h>

#ifndef XST_UI_H
#define XST_UI_H

GtkWidget *xst_ui_list_get_list_item_by_name    (GtkList *list, const gchar *label);
void       xst_ui_combo_remove_by_label         (GtkCombo *combo, const gchar *label);
GtkWidget *xst_ui_create_image_widget           (gchar *name, gchar *string1, gchar *string2,
					         gint int1, gint int2);
gint       xst_ui_option_menu_get_selected_row  (GtkOptionMenu *option_menu);

void       xst_ui_clist_set_checkmark           (GtkCList *clist, gint row, gint column, gboolean state);
gboolean   xst_ui_clist_get_checkmark           (GtkCList *clist, gint row, gint column);

void       xst_ui_ctree_set_checkmark           (GtkCTree *ctree, GtkCTreeNode *node, gint column, gboolean state);
gboolean   xst_ui_ctree_get_checkmark           (GtkCTree *ctree, GtkCTreeNode *node, gint column);

#endif /* XST_UI_H */
