/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of shares-admin, a gnome-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>.
 */

#include <gtk/gtktreeview.h>
#include "gst.h"
#include "table.h"
#include "share-export-smb.h"
#include "share-export-nfs.h"

extern GstTool *tool;
GtkIconTheme *icon_theme;

static GtkTreeModel*
create_table_model (void)
{
	GtkListStore *store;

	store = gtk_list_store_new (COL_LAST,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_POINTER);

	return GTK_TREE_MODEL (store);
}

static void
add_table_columns (GtkTreeView *table)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Share"));
	   
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
					     renderer,
					     "pixbuf", 0,
					     NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,
					     renderer,
					     "text", 1,
					     NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (table), column);

	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), 1,
						     _("Path"),
						     renderer,
						     "text", 2,
						     NULL);
}

void
table_create (void)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;

	model = create_table_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (table), model);
	g_object_unref (G_OBJECT (model));

	add_table_columns (GTK_TREE_VIEW (table));

	icon_theme = gtk_icon_theme_get_default ();
}

void
table_add_node (xmlNodePtr node)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *share_type;
	GstShare     *share = NULL;
	GdkPixbuf    *pixbuf = NULL;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	share_type = gst_xml_element_get_attribute (node, "type");

	if (strcmp (share_type, "smb") == 0) {
		share  = GST_SHARE (gst_share_smb_new_from_xml (node));
		pixbuf = gtk_icon_theme_load_icon (icon_theme,
						   "gnome-fs-smb",
						   48, 0, NULL);
	} else if (strcmp (share_type, "nfs") == 0) {
		share  = GST_SHARE (gst_share_nfs_new_from_xml (node));
		pixbuf = gtk_icon_theme_load_icon (icon_theme,
						   "gnome-fs-nfs",
						   48, 0, NULL);
	}

	if (share) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model),
				    &iter,
				    0, pixbuf,
				    1, gst_share_get_name (share),
				    2, gst_share_get_path (share),
				    3, share,
				    -1);
	}

	if (pixbuf)
		g_object_unref (pixbuf);
	g_free (share_type);
}
