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
  <ETableColumn model_col=\"0\" _title=\"Default\" expansion=\"1.0\" minimum_width=\"16\" resizable=\"false\" cell=\"checkbox\" compare=\"integer\"/> \
  <ETableColumn model_col=\"1\" _title=\"Type\" expansion=\"1.0\" minimum_width=\"10\" resizable=\"true\" cell=\"centered_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"2\" _title=\"Label\" expansion=\"1.0\" minimum_width=\"10\" resizable=\"true\" cell=\"centered_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"3\" _title=\"Image\" expansion=\"1.0\" minimum_width=\"10\" resizable=\"true\" cell=\"centered_cell\" compare=\"string\"/> \
  <ETableState> \
    <column source=\"0\"/> \
    <column source=\"1\"/> \
    <column source=\"2\"/> \
    <column source=\"3\"/> \
    <grouping> \
      <leaf column=\"0\" ascending=\"false\"/> \
    </grouping> \
  </ETableState> \
</ETableSpecification>";

/* Static prototypes */
void init_array (void);
void *boot_value_default (xmlNodePtr node);
void *boot_value_label (xmlNodePtr node);
void *boot_value_type (xmlNodePtr node);
void *boot_value_image (xmlNodePtr node);

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
	case COL_DEFAULT:
		return boot_value_default (node);
		break;
	case COL_LABEL:
		return boot_value_label (node);
		break;
	case COL_TYPE:
		return boot_value_type (node);
		break;
	case COL_IMAGE:
		return boot_value_image (node);
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

static void
boot_double_click (ETable *table, int row, int col, GdkEvent *event)
{
	xmlNodePtr node, n;
	gchar *label;

	if (col != COL_DEFAULT)
		return;
	
	node = g_array_index (boot_array, xmlNodePtr, row);
	label = xml_get_child_content (node, "label");

	n = xml_doc_get_root (tool->config);
	xml_set_child_content (n, "default", label);

	e_table_model_changed (table->model);
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

	boot_table = e_table_scrolled_new (model, extras, table_spec, NULL);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", boot_cursor_change, NULL);
	gtk_signal_connect (GTK_OBJECT (table), "double_click", boot_double_click, NULL);
	
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
	node = xml_doc_get_root (tool->config);
	
	for (node = xml_element_find_first (node, "entry"), row = 0;
		node;
		node = xml_element_find_next (node, "entry"), row++)
		
		g_array_prepend_val (boot_array, node);

}

void *
boot_value_default (xmlNodePtr node)
{
	xmlNodePtr n;
	gchar *def, *label;
	gboolean ret = FALSE;

	g_return_val_if_fail (node != NULL, NULL);
	
	n = xml_doc_get_root (tool->config);

	def = xml_get_child_content (n, "default");
	if (!def)
		return NULL;

	label = xml_get_child_content (node, "label");
	if (!label)
	{
		g_free (def);
		return NULL;
	}

	if (!strcmp (def, label))
		ret = TRUE;

	g_free (def);
	g_free (label);

	return ((void *)ret);
}

void *
boot_value_label (xmlNodePtr node)
{
	g_return_val_if_fail (node != NULL, NULL);

	return xml_get_child_content (node, "label");
}

void *
boot_value_type (xmlNodePtr node)
{
	xmlNodePtr n;
	
	g_return_val_if_fail (node != NULL, NULL);

	n = xml_element_find_first (node, "image");

	if (n)
		return (_("Linux"));

	n = xml_element_find_first (node, "other");

	if (n)
		return (_("Other"));

	return (_("Unknown"));
}

void *
boot_value_image (xmlNodePtr node)
{
	xmlNodePtr n;
	
	g_return_val_if_fail (node != NULL, NULL);

	n = xml_element_find_first (node, "image");

	if (n)
		return xml_element_get_content (n);

	n = xml_element_find_first (node, "other");

	if (n)
		return xml_element_get_content (n);

	return NULL;
}

xmlNodePtr
get_selected_node (void)
{
	ETable *table;
	gint row;

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (boot_table));
	
	if ((row = e_table_get_cursor_row (table)) >= 0)
		return g_array_index (boot_array, xmlNodePtr, row);;

	return NULL;
}
