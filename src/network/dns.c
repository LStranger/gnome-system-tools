/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Carlos García Campos <elkalmail@yahoo.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>

#include "gst.h"

#include "callbacks.h"
#include "transfer.h"
#include "dns.h"

extern GstTool *tool;

GtkActionEntry dns_search_popup_menu_items [] = {
	{ "Delete", GTK_STOCK_DELETE, N_("_Delete"), NULL, NULL, G_CALLBACK (on_dns_search_popup_del_activate) },
};

const gchar *dns_search_ui_description =
	"<ui>"
	"  <popup name='MainMenu'>"
	"    <menuitem action='Delete'/>"
	"  </popup>"
	"</ui>";

gboolean 
gst_dns_search_is_in_list (GtkWidget *list, const gchar *ip_str)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gboolean         valid;
	gchar           *buf;

	g_return_val_if_fail (list != NULL, TRUE);
	
	if (!ip_str || strlen (ip_str) == 0)
		return TRUE;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, DNS_SEARCH_LIST_COL, &buf, -1);

		if (strcmp (buf, ip_str) == 0) {
			g_free (buf);
			return TRUE;
		}

		g_free (buf);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	return FALSE;
}

void
gst_dns_search_update_sensitivity (GtkWidget *list)
{
	GtkWidget *add_button;
	GtkWidget *del_button;
	GtkWidget *entry;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	gboolean add;

	g_return_if_fail (list != NULL);
	
	if (strcmp (gtk_widget_get_name (list), "dns_list") == 0) {
		add_button = gst_dialog_get_widget (tool->main_dialog,
						    "dns_server_add_button");
		del_button = gst_dialog_get_widget (tool->main_dialog,
						    "dns_server_del_button");
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "dns_server_entry");
	}
	else if (strcmp (gtk_widget_get_name (list), "search_list") == 0) {
		add_button = gst_dialog_get_widget (tool->main_dialog,
						    "search_domain_add_button");
		del_button = gst_dialog_get_widget (tool->main_dialog,
						    "search_domain_del_button");
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "search_domain_entry");
	}


	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));

        /* Del button */
	gtk_widget_set_sensitive (del_button,
				  gtk_tree_selection_get_selected (selection,
								   &model, NULL));

	/* Add button */
	add = ((strlen (gtk_entry_get_text (GTK_ENTRY (entry))) > 0) &&
	       (!gtk_tree_selection_get_selected (selection, &model, NULL)));
	gtk_widget_set_sensitive (add_button, add);

}

static GtkTreeModel *
dns_search_list_model_new (void)
{
	GtkListStore *store;

	store = gtk_list_store_new (DNS_SEARCH_LIST_COL_LAST,
				    G_TYPE_STRING);

	return GTK_TREE_MODEL (store);
}

static void
dns_search_list_add_columns (GtkTreeView *treeview)
{
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	GtkTreeModel      *model;

	g_return_if_fail (treeview != NULL);

	model = gtk_tree_view_get_model (treeview);

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("IP Address"), cell,
							"text", DNS_SEARCH_LIST_COL, NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_sort_column_id (col, 0);
	gtk_tree_view_append_column (treeview, col);

}

static void
dns_search_list_select_row (GtkTreeSelection *selection, gpointer gdata)
{
	GtkTreeView     *treeview;
	GtkTreeIter      iter;
	GtkTreeModel    *model;
	gchar           *buf;
	gint             pos;
	GtkWidget       *entry;

	treeview = (GtkTreeView *) gdata;

	if (strcmp (gtk_widget_get_name (GTK_WIDGET (treeview)),
		    "dns_list") == 0)
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "dns_server_entry");
	else if (strcmp (gtk_widget_get_name (GTK_WIDGET (treeview)),
			 "search_list") == 0)
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "search_domain_entry");

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		/* Selection exists */
		gtk_tree_model_get (model, &iter, DNS_SEARCH_LIST_COL, &buf, -1);
		
		gtk_entry_set_text (GTK_ENTRY (entry), buf);

		g_free (buf);

	} 

	gst_dns_search_update_sensitivity (GTK_WIDGET (treeview));
}

static void
dns_search_gui_setup (GstTool *tool, const gchar *listname, const gchar *entryname)
{
	GtkWidget        *treeview;
	GtkTreeSelection *select;
	GtkTreeModel     *model;
	GtkWidget        *entry, *popup;
	GtkTargetEntry   target = { "dns", GTK_TARGET_SAME_WIDGET, 0 };

	g_return_if_fail (tool != NULL);

	model = dns_search_list_model_new ();

	treeview = gst_dialog_get_widget (tool->main_dialog, listname);
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (G_OBJECT (model));

	dns_search_list_add_columns (GTK_TREE_VIEW (treeview));

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (dns_search_list_select_row),
			  (gpointer) treeview);
	
	gtk_widget_show_all (treeview);

	entry = gst_dialog_get_widget (tool->main_dialog, entryname);
	g_signal_connect (G_OBJECT (entry), "changed",
			  G_CALLBACK (on_dns_search_entry_changed),
			  (gpointer) treeview);

	popup = create_popup_menu (treeview,
				   dns_search_popup_menu_items,
				   G_N_ELEMENTS (dns_search_popup_menu_items),
				   dns_search_ui_description);

	g_signal_connect (G_OBJECT (treeview), "button_press_event",
			  G_CALLBACK (callbacks_button_press),
			  (gpointer) popup);
	g_signal_connect (G_OBJECT (treeview), "drag-data-get",
			  G_CALLBACK (on_drag_data_get),
			  NULL);

	g_signal_connect (G_OBJECT (treeview), "drag_data_received",
			  G_CALLBACK (on_drag_data_received),
			  NULL);

	gst_dns_search_update_sensitivity (treeview);

	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (treeview),
						GDK_BUTTON1_MASK,
						&target,
						1,
						GDK_ACTION_MOVE);

	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
					      &target,
					      1,
					      GDK_ACTION_MOVE);
}

void
dns_search_init_gui (GstTool *tool)
{
	g_return_if_fail (tool != NULL);
	
	dns_search_gui_setup (tool, "dns_list", "dns_server_entry");
	dns_search_gui_setup (tool, "search_list", "search_domain_entry");
}

void
dns_search_list_append (GtkWidget *treeview, const gchar *text)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;

	g_return_if_fail (treeview != NULL);

	if ((!text) || (strlen (text) == 0))
		return;

	if (gst_dns_search_is_in_list (treeview, text))
		return;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    DNS_SEARCH_LIST_COL,
			    g_strdup (text),
			    -1);
	gtk_tree_selection_select_iter (gtk_tree_view_get_selection (
						GTK_TREE_VIEW (treeview)),
					&iter);
	
	gst_dns_search_update_sensitivity (treeview);
	
	return;
}

void
dns_search_list_remove (GtkWidget *treeview, GtkWidget *entry)
{
	GtkTreeModel      *model;
	GtkTreeIter        iter;
	GtkTreeSelection  *selection;
	gchar             *buf;
	gboolean           valid;

	g_return_if_fail (treeview != NULL);
	g_return_if_fail (entry != NULL);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_tree_model_get (model, &iter, DNS_SEARCH_LIST_COL, &buf, -1);
		if (gtk_tree_selection_iter_is_selected (selection, &iter))
		{
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			gtk_entry_set_text (GTK_ENTRY (entry), "");
			break;
		}

		valid = gtk_tree_model_iter_next (model, &iter);
	}

	gst_dns_search_update_sensitivity (treeview);
}

