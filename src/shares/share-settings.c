/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-settings.h: this file is part of shares-admin, a gnome-system-tool frontend 
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

#include <gtk/gtk.h>
#include "share-settings.h"
#include "share-export-smb.h"
#include "share-export-nfs.h"
#include "nfs-acl-table.h"
#include "table.h"
#include "gst.h"

extern GstTool *tool;

static void
share_settings_clear_dialog (void)
{
	GtkWidget *widget;
	GtkTreeModel *model;

	/* SMB widgets */
	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
	gtk_entry_set_text (GTK_ENTRY (widget), "");

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_comment");
	gtk_entry_set_text (GTK_ENTRY (widget), "");

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_browsable");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_readonly");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

	/* NFS widgets */
	widget = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
	model  = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	gtk_list_store_clear (GTK_LIST_STORE (model));
}

GtkTreeModel*
share_settings_get_combo_model (void)
{
	GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

	return GTK_TREE_MODEL (store);
}

void
share_settings_create_combo (void)
{
	GtkWidget    *combo  = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkTreeModel *model;

	model = share_settings_get_combo_model ();
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
	g_object_unref (model);
}

static void
share_settings_set_path (const gchar *path)
{
	GtkWidget *path_entry        = gst_dialog_get_widget (tool->main_dialog, "share_path");
	GtkWidget *path_entry_label  = gst_dialog_get_widget (tool->main_dialog, "share_path_fentry_label");
	GtkWidget *path_label        = gst_dialog_get_widget (tool->main_dialog, "share_path_label");
	GtkWidget *path_label_label  = gst_dialog_get_widget (tool->main_dialog, "share_path_label_label");

	if (!path) {
		gtk_widget_show (path_entry);
		gtk_widget_show (path_entry_label);
		gtk_widget_hide (path_label);
		gtk_widget_hide (path_label_label);
	} else {
		gtk_widget_hide (path_entry);
		gtk_widget_hide (path_entry_label);
		gtk_widget_show (path_label);
		gtk_widget_show (path_label_label);
		gtk_label_set_text (GTK_LABEL (path_label), path);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (path_entry), path);
	}
}

static GtkWidget*
share_settings_prepare_dialog (const gchar *path, gboolean standalone)
{
	GtkWidget    *combo       = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkWidget    *dialog      = gst_dialog_get_widget (tool->main_dialog, "share_properties");
	GtkTreeModel *model;
	GtkTreeIter   iter;
	xmlNodePtr    root;

	share_settings_clear_dialog ();
	share_settings_set_path (path);

	root = gst_xml_doc_get_root (tool->config);
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	if (standalone) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, _("Do not share"),
				    1, SHARE_DONT_SHARE,
				    -1);
	}

	if (gst_xml_element_get_boolean (root, "smbinstalled")) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, _("SMB"),
				    1, SHARE_THROUGH_SMB,
				    -1);
	}

	if (gst_xml_element_get_boolean (root, "nfsinstalled")) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, _("NFS"),
				    1, SHARE_THROUGH_NFS,
				    -1);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

	return dialog;
}

static void
share_settings_set_share_smb (GstShareSMB *share)
{
	GtkWidget *widget;
	gint       flags;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
					     gst_share_get_path (GST_SHARE (share)));

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
	gtk_entry_set_text (GTK_ENTRY (widget),
			    gst_share_smb_get_name (share));
	
	widget  = gst_dialog_get_widget (tool->main_dialog, "share_smb_comment");
	gtk_entry_set_text (GTK_ENTRY (widget),
			    gst_share_smb_get_comment (share));

	flags = gst_share_smb_get_flags (share);

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_browsable");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				      flags & GST_SHARE_SMB_BROWSABLE);

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_readonly");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				      ! (flags & GST_SHARE_SMB_WRITABLE));
}

static void
share_settings_set_share_nfs (GstShareNFS *share)
{
	GtkWidget    *widget;
	const GSList *list;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
					     gst_share_get_path (GST_SHARE (share)));

	list = gst_share_nfs_get_acl (share);

	while (list) {
		nfs_acl_table_add_element (list->data);
		list = g_slist_next (list);
	}
}

static void
share_settings_set_active (gint val)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      valid;
	gint          value;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 1, &value, -1);

		if (value == val) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
			valid = FALSE;
		} else
			valid = gtk_tree_model_iter_next (model, &iter);
	}
}

static void
share_settings_set_share (GstShare *share)
{
	if (GST_IS_SHARE_SMB (share)) {
		share_settings_set_active (SHARE_THROUGH_SMB);
		share_settings_set_share_smb (GST_SHARE_SMB (share));
	} else if (GST_IS_SHARE_NFS (share)) {
		share_settings_set_active (SHARE_THROUGH_NFS);
		share_settings_set_share_nfs (GST_SHARE_NFS (share));
	}
}

static void
share_settings_close_dialog (void)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "share_properties");

	gtk_widget_hide (dialog);
}

static GstShare*
share_settings_get_share_smb (void)
{
	GtkWidget   *widget;
	const gchar *path, *name, *comment;
	gint         flags = 0;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	path    = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
	name    = gtk_entry_get_text (GTK_ENTRY (widget));
	
	widget  = gst_dialog_get_widget (tool->main_dialog, "share_smb_comment");
	comment = gtk_entry_get_text (GTK_ENTRY (widget));

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_browsable");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		flags |= GST_SHARE_SMB_BROWSABLE;

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_readonly");
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		flags |= GST_SHARE_SMB_WRITABLE;

	flags |= GST_SHARE_SMB_ENABLED;
	flags |= GST_SHARE_SMB_PUBLIC;

	return GST_SHARE (gst_share_smb_new (name, comment, path, flags));
}

