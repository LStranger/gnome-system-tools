/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Tambet Ingo <tambet@ximian.com>
 *          Miguel de Icaza <miguel@ximian.com> (xst_ui_create_image_widget)
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "xst-ui.h"

/* For xst_ui_create_image_widget */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>

#include "checked.xpm"
#include "unchecked.xpm"


GdkPixmap *checked_pixmap = NULL, *unchecked_pixmap = NULL;
GdkBitmap *checked_mask = NULL, *unchecked_mask = NULL;


static GtkWidget *
get_list_item_by_name (GList *list, const gchar *label)
{
	GList *items, *children;
	GtkWidget *listitem;
	gchar *buf;
	GtkWidget *child;
	
	g_return_val_if_fail (list != NULL, NULL);

	items = list;

	while (items)
	{
		listitem = items->data;
		items = items->next;

		children = gtk_container_children (GTK_CONTAINER (listitem));

		while (children)
		{
			child    = children->data;
			children = children->next;

			if (GTK_IS_LABEL (child))
			{
				gtk_label_get (GTK_LABEL (child), &buf);
				if (strcmp (buf, label))
					continue;

				/* Found */
				return listitem;
			}
		}
	}

	return NULL;
}

GtkWidget *
xst_ui_list_get_list_item_by_name (GtkList *list, const gchar *label)
{
	GList *items;
	
	g_return_val_if_fail (list != NULL, NULL);
	g_return_val_if_fail (GTK_IS_LIST (list), NULL);

	items = gtk_container_children (GTK_CONTAINER (list));

	return get_list_item_by_name (items, label);
}

void
xst_ui_combo_remove_by_label (GtkCombo *combo, const gchar *label)
{
	GtkWidget *item;
	gchar *buf;
	
	g_return_if_fail (combo != NULL);
	g_return_if_fail (GTK_IS_COMBO (combo));

	if (!label)
		buf = gtk_entry_get_text (GTK_ENTRY (combo->entry));
	else
		buf = (void *)label;

	item = xst_ui_list_get_list_item_by_name (GTK_LIST (combo->list),
									  buf);
	if (item)
		gtk_widget_destroy (item);
}

/* Stolen and adapted from evolution's e-util/e-gui-utils.c
 * Arturo Espinosa <arturo@ximian.com> */
GtkWidget *xst_ui_create_image_widget(gchar *name,
				      gchar *string1, gchar *string2,
				      gint int1, gint int2)
{
	char *filename;
	GdkPixbuf *pixbuf;
	double width, height;
	GtkWidget *canvas, *alignment;
	if (string1) {
		filename = g_strdup(string1);
		pixbuf = gdk_pixbuf_new_from_file(filename);

		if (!pixbuf) {
			g_warning ("Pixmap %s not found.", filename);
			return NULL;
		}
		
		width = gdk_pixbuf_get_width(pixbuf);
		height = gdk_pixbuf_get_height(pixbuf);

		canvas = gnome_canvas_new_aa();
		GTK_OBJECT_UNSET_FLAGS(GTK_WIDGET(canvas), GTK_CAN_FOCUS);
		gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)),
				      gnome_canvas_pixbuf_get_type(),
				      "pixbuf", pixbuf,
				      NULL);

		alignment = gtk_widget_new(gtk_alignment_get_type(),
					   "child", canvas,
					   "xalign", (double) 0,
					   "yalign", (double) 0,
					   "xscale", (double) 0,
					   "yscale", (double) 0,
					   NULL);
	
		gtk_widget_set_usize(canvas, width, height);

		gdk_pixbuf_unref(pixbuf);

		gtk_widget_show(canvas);
		gtk_widget_show(alignment);
		g_free(filename);

		return alignment;
	} else
		return NULL;
}

/**
 * xst_ui_option_menu_get_selected_row: Returns index of selected label in GtkOptionMenu.
 * @option_menu: GtkOptionMenu to examine.
 *
 * This is a hack that makes it easier (possible) to define GtkOptionMenu choices in Glade
 * without adding special-case code to add signals to every GtkMenuItem that invoke
 * handlers based on the item selected. It is a bit rough, we could probably do without
 * comparing labels and just go directly for widget pointers, which would allow us to deal
 * with any kind of widget. Glade just does labels, though.
 *
 * Since the selected widget is taken out of the menu's list and placed in the OptionMenu
 * container, we temporarily select row 0, and when we find out which row is selected, we
 * re-select it. If anyone knows how to do this better, tell me how.
 *
 * Return value: List index in the range [0 .. n - 1], or -1 if the selected item is not a
 * label.
 **/
gint
xst_ui_option_menu_get_selected_row (GtkOptionMenu *option_menu)
{
	GtkWidget *selected, *found;
	GList *menu_items;
	gchar *label;
	gint row;

	selected = GTK_WIDGET (GTK_BIN (option_menu)->child);
	if (!GTK_IS_LABEL (selected))
		return -1;

	gtk_label_get (GTK_LABEL (selected), &label);
	gtk_option_menu_set_history (option_menu, 0);
	menu_items = GTK_MENU_SHELL (gtk_option_menu_get_menu (option_menu))->children;

	found = get_list_item_by_name (menu_items, label);
	if (!found)
		return 0;

	row = g_list_index (menu_items, found);
	gtk_option_menu_set_history (option_menu, row);
	return row;
}

void
xst_ui_clist_set_checkmark (GtkCList *clist, gint row, gint column, gboolean state)
{
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	if (state)
	{
		if (!checked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) checked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &checked_pixmap, &checked_mask, 1);
		}

		pixmap = checked_pixmap;
		mask = checked_mask;
	}
	else
	{
		if (!unchecked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) unchecked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &unchecked_pixmap, &unchecked_mask, 1);
		}

		pixmap = unchecked_pixmap;
		mask = unchecked_mask;
	}

	gtk_clist_set_pixmap (clist, row, column, pixmap, mask);
}

gboolean
xst_ui_clist_get_checkmark (GtkCList *clist, gint row, gint column)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	gtk_clist_get_pixmap (clist, row, column, &pixmap, &mask);

	if (pixmap == checked_pixmap) return (TRUE);
	return (FALSE);
}

void
xst_ui_ctree_set_checkmark (GtkCTree *ctree, GtkCTreeNode *node, gint column, gboolean state)
{
	GdkPixbuf *pixbuf;
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	if (state)
	{
		if (!checked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) checked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &checked_pixmap, &checked_mask, 1);
		}

		pixmap = checked_pixmap;
		mask = checked_mask;
	}
	else
	{
		if (!unchecked_pixmap)
		{
			pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) unchecked_xpm);
			gdk_pixbuf_render_pixmap_and_mask (pixbuf, &unchecked_pixmap, &unchecked_mask, 1);
		}

		pixmap = unchecked_pixmap;
		mask = unchecked_mask;
	}

	gtk_ctree_node_set_pixmap (ctree, node, column, pixmap, mask);
}

gboolean
xst_ui_ctree_get_checkmark (GtkCTree *ctree, GtkCTreeNode *node, gint column)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	gtk_ctree_node_get_pixmap (ctree, node, column, &pixmap, &mask);

	if (pixmap == checked_pixmap) return (TRUE);
	return (FALSE);
}
