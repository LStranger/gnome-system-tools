/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include <glade/glade.h>
#include <gtk/gtk.h>

#ifndef GST_UI_H
#define GST_UI_H

GtkWidget *gst_ui_image_widget_create             (gchar *name, gchar *string1, gchar *string2,
						   gint int1, gint int2);
void       gst_ui_image_set_pix                   (GtkWidget *widget, gchar *filename);
GtkWidget *gst_ui_image_widget_get                (GladeXML *gui, gchar *name);

void       gst_ui_entry_set_text                  (void *entry, const gchar *str);

void       gst_ui_text_view_add_text              (GtkTextView *view, const gchar *text);
gchar     *gst_ui_text_view_get_text              (GtkTextView *view);
void       gst_ui_text_view_clear                 (GtkTextView *view);

#endif /* GST_UI_H */
