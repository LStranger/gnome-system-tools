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
#include "gst.h"

extern GstTool *tool;

static void
share_settings_clear_dialog (void)
{
	GtkWidget *widget;
	GtkTreeModel *model;

	/* common widgets */
	widget = gst_dialog_get_widget (tool->main_dialog, "share_path");
	gtk_entry_set_text (GTK_ENTRY (widget), "");

	widget = gst_dialog_get_widget (tool->main_dialog, "share_type");
	gtk_widget_show (widget);
	
	widget = gst_dialog_get_widget (tool->main_dialog, "share_type_label");
	gtk_widget_show (widget);

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

GtkWidget*
share_settings_prepare_dialog (void)
{
	GtkWidget    *combo  = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkWidget    *dialog = gst_dialog_get_widget (tool->main_dialog, "share_properties");
	GtkTreeModel *model;

	share_settings_clear_dialog ();

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("SMB"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("NFS"));

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

	return dialog;
}

static void
share_settings_set_share_smb (GstShareSMB *share)
{
	GtkWidget *widget;
	gint       flags;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	gtk_entry_set_text (GTK_ENTRY (widget),
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

void
share_settings_set_share_nfs (GstShareNFS *share)
{
	GtkWidget    *widget;
	const GSList *list;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	gtk_entry_set_text (GTK_ENTRY (widget),
			    gst_share_get_path (GST_SHARE (share)));

	list = gst_share_nfs_get_acl (share);

	while (list) {
		nfs_acl_table_add_element (list->data);
		list = g_slist_next (list);
	}
}

void
share_settings_set_share (GstShare *share)
{
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkWidget *label = gst_dialog_get_widget (tool->main_dialog, "share_type_label");

	if (GST_IS_SHARE_SMB (share)) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
		share_settings_set_share_smb (GST_SHARE_SMB (share));
	} else if (GST_IS_SHARE_NFS (share)) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);
		share_settings_set_share_nfs (GST_SHARE_NFS (share));
	}

	gtk_widget_hide (combo);
	gtk_widget_hide (label);
}

void
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
	path    = gtk_entry_get_text (GTK_ENTRY (widget));

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

GstShare*
share_settings_get_share_nfs ()
{
	GtkWidget   *widget;
	const gchar *path;
	GSList      *list;
	GstShareNFS *share;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	path    = gtk_entry_get_text (GTK_ENTRY (widget));

	share = gst_share_nfs_new (path);
	nfs_acl_table_insert_elements (share);

	return GST_SHARE (share);
}

GstShare*
share_settings_get_share (void)
{
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GstShare  *share;
	gint       selected;

	selected = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
	
	if (selected == 0) {
		return share_settings_get_share_smb ();
	} else {
		return share_settings_get_share_nfs ();
	}
}
