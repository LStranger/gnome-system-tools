/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* e-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "xst.h"
#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-table/e-table-simple.h>
#include <gal/e-table/e-cell-text.h>

#include "e-table.h"
#include "callbacks.h"

#define BOOT_TABLE_SPEC "boot.etspec"

extern XstTool *tool;

GtkWidget *boot_table = NULL;
GArray *boot_array = NULL;

const gchar *basic_boot_state = "\
<ETableState> \
  <column source=\"1\"/> \
  <column source=\"2\"/> \
  <grouping> \
  </grouping> \
</ETableState>";

const gchar *adv_boot_state = "\
<ETableState> \
  <column source=\"1\"/> \
  <column source=\"2\"/> \
  <column source=\"3\"/> \
  <column source=\"4\"/> \
  <grouping> \
  </grouping> \
</ETableState>";

static int
boot_col_count (ETableModel *etc, void *data)
{
	return 3;
}

static int
boot_row_count (ETableModel *etc, void *data)
{
	if (boot_array)
		return boot_array->len;

	/* else */
	return 0;
}

static void *
boot_value_at (ETableModel *etc, int col, int row, void *data)
{
	xmlNodePtr node;

	node = g_array_index (boot_array, xmlNodePtr, row);

	switch (col)
	{
	case COL_LABEL:
		return boot_value_label (node);
		break;
	case COL_TYPE:
		return boot_value_type_char (node, FALSE);
		break;
	case COL_IMAGE:
		return boot_value_image (node, FALSE);
		break;
	case COL_DEV:
		return boot_value_device (node, FALSE);
		break;
	default:
		return NULL;
	}
}

static void
boot_set_value_at (ETableModel *etc, int col, int row, const void *val, void *data)
{
}

static gboolean
boot_is_cell_editable (ETableModel *etc, int col, int row, void *data)
{
	return FALSE;
}

static void *
boot_duplicate_value (ETableModel *etc, int col, const void *value, void *data)
{
	return g_strdup (value);
}

static void
boot_free_value (ETableModel *etc, int col, void *value, void *data)
{
	g_free (value);
}

static void *
boot_initialize_value (ETableModel *etc, int col, void *data)
{
	return g_strdup ("");
}

static gboolean
boot_value_is_empty (ETableModel *etc, int col, const void *value, void *data)
{
	return !(value && *(char *)value);
}

static char *
boot_value_to_string (ETableModel *etc, int col, const void *value, void *data)
{
	return g_strdup(value);
}

static void
boot_cursor_change (ETable *table, gint row, gpointer user_data)
{
	actions_set_sensitive (TRUE);
}

static ETableExtras *
create_extras (void)
{
	ETableExtras *extras;
	ECell *ec;

	extras = e_table_extras_new ();

	ec = e_cell_text_new (NULL, GTK_JUSTIFY_LEFT);
	e_table_extras_add_cell (extras, "centered_cell", ec);

	return extras;
}

static void
table_structure_change (ETableHeader *eth, gpointer user_data)
{
	ETable *et;
	gchar *state;

	et = E_TABLE (user_data);
	state = e_table_get_state (et);

	switch (xst_dialog_get_complexity (tool->main_dialog)) {
	case XST_DIALOG_ADVANCED:
		xst_conf_set_string (tool, "state_adv", state);
		break;
	case XST_DIALOG_BASIC:
	default:
		xst_conf_set_string (tool, "state_basic", state);
		break;
	}

	g_free (state);
}

static void
table_dimension_change (ETableHeader *eth, int col, gpointer user_data)
{
	table_structure_change (eth, user_data);
}

/**
 * table_connect_signals:
 * @: 
 * 
 *  We have to reconnect these signals after every set_state call, cause
 *  it makes new ETableHeader and loses old signal. Same for sorting, grouping...
 **/
static void
table_connect_signals (ETable *table)
{
	gtk_signal_connect (GTK_OBJECT (table->header),
			    "structure_change",
			    table_structure_change,
			    (gpointer)table);

	gtk_signal_connect (GTK_OBJECT (table->header),
			    "dimension_change",
			    table_dimension_change,
			    (gpointer)table);

	gtk_signal_connect (GTK_OBJECT (table->sort_info),
			    "sort_info_changed",
			    GTK_SIGNAL_FUNC (table_structure_change),
			    (gpointer)table);
}

