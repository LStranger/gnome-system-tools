/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for boot administration.
 * 
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
 * Authors: Tambet Ingo <tambet@ximian.com>,
 *          Carlos Garnacho Parro <garnacho@tuxerver.net>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>

#include "gst.h"

#include "table.h"
#include "callbacks.h"

extern GstTool *tool;

GtkWidget *boot_table = NULL;
GtkTreeIter default_entry_iter;

gchar *boot_table_columns [] = {
	N_("Name"),
	N_("Default"),
	N_("Type"),
	N_("Kernel Image"),
	N_("Device"),
	NULL
};

GtkActionEntry popup_menu_items [] = {
	{ "Add",        GTK_STOCK_ADD,        N_("_Add"),        NULL, NULL, G_CALLBACK (on_popup_add_activate)      },
	{ "Properties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_popup_settings_activate) },
	{ "Delete",     GTK_STOCK_DELETE,     N_("_Delete"),     NULL, NULL, G_CALLBACK (on_popup_delete_activate)   },
};

const gchar *ui_description =
	"<ui>"
	"  <popup name='MainMenu'>"
	"    <menuitem action='Add'/>"
	"    <separator/>"
	"    <menuitem action='Properties'/>"
	"    <menuitem action='Delete'/>"
	"  </popup>"
	"</ui>";

static GtkWidget*
boot_popup_menu_create (GtkTreeView *treeview)
{
	GtkUIManager   *ui_manager;
        GtkActionGroup *action_group;
        GtkWidget      *popup;

        g_return_val_if_fail (treeview != NULL, NULL);

        action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
        gtk_action_group_add_actions (action_group,
                                      popup_menu_items,
                                      G_N_ELEMENTS (popup_menu_items), treeview);

        ui_manager = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

        if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
                return NULL;

        popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

        return popup;
}

void
table_create (void)
{
	GtkTreeModel      *model;
	GtkCellRenderer   *renderer;
	GtkTreeSelection  *selection;
	GtkTreeViewColumn *column;
	GtkWidget         *popup_menu;
	gint i;

	model = GTK_TREE_MODEL (gtk_tree_store_new (BOOT_LIST_COL_LAST, G_TYPE_STRING, G_TYPE_BOOLEAN,
                                                    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER));

	boot_table = gst_dialog_get_widget (tool->main_dialog, "boot_table");
	gtk_tree_view_set_model (GTK_TREE_VIEW (boot_table), model);
	g_object_unref (model);

	for (i = 0; i < BOOT_LIST_COL_LAST - 1; i++) {
		if (i == BOOT_LIST_COL_DEFAULT) {
			renderer = gtk_cell_renderer_toggle_new ();
			gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);

			/* We add the signal that will change the default option */
			g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (on_boot_table_default_toggled), boot_table);
			column = gtk_tree_view_column_new_with_attributes (_(boot_table_columns [i]),
									   renderer,
									   "active", i,
									   NULL);
		} else {
			renderer = gtk_cell_renderer_text_new ();

			column = gtk_tree_view_column_new_with_attributes (_(boot_table_columns [i]),
									   renderer,
									   "text", i,
									   NULL);
			gtk_tree_view_column_set_resizable (column, TRUE);
			gtk_tree_view_column_set_sort_column_id (column, i);
		}

		gtk_tree_view_insert_column (GTK_TREE_VIEW (boot_table), column, i);
	}
		
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (boot_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_boot_table_cursor_changed),
			  NULL);

	popup_menu = boot_popup_menu_create (GTK_TREE_VIEW (boot_table));
	
	g_signal_connect (G_OBJECT (boot_table), "button_press_event",
			  G_CALLBACK (on_boot_table_button_press),
			  (gpointer) popup_menu);
}

static gboolean
boot_is_linux (xmlNodePtr node)
{
	xmlNodePtr n;
	
	g_return_val_if_fail (node != NULL, FALSE);

	n = gst_xml_element_find_first (node, "image");
	if (n)
		return TRUE;
	else
		return FALSE;
}

gchar *
boot_value_label (xmlNodePtr node)
{
	g_return_val_if_fail (node != NULL, NULL);

	return gst_xml_get_child_content (node, "label");
}

gboolean
boot_value_default (xmlNodePtr node)
{
	gchar *name = NULL;
	gchar *def = NULL;
	gboolean value = FALSE;
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	
	def = gst_xml_get_child_content (root, "default");
	name = gst_xml_get_child_content (node, "label");

	if (def && name)
		value = (strcmp (name, def) == 0);

	g_free (def);
	g_free (name);

	return value;
}

