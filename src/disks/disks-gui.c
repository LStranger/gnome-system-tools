/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* disks-gui.c: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
 * 
 * Copyright (C) 2001 Ximian, Inc.
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
 * Authors: Alvaro Peña Gonzalez <apg@esware.com>
 *          Carlos Garcia Campos <elkalmail@yahoo.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "gst.h"

#include "disks-config.h"
#include "disks-storage.h"
#include "disks-storage-disk.h"
#include "disks-storage-partition.h"
#include "disks-gui.h"
#include "callbacks.h"

extern GstTool *tool;
GnomeIconTheme *icon_theme;

GtkItemFactoryEntry popup_partition_menu_items[] = {
	{ N_("/_Format"), NULL, NULL, POPUP_PARTITION_FORMAT, "<StockItem>", GTK_STOCK_DELETE },
	{ N_("/_Delete"), NULL, NULL, POPUP_PARTITION_REMOVE, "<StockItem>", GTK_STOCK_REMOVE }
};

static char *
disks_partition_item_factory_trans (const char *path, gpointer data)
{
	return _((gchar*)path);
}

static GtkItemFactory *
gst_disks_partition_popup_item_factory_create (GtkWidget *treeview)
{
	GtkItemFactory *item_factory;

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);
	gtk_item_factory_set_translate_func (item_factory, disks_partition_item_factory_trans,
					     NULL, NULL);
	gtk_item_factory_create_items (item_factory, G_N_ELEMENTS (popup_partition_menu_items),
				       popup_partition_menu_items,
				       (gpointer) treeview);

	return item_factory;
}

gchar *
gst_storage_get_human_readable_size (const gulong size)
{
	if (size == 0)
		return g_strdup ("");
	if ((size / 1024) <= 1024)
		return g_strdup_printf ("%2.2f MiB", (gfloat) size / 1024);
	if ((size / 1024) > 1024)
		return g_strdup_printf ("%2.2f GiB", (gfloat) size / (1024 * 1024));
}

gfloat 
gst_storage_get_float_size (const gulong size)
{
	if (size == 0)
		return 0.0;
	if ((size / 1024) <= 1024)
		return ((gfloat) size / 1024);
	if ((size / 1024) > 1024)
		return ((gfloat) size / (1024 * 1024));
}

GdkPixbuf *
gst_storage_get_icon (const gchar *icon_name)
{
	GdkPixbuf *pb;
	gchar *file;
	
	file = gnome_icon_theme_lookup_icon (icon_theme,
					     icon_name, 48,
					     NULL, NULL);
	
	pb = gdk_pixbuf_new_from_file (file, NULL);
	
	g_free (file);
	
	return pb;
}

static GtkWidget *
gst_storage_list_new ()
{
	GtkTreeModel *model;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	model = GTK_TREE_MODEL (gtk_tree_store_new (STORAGE_LIST_LAST,
						    GDK_TYPE_PIXBUF,
						    G_TYPE_STRING,
						    G_TYPE_POINTER));
	
	treeview = gst_dialog_get_widget (tool->main_dialog, "storage_list");
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (model);

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "pixbuf",
							   STORAGE_LIST_ICON,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, STORAGE_LIST_ICON);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "markup",
							   STORAGE_LIST_NAME,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, STORAGE_LIST_NAME);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (gst_on_storage_list_selection_change),
			  NULL);

	return (treeview);
}

static void
gst_storage_list_set (GtkWidget *treeview, GList *storages)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *list;
	GstDisksStorage *dsk;
	gint i;
	gchar *icon, *name;
	gulong size;

	g_return_if_fail (treeview != NULL);
	g_return_if_fail (storages != NULL);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	list = g_list_first (storages);
	while (list) {
		dsk = list->data;
		if (GST_IS_DISKS_STORAGE (dsk)) {
			g_object_get (G_OBJECT (dsk), "icon_name", &icon,
				      "size", &size, "name", &name,
				      NULL);

			gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
			gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
					    STORAGE_LIST_ICON,
					    gst_storage_get_icon (icon),
					    STORAGE_LIST_NAME,
					    g_strdup_printf ("<b>%s</b>\n%s", name,
							     gst_storage_get_human_readable_size (size)),
					    STORAGE_LIST_POINTER, dsk,
					    -1);
		}
		list = g_list_next (list);
	}

	gtk_tree_model_get_iter_first (model, &iter);
	gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
					&iter);
}

static void
gst_storage_list_reload (GtkWidget *widget, gpointer gdata)
{
	gst_storage_list_set (gst_dialog_get_widget (tool->main_dialog, "storage_list"),
			      (GList *) gdata);
}

static GtkWidget *
gst_partition_list_new ()
{
	GtkTreeModel *model;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;
	/*GtkItemFactory *item_factory;*/ /* Uncoment for 0.2 */

	model = GTK_TREE_MODEL (gtk_tree_store_new (PARTITION_LIST_LAST,
						    G_TYPE_STRING,
						    G_TYPE_POINTER));

	treeview = gst_dialog_get_widget (tool->main_dialog, "partition_list");
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (model);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "text",
							   PARTITION_LIST_NAME,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, PARTITION_LIST_NAME);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (gst_on_partition_list_selection_change),
			  NULL);

	/* Uncoment for 0.2 */
	/*item_factory = gst_disks_partition_popup_item_factory_create (treeview);
	g_signal_connect (G_OBJECT (treeview), "button_press_event",
			  G_CALLBACK (gst_on_partition_list_button_press),
			  (gpointer) item_factory);*/

	return (treeview);
}


void 
gst_storage_partition_gui_setup (GtkWidget *treeview, GList *partitions)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *list;
	GstDisksStoragePartition *part;
	gint i;
	gchar *device, *name;
	GstPartitionType type;

	g_return_if_fail (treeview != NULL);
	g_return_if_fail (partitions != NULL);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	list = g_list_first (partitions);
	while (list) {
		part = list->data;
		if (GST_IS_DISKS_STORAGE_PARTITION (part)) {
			g_object_get (G_OBJECT (part),
				      "device", &device,
				      "name", &name,
				      "type", &type, 
				      NULL);
			if (type != PARTITION_TYPE_FREE) {
				gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
				gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						    PARTITION_LIST_NAME,
						    g_strdup (name),
						    PARTITION_LIST_POINTER, part,
						    -1);
			}
			
			if (device) g_free (device);
		}
		list = g_list_next (list);
	}
	
}

void
gst_disks_gui_set_device_speed (GstDisksStorage *storage)
{
	gst_tool_queue_directive (tool, gst_disks_storage_get_device_speed_cb,
				  (gpointer) storage, NULL, NULL, "dev_speed");
}


void
gst_storage_gui_setup (GstDisksConfig *cfg)
{
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *list;

	g_return_if_fail (cfg != NULL);

	list = cfg->storages;

	icon_theme = gnome_icon_theme_new ();
	gnome_icon_theme_set_allow_svg (icon_theme, TRUE);
	g_signal_connect (G_OBJECT (icon_theme), "changed",
			  G_CALLBACK (gst_storage_list_reload),
			  (gpointer) list);
	
	treeview = gst_partition_list_new ();

	treeview = gst_storage_list_new ();

	gst_storage_list_set (treeview, list);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_model_get_iter_first (model, &iter);
	gtk_tree_selection_select_iter (selection, &iter);
	
	gtk_widget_show_all (treeview);

}