GtkWidget *
table_create (void)
{
	ETableModel  *model;
	ETableExtras *extras;
	ETable       *table;
	gchar        *spec;

	g_print ("Table create\n");
	
	if (boot_table)
		return NULL;
	
	extras = create_extras ();
	model = e_table_simple_new (boot_col_count,
				    boot_row_count,
				    boot_value_at,
				    boot_set_value_at,
				    boot_is_cell_editable,
				    boot_duplicate_value,
				    boot_free_value,
				    boot_initialize_value,
				    boot_value_is_empty,
				    boot_value_to_string,
				    NULL);

	spec = xst_conf_get_string (tool, "spec");
	if (!spec) {
		spec = xst_ui_load_etspec (tool->etspecs_common_path, BOOT_TABLE_SPEC);
		if (!spec)
			g_error ("create_table: Couldn't create table.");
		xst_conf_set_string (tool, "spec", spec);
	}

	boot_table = e_table_scrolled_new (E_TABLE_MODEL (model), extras, spec, basic_boot_state);
	g_free (spec);

	g_return_val_if_fail (boot_table != NULL, NULL);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	table_connect_signals (table);
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", boot_cursor_change, NULL);

	return boot_table;
}

void
table_populate (xmlNodePtr root)
{
	xmlNodePtr  node;
	gint        row;
	ETable     *table;

	g_return_if_fail (root != NULL);
	
	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));

	boot_array = g_array_new (FALSE, FALSE, sizeof (xmlNodePtr));
	
	for (node = xst_xml_element_find_first (root, "entry"), row = 0;
	     node != NULL;
	     node = xst_xml_element_find_next (node, "entry"), row++)
		
		g_array_prepend_val (boot_array, node);

	e_table_model_changed (table->model);
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

void *
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

	n = xst_xml_element_find_first (node, "XstPartitionType");
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

void *
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
	n0 = xst_xml_element_find_first (node, "XstPartitionType");
	if (!n0)
		n0 = xst_xml_element_add (node, "XstPartitionType");

	xst_xml_element_set_content (n0, buf);
	g_free (buf);
}

xmlNodePtr
get_selected_node (void)
{
	ETable *table;
	gint row;

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	
	if ((row = e_table_get_cursor_row (table)) >= 0)
		return g_array_index (boot_array, xmlNodePtr, row);

	return NULL;
}

void
boot_table_update_state (void)
{
	ETable *table;
	XstDialogComplexity complexity;
	gchar *state;

	g_return_if_fail (boot_table != NULL);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	complexity = tool->main_dialog->complexity;

	if (complexity == XST_DIALOG_BASIC) {
		state = xst_conf_get_string (tool, "state_basic");
		if (state == NULL) {
			state = g_strdup (basic_boot_state);
			xst_conf_set_string (tool, "state_basic", state);
		}
	} else {
		state = xst_conf_get_string (tool, "state_adv");
		if (state == NULL) {
			state = g_strdup (adv_boot_state);
			xst_conf_set_string (tool, "state_adv", state);
		}
	}

	e_table_set_state (table, state);
	table_connect_signals (table);
	g_free (state);
}

void
boot_table_delete (void)
{
	ETable *table;
	gint row;
	xmlNodePtr node;

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	row = e_table_get_cursor_row (table);
	node = g_array_index (boot_array, xmlNodePtr, row);

	xst_xml_element_destroy (node);
	g_array_remove_index (boot_array, row);

	e_table_model_row_deleted (table->model, row);
}

void
boot_table_update (void)
{
	ETable *table;
	gint row;

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	row = e_table_get_cursor_row (table);

	e_table_model_row_changed (table->model, row);
}

xmlNodePtr
boot_table_add (void)
{
	ETable *table;
	gint row;
	xmlNodePtr node;

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));

	node = xst_xml_doc_get_root (tool->config);
	node = xst_xml_element_add (node, "entry");
	
	g_array_append_val (boot_array, node);
	
	row = boot_array->len - 1;
	
	e_table_model_row_inserted (table->model, row);
	e_table_set_cursor_row (table, row);

	return node;
}

