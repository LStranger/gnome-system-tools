/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <gnome.h>
#include <glade/glade.h>

#ifndef GST_UI_H
#define GST_UI_H

void       gst_ui_combo_remove_by_label           (GtkCombo *combo, const gchar *label);
GtkWidget *gst_ui_image_widget_create             (gchar *name, gchar *string1, gchar *string2,
						   gint int1, gint int2);
void       gst_ui_image_set_pix                   (GtkWidget *widget, gchar *filename);
GtkWidget *gst_ui_image_widget_get                (GladeXML *gui, gchar *name);

gint       gst_ui_option_menu_get_selected_row    (GtkOptionMenu *option_menu);
gchar     *gst_ui_option_menu_get_selected_string (GtkOptionMenu *option_menu);
void       gst_ui_option_menu_set_selected_string (GtkOptionMenu *option_menu, const gchar *string);
GtkWidget *gst_ui_option_menu_add_string          (GtkOptionMenu *option_menu, const gchar *string);
void       gst_ui_option_menu_remove_string       (GtkOptionMenu *option_menu, const gchar *string);
void       gst_ui_option_menu_clear               (GtkOptionMenu *option_menu);

void       gst_ui_entry_set_text                  (void *entry, const gchar *str);

void       gst_ui_text_view_add_text              (GtkTextView *view, const gchar *text);
gchar     *gst_ui_text_view_get_text              (GtkTextView *view);
void       gst_ui_text_view_clear                 (GtkTextView *view);

#endif /* GST_UI_H */