static GstShare*
share_settings_get_share_nfs ()
{
	GtkWidget   *widget;
	const gchar *path;
	GstShareNFS *share;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	path    = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));

	share = gst_share_nfs_new (path);
	nfs_acl_table_insert_elements (share);

	return GST_SHARE (share);
}

static GstShare*
share_settings_get_share (void)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          selected;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
		return NULL;

	gtk_tree_model_get (model, &iter, 1, &selected, -1);

	if (selected == SHARE_THROUGH_SMB)
		return share_settings_get_share_smb ();
	else if (selected == SHARE_THROUGH_NFS)
		return share_settings_get_share_nfs ();
	else
		return NULL;
}

gboolean
share_settings_validate (void)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkWidget    *widget;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          selected;
	const gchar  *text;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter, 1, &selected, -1);

	if (selected == SHARE_THROUGH_SMB) {
		widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
		text   = gtk_entry_get_text (GTK_ENTRY (widget));
		
		return (text && *text);
	}

	/* in any other case, it's valid */
	return TRUE;
}

void
share_settings_dialog_run (const gchar *path, gboolean standalone)
{
	GtkWidget   *dialog;
	gint         response;
	GstShare    *new_share, *share;
	GtkTreeIter  iter;
	gboolean     path_exists;

	share  = NULL;
	dialog = share_settings_prepare_dialog (path, standalone);

	/* check whether the path already exists */
	path_exists = table_get_iter_with_path (path, &iter);

	if (path_exists) {
		share = table_get_share_at_iter (&iter);
		share_settings_set_share (share);
		g_object_unref (share);
	}

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	while ((response = gtk_dialog_run (GTK_DIALOG (dialog))) == GTK_RESPONSE_HELP);
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) {
		new_share = share_settings_get_share ();

		if (path_exists && new_share)
			table_modify_share_at_iter (&iter, new_share);
		else if (!path_exists && new_share)
			table_add_share (new_share);
		else if (path_exists && !new_share)
			table_delete_share_at_iter (&iter);

		gst_dialog_modify (tool->main_dialog);

		if (standalone)
			gtk_signal_emit_by_name (GTK_OBJECT (tool->main_dialog),
						 "apply", tool);
	}

	share_settings_close_dialog ();
}

void
smb_settings_prepare_dialog (void)
{
	GtkWidget *widget;
	gchar     *desc, *workgroup, *wins_server;
	gboolean   wins_use;
	xmlNodePtr root;

	root = gst_xml_doc_get_root (tool->config);
	desc        = gst_xml_get_child_content (root, "smbdesc");
	workgroup   = gst_xml_get_child_content (root, "workgroup");
	wins_server = gst_xml_get_child_content (root, "smb_wins_server");
	wins_use    = gst_xml_element_get_boolean (root, "winsuse");
	
	widget = gst_dialog_get_widget (tool->main_dialog, "smb_description");
	gtk_entry_set_text (GTK_ENTRY (widget), desc);

	widget = gst_dialog_get_widget (tool->main_dialog, "smb_workgroup");
	gtk_entry_set_text (GTK_ENTRY (widget), workgroup);

	if (wins_server) {
		widget = gst_dialog_get_widget (tool->main_dialog, "smb_wins_server");
		gtk_entry_set_text (GTK_ENTRY (widget), wins_server);

		widget = gst_dialog_get_widget (tool->main_dialog, "smb_use_wins");
	} else if (wins_use)
		widget = gst_dialog_get_widget (tool->main_dialog, "smb_is_wins");
	else
		widget = gst_dialog_get_widget (tool->main_dialog, "smb_no_wins");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

	g_free (desc);
	g_free (workgroup);
	g_free (wins_server);
}

void
smb_settings_save (void)
{
	GtkWidget   *widget;
	const gchar *text;
	gboolean    *active;
	xmlNodePtr   root;

	root = gst_xml_doc_get_root (tool->config);

	widget = gst_dialog_get_widget (tool->main_dialog, "smb_description");
	text = gtk_entry_get_text (GTK_ENTRY (widget));
	gst_xml_set_child_content (root, "smbdesc", text);

	widget = gst_dialog_get_widget (tool->main_dialog, "smb_workgroup");
	text = gtk_entry_get_text (GTK_ENTRY (widget));
	gst_xml_set_child_content (root, "workgroup", text);

	widget = gst_dialog_get_widget (tool->main_dialog, "smb_no_wins");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		gst_xml_element_destroy_children_by_name (root, "smb_wins_server");
		gst_xml_element_set_boolean (root, "winsuse", FALSE);
	} else {
		widget = gst_dialog_get_widget (tool->main_dialog, "smb_is_wins");

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
			gst_xml_element_destroy_children_by_name (root, "smb_wins_server");
			gst_xml_element_set_boolean (root, "winsuse", TRUE);
		} else {
			widget = gst_dialog_get_widget (tool->main_dialog, "smb_wins_server");
			text   = gtk_entry_get_text (GTK_ENTRY (widget));

			gst_xml_set_child_content (root, "smb_wins_server", text);
			gst_xml_element_set_boolean (root, "winsuse", FALSE);
		}
	}
}
