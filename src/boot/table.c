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

#include <gnome.h>
#include "xst.h"

#include "table.h"
#include "callbacks.h"

extern XstTool *tool;

GtkWidget *boot_table = NULL;

BootTableConfig boot_table_config [] = {
	{ N_("Default"),	FALSE,	FALSE },
	{ N_("Name"),		TRUE,	TRUE },
	{ N_("Type"),		TRUE,	TRUE },
	{ N_("Kernel Image"),	TRUE,	FALSE },
	{ N_("Device"),		TRUE,	FALSE },
	{NULL}
};

void
table_construct (void)
{
	GtkWidget *sw;
	GtkWidget *list;

	sw = xst_dialog_get_widget (tool->main_dialog, "boot_table_sw");
	list = table_create ();
	gtk_widget_show_all (list);
	gtk_container_add (GTK_CONTAINER (sw), list);
}

GtkWidget *
table_create (void)
{
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	gint i;
	
	
	model = GTK_TREE_MODEL (gtk_tree_store_new (BOOT_LIST_COL_LAST, G_TYPE_STRING, G_TYPE_STRING, 
	                                            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER));
	boot_table = gtk_tree_view_new_with_model (model);
	g_object_unref (model);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (boot_table), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (boot_table), TRUE);

	for (i = 0; i < BOOT_LIST_COL_LAST - 1; i++) {
		renderer = gtk_cell_renderer_text_new ();
		
		column = gtk_tree_view_column_new_with_attributes (boot_table_config [i].name, renderer, "text", i, NULL);
		
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_column_set_sort_column_id (column, i);
		
		gtk_tree_view_insert_column (GTK_TREE_VIEW (boot_table), column, i);
	}

	/* sets the 'default' column as default sort column */
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (boot_table), 0);
	gtk_tree_view_column_set_sort_indicator (column, TRUE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (boot_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_boot_table_cursor_changed),
			  NULL);
	return boot_table;
}

void
boot_table_update_state (XstDialogComplexity complexity)
{
	GtkTreeViewColumn *column;
	gint i;
	
	g_return_if_fail (boot_table != NULL);
	
	switch (complexity) {
	case XST_DIALOG_BASIC:
		for (i = 0; i < BOOT_LIST_COL_LAST - 1; i++) {
			column = gtk_tree_view_get_column (GTK_TREE_VIEW (boot_table), i);
			gtk_tree_view_column_set_visible (column, boot_table_config [i].basic_state_showable);
		}
		break;
	case XST_DIALOG_ADVANCED:
		for (i = 0; i < BOOT_LIST_COL_LAST - 1; i++) {
			column = gtk_tree_view_get_column (GTK_TREE_VIEW (boot_table), i);
			gtk_tree_view_column_set_visible (column, boot_table_config [i].advanced_state_showable);
		}
		break;
	default:
		g_warning ("tables_update_complexity: Unsupported complexity.");
		return;
	}
}

static gboolean
boot_is_linux (xmlNodePtr node)
{
	xmlNodePtr n;
	
	g_return_val_if_fail (node != NULL, FALSE);

	n = xst_xml_element_find_first (node, "image");
	if (n)
		return TRUE;
	else
		return FALSE;
}

gchar *
boot_value_label (xmlNodePtr node)
{
	g_return_val_if_fail (node != NULL, NULL);

	return xst_xml_get_child_content (node, "label");
}

gboolean
boot_value_default (const gchar *label)
{
	gchar *def;
	gboolean retval = FALSE;
	xmlNodePtr root = xst_xml_doc_get_root (tool->config);

	def = xst_xml_get_child_content (root, "default");
	
	if (def) {		
		if (label && !strcmp (label, def))
			retval = TRUE;
		g_free (def);
	}
	return retval;
}

XstBootImageType
boot_value_type (xmlNodePtr node)
{
	xmlNodePtr n;
	XstBootImageType type = TYPE_UNKNOWN;
	
	g_return_val_if_fail (node != NULL, type);

	n = xst_xml_element_find_first (node, "type");
	if (n) {
		gchar *buf;
		
		buf = xst_xml_element_get_content (n);
		type =  label_to_type (buf);
		g_free (buf);
		return type;
	}

	n = xst_xml_element_find_first (node, "image");
	if (n)
		type = TYPE_LINUX;
	
	return type;
}

gchar *
boot_value_type_char (xmlNodePtr node, gboolean bare)
{
	XstBootImageType type;
	gchar *buf, *label;

	type = boot_value_type (node);
	buf = type_to_label (type);

	if (!bare) {
		label = xst_xml_get_child_content (node, "label");
		if (label) {
			if (boot_value_default (label))
				buf = g_strdup_printf (_("%s (default)"), buf);
			g_free (label);
		}
	}
	
	return buf;
}

void *
boot_value_device (xmlNodePtr node, gboolean bare)
{
	gchar *buf, *device;

	g_return_val_if_fail (node != NULL, NULL);
	
	if (boot_is_linux (node))
		buf = xst_xml_get_child_content (node, "root");
	else
		buf = xst_xml_get_child_content (node, "other");

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
		buf = xst_xml_get_child_content (node, "image");
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

	return xst_xml_get_child_content (node, "root");
}

