/* e-table.c: this file is part of users-admin, a helix-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Tambet Ingo <tambeti@sa.ee> and Arturo Espinosa <arturo@helixcode.com>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "global.h"

#include "callbacks.h"
#include "transfer.h"
#include "user_group.h"

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-preview.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gal/e-table/e-table-header.h>
#include <gal/e-table/e-table-header-item.h>
#include <gal/e-table/e-table-item.h>
#include <gal/e-table/e-cell-text.h>
#include <gal/e-table/e-cell-tree.h>
#include <gal/e-table/e-cell-checkbox.h>
#include <gal/e-table/e-table.h>
#include <gal/e-table/e-table-simple.h>

#include "e-table.h"


#define USER_COLS 1
#define GROUP_COLS 1

#define USER_SPEC "<ETableSpecification> \
  <ETableColumn model_col=\"0\" _title=\"Users\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
    <ETableState> \
      <column source=\"0\"/> \
      <grouping><leaf column=\"0\" ascending=\"true\"/></grouping> \
    </ETableState> \
  </ETableSpecification>"

#define GROUP_SPEC "<ETableSpecification> \
  <ETableColumn model_col=\"0\" _title=\"Groups\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
    <ETableState> \
      <column source=\"0\"/> \
      <grouping><leaf column=\"0\" ascending=\"true\"/></grouping> \
    </ETableState> \
  </ETableSpecification>"


/* Local globals */

GtkWidget *user_table;
GtkWidget *group_table;

/* ETreeSimple callbacks
 * These are the callbacks that define the behaviour of our custom model.
 */

/* This function returns the number of columns in our ETableModel. */
static int
col_count (ETableModel *etm, void *data)
{
	if (E_TABLE (user_table)->model == etm)
		return USER_COLS;

	if (E_TABLE (group_table)->model == etm)
		return GROUP_COLS;

	return 0;
}

/* This function returns the number of rows in our ETableModel. */ 
static int
row_count (ETableModel *etm, void *data)
{
	return g_list_length ((GList *)data);
}

/* This function returns the value at a particular point in our ETableModel. */
static void *
value_at (ETableModel *etm, int col, int row, void *data)
{
	GList *tmp_list = data;

	if (E_TABLE (user_table)->model == etm)
	{
		user *u;

		u = g_list_nth_data (tmp_list, row);
		return u->login;
	}
	
	if (E_TABLE (group_table)->model == etm)
	{
		group *g;

		g = g_list_nth_data (tmp_list, row);
		return g->name;
	}

	return NULL;
}

/* This function sets value at a particular point in our ETableModel. */
static void
set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
	GList *tmp_list = data;

	if (E_TABLE (user_table)->model == etm)
	{
		user *u;

		u = g_list_nth_data (tmp_list, row);
		g_free (u->login);
		u->login = g_strdup (val);
		return;
	}

	if (E_TABLE (group_table)->model == etm)
	{
		group *g;

		g = g_list_nth_data (tmp_list, row);
		g_free (g->name);
		g->name = g_strdup (val);
		return;
	}

	return;
}

/* This function checks if cell is editable. */
static gboolean
is_cell_editable (ETableModel *etm, int col, int row, void *data)
{
	return FALSE;
}

/* This function duplicates the value passed to it. */
static void *
duplicate_value (ETableModel *etm, int col, const void *value, void *data)
{
	return g_strdup (value);
}

/* This function frees the value passed to it. */
static void
free_value (ETableModel *etm, int col, void *value, void *data)
{
	g_free (value);
}

/* This function creates an empty value. */
static void *
initialize_value (ETableModel *etm, int col, void *data)
{
	return g_strdup ("");
}

/* This function reports if a value is empty. */
static gboolean
value_is_empty (ETableModel *etm, int col, const void *value, void *data)
{
	return !(value && *(char *)value);
}

/* This function converts value to string. */
static char *
value_to_string (ETableModel *etm, int col, const void *value, void *data)
{
	return g_strdup(value);
}

/* End of ETableSimple callbacks. */



