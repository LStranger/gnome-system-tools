/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of shares-admin, a gnome-system-tool frontend 
 * for shared folders administration.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gst.h"
#include "gst-hig-dialog.h"
#include "table.h"
#include "callbacks.h"
#include "share-settings.h"
#include "share-nfs-add-hosts.h"

extern GstTool *tool;

void
on_shares_table_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GtkWidget *settings_button, *delete_button;
	gboolean   active;

	settings_button = gst_dialog_get_widget (tool->main_dialog, "edit_share");
	delete_button   = gst_dialog_get_widget (tool->main_dialog, "delete_share");

	active = (gtk_tree_selection_count_selected_rows (selection) > 0);

	gtk_widget_set_sensitive (settings_button, active);
	gtk_widget_set_sensitive (delete_button, active);
}

static void
do_popup_menu (GtkWidget *popup, GdkEventButton *event)
{
	gint button, event_time;

	if (!popup)
		return;

	if (event) {
		button     = event->button;
		event_time = event->time;
	} else {
		button     = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

gboolean
on_shares_table_button_press (GtkWidget *table, GdkEventButton *event, gpointer data)
{
	GtkTreePath      *path;
	GtkWidget        *popup;
	GtkTreeSelection *selection;

	popup = (GtkWidget *) data;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (event->button == 3) {
		gtk_widget_grab_focus (table);

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (table),
						   event->x, event->y,
						   &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);

			do_popup_menu (popup, event);
		}

		return TRUE;
	}

	if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
		on_edit_share_clicked (NULL, NULL);
		return TRUE;
	}

	return FALSE;
}

gboolean
on_shares_table_popup_menu (GtkWidget *widget, GtkWidget *popup)
{
	do_popup_menu (popup, NULL);
	return TRUE;
}

void
on_add_share_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	gint       response;
	GstShare  *share;

	dialog = share_settings_prepare_dialog ();

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	while ((response = gtk_dialog_run (GTK_DIALOG (dialog))) == GTK_RESPONSE_HELP);
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) {
		share = share_settings_get_share ();
		table_add_share (share);
		gst_dialog_modify (tool->main_dialog);
	}

	share_settings_close_dialog ();
}

void
on_edit_share_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget        *dialog, *table;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GstShare         *share;
	gint              response;

	table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, COL_POINTER, &share, -1);

	dialog = share_settings_prepare_dialog ();
	share_settings_set_share (share);
	g_object_unref (share);
		
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	while ((response = gtk_dialog_run (GTK_DIALOG (dialog))) == GTK_RESPONSE_HELP);
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) {
		share = share_settings_get_share ();
		table_modify_share_at_iter (iter, share);
		gst_dialog_modify (tool->main_dialog);
	}

	share_settings_close_dialog ();
}

void
on_delete_share_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget        *table, *dialog;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gint              response;

	table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		dialog = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     GST_HIG_MESSAGE_WARNING,
					     _("Are you sure you want to delete this share?"),
					     _("Other computers in your network will stop viewing this"),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
					     NULL);
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (response == GTK_RESPONSE_ACCEPT) {
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			gst_dialog_modify (tool->main_dialog);
		}
	}
}

void
on_share_type_changed (GtkWidget *widget, gpointer data)
{
	gint selected;
	GtkWidget *smb_frame, *nfs_frame;

	selected  = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	smb_frame = gst_dialog_get_widget (tool->main_dialog, "smb_properties");
	nfs_frame = gst_dialog_get_widget (tool->main_dialog, "nfs_properties");

	if (selected == 0) {
		gtk_widget_show (smb_frame);
		gtk_widget_hide (nfs_frame);
	} else {
		gtk_widget_hide (smb_frame);
		gtk_widget_show (nfs_frame);
	}
}

void
on_share_nfs_delete_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget        *table;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	GtkTreeModel     *model;
	
	table = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

void
on_share_nfs_add_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget    *parent = gst_dialog_get_widget (tool->main_dialog, "share_properties");
	GtkWidget    *dialog;
	GtkWidget    *table;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          response;
	gchar        *host = NULL;
	gboolean      read_only;

	dialog = share_nfs_add_hosts_dialog_prepare ();

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) {
		if (!share_nfs_add_hosts_dialog_get_data (&host, &read_only))
			return;

		table = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, host,
				    1, read_only,
				    -1);
		g_free (host);
	}
}

/* helper for setting visibility */
static void
widget_set_visibility (GtkWidget *w, gboolean visible)
{
	if (visible)
		gtk_widget_show (w);
	else
		gtk_widget_hide (w);
}

void
on_share_nfs_host_type_changed (GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          selected_type;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
		gtk_tree_model_get (model, &iter,
				    NFS_HOST_COL_TYPE, &selected_type, -1);

		widget_set_visibility (gst_dialog_get_widget (tool->main_dialog, "share_nfs_hostname_box"),
				       (selected_type == NFS_SHARE_HOSTNAME));

		widget_set_visibility (gst_dialog_get_widget (tool->main_dialog, "share_nfs_address_box"),
				       (selected_type == NFS_SHARE_ADDRESS));

		widget_set_visibility (gst_dialog_get_widget (tool->main_dialog, "share_nfs_network_box"),
				       (selected_type == NFS_SHARE_NETWORK));
	}
}