gchar *
boot_value_append (xmlNodePtr node)
{
	gchar *buf;
	gchar ** str_array;
	
	g_return_val_if_fail (node != NULL, NULL);

	buf = xst_xml_get_child_content (node, "append");
	if (!buf)
		return NULL;

	str_array = g_strsplit (buf, "\"", 10); /* FIXME: 10 = max_tokens */
	buf = g_strjoinv (NULL, str_array);
	g_strfreev (str_array);

	return buf;
}

void
table_populate (xmlNodePtr root)
{
	xmlNodePtr node;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (boot_table));
	
	for (node = xst_xml_element_find_first (root, "entry"); node != NULL; node = xst_xml_element_find_next (node, "entry")) 
	{
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
		                    BOOT_LIST_COL_DEFAULT, NULL,
		                    BOOT_LIST_COL_LABEL, boot_value_label (node),
		                    BOOT_LIST_COL_TYPE, boot_value_type_char (node, FALSE),
		                    BOOT_LIST_COL_IMAGE, boot_value_image (node, FALSE),
		                    BOOT_LIST_COL_DEV, boot_value_device (node, FALSE),
	                            BOOT_LIST_COL_POINTER, node,
		                    -1);
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
	xmlNodePtr root = xst_xml_doc_get_root (tool->config);

	boot_table_clear ();
	table_populate (root);
}

/* Set value functions */

void
boot_value_set_default (xmlNodePtr node)
{
	xmlNodePtr n;
	gchar *label;

	g_return_if_fail (node != NULL);
	
	n = xst_xml_doc_get_root (tool->config);

	label = xst_xml_get_child_content (node, "label");
	if (!label)
		return;

	xst_xml_set_child_content (n, "default", label);
}

void
boot_value_set_label (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;
	gchar *old_name = NULL;
	
	g_return_if_fail (node != NULL);

	n0 = xst_xml_element_find_first (node, "label");
	if (n0)
		old_name = xst_xml_element_get_content (n0);
	
	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = xst_xml_element_add (node, "label");
		xst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			xst_xml_element_destroy (n0);

		/* TODO: remove default label */
		return;
	}

	if (old_name) {
		if (boot_value_default (old_name))
			boot_value_set_default (node);
		g_free (old_name);
	}	
}

void
boot_value_set_image (xmlNodePtr node, const gchar *val, XstBootImageType type)
{
	xmlNodePtr n0;
	gchar buf[6]; /* (strlen ('image') || strlen ('other')) + 1; */
	
	g_return_if_fail (node != NULL);

	if (type == TYPE_LINUX)
		strncpy (buf, "image", 6);
	else
		strncpy (buf, "other", 6);

	n0 = xst_xml_element_find_first (node, buf);
	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = xst_xml_element_add (node, buf);
		xst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			xst_xml_element_destroy (n0);
	}
}

void
boot_value_set_root (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;
	
	g_return_if_fail (node != NULL);

	n0 = xst_xml_element_find_first (node, "root");
	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = xst_xml_element_add (node, "root");
		xst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			xst_xml_element_destroy (n0);
	}
}

void
boot_value_set_append (xmlNodePtr node, const gchar *val)
{
	xmlNodePtr n0;
	
	g_return_if_fail (node != NULL);

	n0 = xst_xml_element_find_first (node, "append");
	if (val && strlen (val) > 0) {
		if (!n0)
			n0 = xst_xml_element_add (node, "append");
		xst_xml_element_set_content (n0, val);
	} else {
		if (n0)
			xst_xml_element_destroy (n0);
	}
}

void
boot_value_set_type (xmlNodePtr node, XstBootImageType type)
{
	gchar *buf;
	xmlNodePtr n0;
	
	g_return_if_fail (node != NULL);

	buf = type_to_label (type);
	n0 = xst_xml_element_find_first (node, "type");
	if (!n0)
		n0 = xst_xml_element_add (node, "type");

	xst_xml_element_set_content (n0, buf);
	g_free (buf);
}

static gchar *
boot_table_get_new_key (xmlNodePtr root)
{
	xmlNodePtr node;
	gchar *key;
	gint maxkey, keynum;

	maxkey = 0;
	for (node = xst_xml_element_find_first (root, "entry");
	     node; node = xst_xml_element_find_next (node, "entry"))
	{
		key = xst_xml_get_child_content (node, "key");
		if (key) {
			keynum = atoi (key);
			if (maxkey <= keynum)
				maxkey = keynum + 1;
			g_free (key);
		} else
			/* This leaks, but it's not supposed to happen in production. */
			g_warning ("Entry %s has no key.", xst_xml_get_child_content (node, "label"));
	}

	return g_strdup_printf ("%d", maxkey);
}

xmlNodePtr
boot_table_add (void)
{
	gint row;
	gchar *newkey;
	xmlNodePtr root, node;

	root = xst_xml_doc_get_root (tool->config);
	
	newkey = boot_table_get_new_key (root);
	node = xst_xml_element_add (root, "entry");
	xst_xml_element_add_with_content (node, "key", newkey);
	g_free (newkey);
	
	return node;
}