static void
select_row (ETable *et, int row)
{
	ETableModel *etm;
	gchar *txt, *label;

	etm = E_TABLE_MODEL (et->model);
	txt = e_table_model_value_at (etm, 0, row);

	if (et == E_TABLE (user_table))
	{
		user *u;

		u = g_list_nth_data (user_list, row);
		user_actions_set_sensitive (TRUE);
		label = g_strconcat (_("Settings for user "), u->login, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")), label);
		g_free (label);
		return;
	}

	if (et == E_TABLE (group_table))
	{
		group *g;

		g = g_list_nth_data (group_list, row);
		group_actions_set_sensitive (TRUE);
		label = g_strconcat ("Settings for group ", g->name, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")), label);
		g_free (label);
		return;
	}

	return;
}

/* Public functions */

void
e_table_create (void)
{
	GtkWidget *w0;
	ETableModel *e_table_model = NULL;

	/* Users table */

	e_table_model = e_table_simple_new (col_count,
                                           row_count,
                                           value_at,
                                           set_value_at,
                                           is_cell_editable,
                                           duplicate_value,
                                           free_value,
                                           initialize_value,
                                           value_is_empty,
                                           value_to_string,
                                           user_list);

	user_table = e_table_new (E_TABLE_MODEL(e_table_model), NULL, USER_SPEC, NULL);

	if (!user_table)
		g_warning ("e-table: Can't make user table");

	gtk_signal_connect (GTK_OBJECT (user_table), "cursor_change", select_row, NULL);

	w0 = tool_widget_get ("users_holder");
	gtk_container_add (GTK_CONTAINER (w0), user_table);
	gtk_widget_show (user_table);

	/* Groups table */
	e_table_model = e_table_simple_new (col_count,
                                           row_count,
                                           value_at,
                                           set_value_at,
                                           is_cell_editable,
                                           duplicate_value,
                                           free_value,
                                           initialize_value,
                                           value_is_empty,
                                           value_to_string,
                                           group_list);

	group_table = e_table_new (E_TABLE_MODEL(e_table_model), NULL, GROUP_SPEC, NULL);

	if (!group_table)
		g_warning ("e-table: Can't make group table");

	gtk_signal_connect (GTK_OBJECT (group_table), "cursor_change", select_row, NULL);

	w0 = tool_widget_get ("groups_holder");
	gtk_container_add (GTK_CONTAINER (w0), group_table);
	gtk_widget_show (group_table);

	user_actions_set_sensitive (FALSE);
	group_actions_set_sensitive (FALSE);
}

void
e_table_del (gchar del)
{
	gint row;
	ETableModel *etm;
	ETable *table;
	GList *tmp_list;

	if (del == USER)
	{
		table = E_TABLE (user_table);
		row = e_table_get_cursor_row (table);
		tmp_list = g_list_nth (user_list, row);
		user_list = g_list_remove (user_list, tmp_list->data);
	}

	else if (del == GROUP)
	{
		table = E_TABLE (group_table);
		row = e_table_get_cursor_row (table);
		tmp_list = g_list_nth (group_list, row);
		group_list = g_list_remove (group_list, tmp_list->data);
	}

	else
		return;
	
	etm = E_TABLE_MODEL (table->model);
	e_table_model_row_deleted (etm, row);
	e_table_model_changed (etm);
}

void
e_table_changed (gchar change, gboolean new)
{
	ETableModel *etm;
	ETable *table;
	gint row = 0;

	if (change == USER)
		table = E_TABLE (user_table);

	else if (change == GROUP)
		table = E_TABLE (group_table);
	else
		return;

	etm = E_TABLE_MODEL (table->model);

	if (new)
	{
		e_table_model_append_row (etm, NULL, row);
		e_table_model_row_inserted (etm, row);
	}
	else
	{
		row = e_table_get_cursor_row (table);
		e_table_model_row_changed (etm, row);
	}

	e_table_model_changed (etm);
	e_table_set_cursor_row (table, row);
	
}

void *
e_table_get (gchar get)
{
	ETableModel *etm;
	ETable *table;
	gint row;

	if (get == USER)
	{
		table = E_TABLE (user_table);
		etm = E_TABLE_MODEL (table->model);
		row = e_table_get_cursor_row (table);

		return (user *)g_list_nth_data (user_list, row);
	}

	if (get == GROUP)
	{
		table = E_TABLE (group_table);
		etm = E_TABLE_MODEL (table->model);
		row = e_table_get_cursor_row (table);

		return (group *)g_list_nth_data (group_list, row);
	}

	return NULL;
}

