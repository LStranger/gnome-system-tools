/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <gnome.h>

#ifndef XST_UI_H
#define XST_UI_H

GtkWidget *xst_ui_list_get_list_item_by_name (GtkList *list, const gchar *label);
void       xst_ui_combo_remove_by_label      (GtkCombo *combo, const gchar *label);
GtkWidget *xst_ui_create_image_widget (gchar *name, gchar *string1, gchar *string2, gint int1, gint int2);

#endif /* XST_UI_H */