GstBootImageType
boot_value_type (xmlNodePtr node)
{
	xmlNodePtr n;
	GstBootImageType type = TYPE_UNKNOWN;
	
	g_return_val_if_fail (node != NULL, type);

	n = gst_xml_element_find_first (node, "type");
	if (n) {
		gchar *buf;
		
		buf = gst_xml_element_get_content (n);
		type =  label_to_type (buf);
		g_free (buf);
		return type;
	}

	n = gst_xml_element_find_first (node, "image");
	if (n)
		type = TYPE_LINUX;

	n = gst_xml_element_find_first (node, "module");
	if (n)
		type = TYPE_HURD;
	
	return type;
}

gchar *
boot_value_type_char (xmlNodePtr node, gboolean bare)
{
	GstBootImageType type;
	gchar *buf, *label;

	type = boot_value_type (node);
	buf = type_to_label (type);

	return buf;
}

void *
boot_value_device (xmlNodePtr node, gboolean bare)
{
	gchar *buf, *device;

	g_return_val_if_fail (node != NULL, NULL);
	
	if (boot_is_linux (node))
		buf = gst_xml_get_child_content (node, "root");
	else
		buf = gst_xml_get_child_content (node, "other");

	if (buf == NULL)
		buf = g_strdup ("");

	if (bare)
		device = buf;
	else {
		device = g_strdup_printf ("  %s", buf);
		g_free (buf);
	}

	return device;
}			

void *
boot_value_image (xmlNodePtr node, gboolean bare)
{
	gchar *buf, *image;
	
	g_return_val_if_fail (node != NULL, NULL);

	if (boot_is_linux (node))
		buf = gst_xml_get_child_content (node, "image");
	else
		buf = g_strdup ("");

	if (bare)
		image = buf;
	else {
		image = g_strdup_printf ("  %s", buf);
		g_free (buf);
	}

	return image;
}

void *
boot_value_root (xmlNodePtr node)
{
	g_return_val_if_fail (node != NULL, NULL);

	return gst_xml_get_child_content (node, "root");
}

gchar *
boot_value_append (xmlNodePtr node)
{
	gchar *buf;
	gchar ** str_array;
	
	g_return_val_if_fail (node != NULL, NULL);

	buf = gst_xml_get_child_content (node, "append");
	if (!buf)
		return NULL;

	str_array = g_strsplit (buf, "\"", 10); /* FIXME: 10 = max_tokens */
	buf = g_strjoinv (NULL, str_array);
	g_strfreev (str_array);

	return buf;
}

gchar *
boot_value_initrd (xmlNodePtr node)
{
	gchar *buf;
	
	g_return_val_if_fail (node != NULL, NULL);

	if (boot_is_linux (node))
		buf = gst_xml_get_child_content (node, "initrd");
	if (!buf)
		return NULL;

	return buf;
}

gchar *
boot_value_module (xmlNodePtr node)
{
	gchar *buf;

	g_return_val_if_fail (node != NULL, NULL);

	buf = gst_xml_get_child_content (node, "module");

	return buf;
}

gchar *
boot_value_password (xmlNodePtr node)
{
	gchar *buf;
	
	g_return_val_if_fail (node != NULL, NULL);
	
	buf = gst_xml_get_child_content (node, "password");
	
	if (!buf)
		return NULL;
	
	return buf;
}

void
table_populate (xmlNodePtr root)
{
	xmlNodePtr node;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (boot_table));

	for (node = gst_xml_element_find_first (root, "entry");
	     node != NULL; node = gst_xml_element_find_next (node, "entry")) 
	{
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
				    BOOT_LIST_COL_LABEL, boot_value_label (node),
				    BOOT_LIST_COL_DEFAULT, boot_value_default (node),
				    BOOT_LIST_COL_TYPE, boot_value_type_char (node, FALSE),
				    BOOT_LIST_COL_IMAGE, boot_value_image (node, FALSE),
				    BOOT_LIST_COL_DEV, boot_value_device (node, FALSE),
				    BOOT_LIST_COL_POINTER, node,
				    -1);

		/* we save the default entry, so it's easier to search */
		if (boot_value_default (node))
			default_entry_iter = iter;
	}
}

void
boot_table_clear (void)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (boot_table));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

