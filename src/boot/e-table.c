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
#include "global.h"
#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-table/e-table-simple.h>
#include <gal/e-table/e-cell-text.h>

#include "e-table.h"
#include "callbacks.h"

extern XstTool *tool;

GtkWidget *boot_table;
GArray *boot_array;

const gchar *table_spec = "\
<ETableSpecification cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Default\" expansion=\"1.0\" minimum_width=\"16\" resizable=\"true\" cell=\"checkbox\" compare=\"integer\"/> \
  <ETableColumn model_col=\"1\" _title=\"Type\" expansion=\"1.0\" minimum_width=\"10\" resizable=\"true\" cell=\"centered_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"2\" _title=\"Name\" expansion=\"1.0\" minimum_width=\"10\" resizable=\"true\" cell=\"centered_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"3\" _title=\"Kernel Image\" expansion=\"1.0\" minimum_width=\"10\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"4\" _title=\"Device\" expansion=\"1.0\" minimum_width=\"10\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
</ETableSpecification>";

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


/* Static prototypes */
void init_array (void);

static int
boot_col_count (ETableModel *etc, void *data)
{
	return 3;
}

static int
boot_row_count (ETableModel *etc, void *data)
{
	return boot_array->len;
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
		return boot_value_type (node, FALSE);
		break;
	case COL_IMAGE:
		return boot_value_image (node, FALSE);
		break;
	case COL_DEV:
		return boot_value_dev (node, FALSE);
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

	ec = e_cell_text_new (NULL, GTK_JUSTIFY_CENTER);
	e_table_extras_add_cell (extras, "centered_cell", ec);

	return extras;
}

extern void
create_table (xmlNodePtr root)
{
	ETableModel *model;
	ETableExtras *extras;
	ETable *table;
	GtkWidget *container;

	init_array ();
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

	boot_table = e_table_scrolled_new (model, extras, table_spec, basic_boot_state);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", boot_cursor_change, NULL);
	
	container = xst_dialog_get_widget (tool->main_dialog, "table_holder");
	gtk_container_add (GTK_CONTAINER (container), boot_table);
	gtk_widget_show (boot_table);
}

void
init_array (void)
{
	xmlNodePtr node;
	gint row;

	boot_array = g_array_new (FALSE, FALSE, sizeof (xmlNodePtr));
	node = xst_xml_doc_get_root (tool->config);
	
	for (node = xst_xml_element_find_first (node, "entry"), row = 0;
		node;
		node = xst_xml_element_find_next (node, "entry"), row++)
		
		g_array_prepend_val (boot_array, node);

}

void *
boot_value_label (xmlNodePtr node)
{
	g_return_val_if_fail (node != NULL, NULL);

	return xst_xml_get_child_content (node, "label");
}

void *
boot_value_type (xmlNodePtr node, gboolean bare)
{
	xmlNodePtr n;
	gchar *label, *def;
	gchar *buf = NULL;
	
	g_return_val_if_fail (node != NULL, NULL);

	n = xst_xml_element_find_first (node, "XstPartitionType");
	if (n)
		buf = xst_xml_element_get_content (n);

	else
	{
		n = xst_xml_element_find_first (node, "image");
		if (n)
			buf = g_strdup (_("Linux"));
	}

	if (!buf)
		buf = g_strdup (_("Unknown"));

	if (bare)
		return buf;

	label = xst_xml_get_child_content (node, "label");
	def = xst_xml_get_child_content (xst_xml_doc_get_root (tool->config), "default");

	if (def && !strcmp (def, label))
	{
		buf = g_strdup_printf ("%s (default)", buf);
		g_free (def);
		g_free (label);
	}

	return buf;
}

void *
boot_value_image (xmlNodePtr node, gboolean bare)
{
	gchar *buf;
	
	g_return_val_if_fail (node != NULL, NULL);
	
	buf = xst_xml_get_child_content (node, "image");
	if (!buf)
		return NULL;

	if (bare)
		return buf;
	
	return g_strdup_printf ("  %s", buf);
}

void *
boot_value_dev (xmlNodePtr node, gboolean bare)
{
	gchar *buf;
	
	g_return_val_if_fail (node != NULL, NULL);

	buf = xst_xml_get_child_content (node, "other");
	if (!buf)
		return NULL;

	if (bare)
		return buf;
	
	return g_strdup_printf ("  %s", buf);
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
boot_value_set_label (xmlNodePtr node, gchar *val)
{
	g_return_if_fail (node != NULL);

	xst_xml_set_child_content (node, "label", val);
}

void
boot_value_set_image (xmlNodePtr node, gchar *val)
{
	g_return_if_fail (node != NULL);

	xst_xml_set_child_content (node, "image", val);
}

void
boot_value_set_dev (xmlNodePtr node, gchar *val)
{
	g_return_if_fail (node != NULL);

	xst_xml_set_child_content (node, "other", val);
}

void boot_value_set_root (xmlNodePtr node, gchar *val)
{
	g_return_if_fail (node != NULL);

	xst_xml_set_child_content (node, "root", val);
}

void boot_value_set_append (xmlNodePtr node, gchar *val)
{
	g_return_if_fail (node != NULL);

	if (strlen (val) < 1)
	{
		xst_xml_element_destroy (xst_xml_element_find_first (node, "append"));
		return;
	}
	
	if (strstr (val, " ") && !strstr (val, "\""))
		val = g_strconcat ("\"", val, "\"", NULL);
	
	xst_xml_set_child_content (node, "append", val);
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

extern void
boot_table_update_state (void)
{
	ETable *table;
	XstDialogComplexity complexity;

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	complexity = tool->main_dialog->complexity;

	if (complexity == XST_DIALOG_BASIC)
		e_table_set_state (table, basic_boot_state);

	else
		e_table_set_state (table, adv_boot_state);
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