void
boot_table_update (void)
{
	xmlNodePtr root;

	root = gst_xml_doc_get_root (tool->config);
	boot_table_clear ();
	callbacks_actions_set_sensitive (FALSE);
	table_populate (root);
}

/* Set value functions */
void
boot_value_set_default (xmlNodePtr node)
{
	xmlNodePtr n;
	gchar *label;

	g_return_if_fail (node != NULL);
	
	n = gst_xml_doc_get_root (tool->config);

	label = gst_xml_get_child_content (node, "label");
	if (!label)
		return;

	gst_xml_set_child_content (n, "default", label);
}

void
boot_value_set_label (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;
	gboolean is_default;

	g_return_if_fail (node != NULL);

	n0 = gst_xml_element_find_first (node, "label");
	if (n0)
		is_default = boot_value_default (node);
	else
		is_default = FALSE;

	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = gst_xml_element_add (node, "label");
		gst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			gst_xml_element_destroy (n0);

		/* TODO: remove default label */
		return;
	}

	if (is_default)
		boot_value_set_default (node);
}

void
boot_value_set_image (xmlNodePtr node, const gchar *val, GstBootImageType type)
{
	xmlNodePtr n0;
	gchar buf[6]; /* (strlen ('image') || strlen ('other')) + 1; */
	
	g_return_if_fail (node != NULL);

	if (type == TYPE_LINUX)
		strncpy (buf, "image", 6);
	else
		strncpy (buf, "other", 6);

	n0 = gst_xml_element_find_first (node, buf);
	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = gst_xml_element_add (node, buf);
		gst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			gst_xml_element_destroy (n0);
	}
}

void
boot_value_set_root (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;
	
	g_return_if_fail (node != NULL);

	n0 = gst_xml_element_find_first (node, "root");
	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = gst_xml_element_add (node, "root");
		gst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			gst_xml_element_destroy (n0);
	}
}

void
boot_value_set_append (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;
	
	g_return_if_fail (node != NULL);

	n0 = gst_xml_element_find_first (node, "append");
	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = gst_xml_element_add (node, "append");
		gst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			gst_xml_element_destroy (n0);
	}
}

void
boot_value_set_type (xmlNodePtr node, GstBootImageType type)
{
	gchar *buf;
	xmlNodePtr n0;
	
	g_return_if_fail (node != NULL);

	buf = type_to_label (type);
	n0 = gst_xml_element_find_first (node, "type");
	if (!n0)
		n0 = gst_xml_element_add (node, "type");

	gst_xml_element_set_content (n0, buf);
}

void
boot_value_set_initrd (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;

	g_return_if_fail (node != NULL);

	n0 = gst_xml_element_find_first (node, "initrd");
	if (val && strlen (val) > 0)
	{
		if (!n0)
			n0 = gst_xml_element_add (node, "initrd");
		gst_xml_element_set_content (n0, val);
	}
	else
		if (n0)
			gst_xml_element_destroy (n0);
}

void
boot_value_set_password (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;
	
	g_return_if_fail (node != NULL);
	
	n0 = gst_xml_element_find_first (node, "password");
	if (val && strlen (val) > 0)
	{
		if (!n0)
			n0 = gst_xml_element_add (node, "password");
		gst_xml_element_set_content (n0, val);
	}
	else
		if (n0)
			gst_xml_element_destroy (n0);
}

static gchar *
boot_table_get_new_key (xmlNodePtr root)
{
	xmlNodePtr node;
	gchar *key;
	gint maxkey, keynum;

	maxkey = 0;
	for (node = gst_xml_element_find_first (root, "entry");
	     node; node = gst_xml_element_find_next (node, "entry"))
	{
		key = gst_xml_get_child_content (node, "key");
		if (key) {
			keynum = atoi (key);
			if (maxkey <= keynum)
				maxkey = keynum + 1;
			g_free (key);
		} else
			/* This leaks, but it's not supposed to happen in production. */
			g_warning ("Entry %s has no key.", gst_xml_get_child_content (node, "label"));
	}

	return g_strdup_printf ("%d", maxkey);
}

xmlNodePtr
boot_table_add (void)
{
	gint row;
	gchar *newkey;
	xmlNodePtr root, node;

	root = gst_xml_doc_get_root (tool->config);
	
	newkey = boot_table_get_new_key (root);
	node = gst_xml_element_add (root, "entry");
	gst_xml_element_add_with_content (node, "key", newkey);
	g_free (newkey);
	
	return node;
}

