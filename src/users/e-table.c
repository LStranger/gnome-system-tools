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

#define USER_SPEC "<ETableSpecification cursor-mode=\"line\"> <ETableColumn model_col=\"0\" _title=\"Users\" expansion=\"1.0\" minimum_width=\"40\" resizable=\"true\" cell=\"string\" compare=\"string\"/> <ETableColumn model_col=\"1\" _title=\"UID\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"id_compare\"/> <ETableColumn model_col=\"2\" _title=\"Home\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"string\" compare=\"string\"/> <ETableColumn model_col=\"3\" _title=\"Shell\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"string\" compare=\"string\"/><ETableColumn model_col=\"4\" _title=\"Comment\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"string\" compare=\"string\"/></ETableSpecification>"

#define GROUP_SPEC "<ETableSpecification cursor-mode=\"line\"> <ETableColumn model_col=\"0\" _title=\"Groups\" expansion=\"1.0\" minimum_width=\"60\" resizable=\"true\" cell=\"string\" compare=\"string\"/><ETableColumn model_col=\"1\" _title=\"GID\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"id_compare\"/> </ETableSpecification>"

#define ADV_USER_STATE "<ETableState><column source=\"0\"/><column source=\"1\"/><column source=\"2\"/><column source=\"3\"/><grouping><leaf column=\"0\" ascending=\"true\"/></grouping></ETableState>"

#define BASIC_USER_STATE "<ETableState><column source=\"0\"/><column source=\"4\"/><grouping><leaf column=\"0\" ascending=\"true\"/></grouping></ETableState>"

#define ADV_GROUP_STATE "<ETableState><column source=\"0\"/><column source=\"1\"/><grouping><leaf column=\"0\" ascending=\"true\"/></grouping> </ETableState>"

#define BASIC_GROUP_STATE "<ETableState><column source=\"0\"/><grouping><leaf column=\"0\" ascending=\"true\"/></grouping> </ETableState>"


/* Local globals */

GtkWidget *user_table;
GtkWidget *group_table;

/* FIXME:
 * This is a hack: I don't know how to get current row number (e_table_get_current_row doesn't work)
 * when e-table is sorted by some other column, not by initial sort. */
gint user_row;
gint group_row;

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
user_row_count (ETableModel *etm, void *data)
{
	xmlNodePtr parent = data;

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
		return xml_parent_childs (parent);

	return basic_user_count (parent);
}

static int
group_row_count (ETableModel *etm, void *data)
{
	xmlNodePtr parent = data;

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
		return xml_parent_childs (parent);

	return basic_group_count (parent);
}

/* This function returns the value at a particular point in our ETableModel. */
static void *
user_value_at (ETableModel *etm, int col, int row, void *data)
{
	xmlNodePtr parent = data;
	xmlNodePtr node, name;
	gchar *field;

	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
		node = basic_user_find_nth (parent, row);
	else
		node = xml_element_find_nth (parent, "user", row);

	if (!node)
	{
		g_warning ("value_at: Can't find %dth node\n", row);
		return NULL;
	}

	switch (col)
	{
		case 0:
			field = g_strdup ("login");
			break;
		case 1:
			field = g_strdup ("uid");
			break;
		case 2:
			field = g_strdup ("home");
			break;
		case 3:
			field = g_strdup ("shell");
			break;
		case 4:
			field = g_strdup ("comment");
			break;
		default:
			g_warning ("Wrong col nr: %d", col);
			return NULL;
	}
	
	name = xml_element_find_first (node, field);

	if (!name)
	{
		g_warning ("value_at: Can't get name for row %d\n", row);
		return NULL;
	}
	
	g_free (field);

	return xml_element_get_content (name);
}

static void *
group_value_at (ETableModel *etm, int col, int row, void *data)
{
	xmlNodePtr parent = data;
	xmlNodePtr node, name;
	gchar *field;

	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
		node = basic_group_find_nth (parent, row);
	else
		node = xml_element_find_nth (parent, "group", row);

	if (!node)
	{
		g_warning ("value_at: Can't find %dth node\n", row);
		return NULL;
	}

	switch (col)
	{
		case 0:
			field = g_strdup ("name");
			break;
		case 1:
			field = g_strdup ("gid");
			break;
		default:
			g_warning ("group_value_at: wrong col %d.", col);
			return NULL;
	}
	
	name = xml_element_find_first (node, field);
	g_free (field);

	if (!name)
	{
		g_warning ("value_at: Can't get name for row %d\n", row);
		return NULL;
	}

	return xml_element_get_content (name);
}

/* This function sets value at a particular point in our ETableModel. */
static void
user_set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
	gchar *field;
	xmlNodePtr node;

	g_return_if_fail (node = e_table_get_current_user ());

	switch (col)
	{
		case 0:
			field = g_strdup ("login");
			break;
		case 1:
			field = g_strdup ("uid");
			break;
		case 2:
			field = g_strdup ("home");
			break;
		case 3:
			field = g_strdup ("shell");
			break;
		case 4:
			field = g_strdup ("comment");
			break;
		default:
			g_warning ("Wrong col nr: %d", col);
			return;
	}

	e_table_change_user (node, field, val);
	g_free (field);

	tool_set_modified(TRUE);
}

static void
group_set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
	gchar *field;
	xmlNodePtr node;

	g_return_if_fail (node = e_table_get_current_group ());

	switch (col)
	{
		case 0:
			field = g_strdup ("name");
			break;
		case 1:
			field = g_strdup ("gid");
			break;
		default:
			g_warning ("Wrong col nr: %d", col);
			return;
	}

	e_table_change_group (node, field, val);
	g_free (field);

	tool_set_modified(TRUE);
}

/* This function checks if cell is editable. */
static gboolean
is_cell_editable (ETableModel *etm, int col, int row, void *data)
{
	if ((tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED) && tool_get_access())
		return TRUE;
	else
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
		user_row = row;
		user_actions_set_sensitive (TRUE);
		label = g_strconcat (_("Settings for user "), txt, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")), label);
		g_free (label);
		return;
	}

	if (et == E_TABLE (group_table))
	{
		group_row = row;
		group_actions_set_sensitive (TRUE);
		label = g_strconcat ("Settings for group ", txt, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")), label);
		g_free (label);
		return;
	}
	
	g_warning ("Shouldn't be here.");
	return;
}

static gint
id_compare (gconstpointer id1, gconstpointer id2)
{
	g_return_val_if_fail (id1 != NULL, 1);
	g_return_val_if_fail (id2 != NULL, -1);

	if (atoi (id1) > atoi (id2))
		return 1;
	if (atoi (id1) < atoi (id2))
		return -1;
	return 0;
}

static ETableExtras *
create_extras (void)
{
	ETableExtras *extras;

	extras = e_table_extras_new ();
	e_table_extras_add_compare (extras, "id_compare", id_compare);

	return extras;
}

/* Public functions */

/* Functions for both tables. */

void
e_table_create (void)
{
	GtkWidget *w0;
	ETableModel *e_table_model = NULL;
	xmlNodePtr root, users_node, groups_node;
	ETableExtras *extras;

	root = xml_doc_get_root (tool_config_get_xml ());
	if (!root)
	{
		g_warning ("e_table_create: couldn't find root node.");
		return;
	}

	extras = create_extras ();
	
	/* Users table */

	users_node = xml_element_find_first (root, "userdb");
	if (!users_node)
	{
		g_warning ("e_table_create: couldn't find userdb node.");
		return;
	}

	e_table_model = e_table_simple_new (col_count,
                                           user_row_count,
                                           user_value_at,
                                           user_set_value_at,
                                           is_cell_editable,
                                           duplicate_value,
                                           free_value,
                                           initialize_value,
                                           value_is_empty,
                                           value_to_string,
                                           users_node);

	user_table = e_table_new (E_TABLE_MODEL(e_table_model), extras, USER_SPEC,
			BASIC_USER_STATE);

	if (!user_table)
		g_warning ("e-table: Can't make user table");

	gtk_signal_connect (GTK_OBJECT (user_table), "cursor_change", select_row, NULL);
	gtk_signal_connect (GTK_OBJECT (user_table), "double_click", on_user_settings_clicked, NULL);

	w0 = tool_widget_get ("users_holder");
	gtk_container_add (GTK_CONTAINER (w0), user_table);
	gtk_widget_show (user_table);

	/* Groups table */

	groups_node = xml_element_find_first (root, "groupdb");
	if (!groups_node)
	{
		g_warning ("e_table_create: couldn't find groupdb node.");
		return;
	}

	e_table_model = e_table_simple_new (col_count,
                                           group_row_count,
                                           group_value_at,
                                           group_set_value_at,
                                           is_cell_editable,
                                           duplicate_value,
                                           free_value,
                                           initialize_value,
                                           value_is_empty,
                                           value_to_string,
                                           groups_node);

	group_table = e_table_new (E_TABLE_MODEL(e_table_model), extras, GROUP_SPEC,
			BASIC_GROUP_STATE);

	if (!group_table)
		g_warning ("e-table: Can't make group table");

	gtk_signal_connect (GTK_OBJECT (group_table), "cursor_change", select_row, NULL);
	gtk_signal_connect (GTK_OBJECT (group_table), "double_click", on_group_settings_clicked, NULL);

	w0 = tool_widget_get ("groups_holder");
	gtk_container_add (GTK_CONTAINER (w0), group_table);
	gtk_widget_show (group_table);

	user_actions_set_sensitive (FALSE);
	group_actions_set_sensitive (FALSE);
}

void
e_table_state (gboolean state)
{
	if (state)
	{
		e_table_set_state (E_TABLE (user_table), ADV_USER_STATE);
		e_table_set_state (E_TABLE (group_table), ADV_GROUP_STATE);
	}
	else
	{
		e_table_set_state (E_TABLE (user_table), BASIC_USER_STATE);
		e_table_set_state (E_TABLE (group_table), BASIC_GROUP_STATE);
	}

	e_table_model_changed (E_TABLE (user_table)->model);
	e_table_model_changed (E_TABLE (group_table)->model);
}

xmlNodePtr
e_table_get_table_data (gchar get)
{
	ETableModel *etm;
	ETable *table;
	xmlNodePtr node;

	switch (get)
	{
		case USER:
			table = E_TABLE (user_table);
			break;
		case GROUP:
			table = E_TABLE (group_table);
			break;
		default:
			return NULL;
	}

	etm = E_TABLE_MODEL (table->model);
	node = E_TABLE_SIMPLE (etm)->data;

	return node;
}

/* Users table functions. */

xmlNodePtr
e_table_get_current_user (void)
{
	ETableModel *etm;
	gint row;
	xmlNodePtr root, node;

	etm = E_TABLE (user_table)->model;

	/* row = e_table_get_cursor_row (E_TABLE (user_table)); */
	row = user_row;

	root = E_TABLE_SIMPLE (etm)->data;

	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
                node = basic_user_find_nth (root, row);
	else
		node = xml_element_find_nth (root, "user", row);

	if (!node)
	{
		g_warning ("e_table_get_char: can't get current user node.");
		return NULL;
	}

	return node;
}

void
e_table_del_user (xmlNodePtr node)
{
	gint row;
	ETableModel *etm;

	g_return_if_fail (node != NULL);

	xml_element_destroy (node);

/*	row = e_table_get_cursor_row (E_TABLE (user_table)); */
	row = user_row;
	etm = E_TABLE_MODEL (E_TABLE (user_table)->model);

	e_table_model_row_deleted (etm, row);
	e_table_model_changed (etm);
}

void
e_table_change_user (xmlNodePtr parent, gchar *field, gchar *val)
{
	ETableModel *etm;
	gint row;
	xmlNodePtr node;

	g_return_if_fail (parent != NULL);

	etm = E_TABLE (user_table)->model;
	/* row = e_table_get_cursor_row (E_TABLE (user_table)); */
	row = user_row;

	node = xml_element_find_first (parent, field);
	if (!node)
	{
		g_warning ("e_table_change_user: can't get field %s.", field);
		return;
	}

	xml_element_set_content (node, val);

	e_table_model_row_changed (etm, row);
	e_table_model_changed (etm);
	e_table_set_cursor_row (E_TABLE (user_table), row);
}

xmlNodePtr
e_table_add_user (gchar *login)
{
	ETableModel *etm;
	ETable *table;
	xmlNodePtr root, user;
	gint row;

	table = E_TABLE (user_table);
	etm = E_TABLE_MODEL (table->model);

	root = E_TABLE_SIMPLE (etm)->data;
	user = xml_element_add (root, "user");

	xml_element_add_with_content (user, "key", find_new_key (USER));
	xml_element_add_with_content (user, "login", login);
	xml_element_add (user, "password");
	xml_element_add_with_content (user, "uid", find_new_id (USER));
	xml_element_add (user, "gid");
	xml_element_add (user, "comment");

	if (logindefs.create_home)
		xml_element_add_with_content (user, "home", g_strdup_printf ("/home/%s", login));
	xml_element_add_with_content (user, "shell", g_strdup ("/bin/bash"));
	xml_element_add (user, "last_mod");

	xml_element_add_with_content (user, "passwd_min_life",
			g_strdup_printf ("%d", logindefs.passwd_min_day_use));

	xml_element_add_with_content (user, "passwd_max_life",
			g_strdup_printf ("%d", logindefs.passwd_max_day_use));

	xml_element_add_with_content (user, "passwd_exp_warn",
			g_strdup_printf ("%d", logindefs.passwd_warning_advance_days));
	xml_element_add (user, "passwd_exp_disable");
	xml_element_add (user, "passwd_disable");
	xml_element_add (user, "reserved");
	xml_element_add_with_content (user, "is_shadow", g_strdup ("1"));

	row = xml_parent_childs (root);
	e_table_model_append_row (etm, NULL, row);
	
	e_table_model_changed (etm); 
	e_table_model_row_inserted (etm, row);

	return user;
}


/* Groups table functions. */

xmlNodePtr 
e_table_get_current_group (void)
{
	ETableModel *etm;
	gint row;
	xmlNodePtr root, node;

	etm = E_TABLE (group_table)->model;
	/* row = e_table_get_cursor_row (E_TABLE (group_table)); */
	row = group_row;

	root = E_TABLE_SIMPLE (etm)->data;

	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
                node = basic_group_find_nth (root, row);
	else
		node = xml_element_find_nth (root, "group", row);

	if (!node)
	{
		g_warning ("e_table_get_char: can't get current group node.");
		return NULL;
	}

	return node;
}

void
e_table_del_group (xmlNodePtr node)
{
	gint row;
	ETableModel *etm;

	g_return_if_fail (node != NULL);

	xml_element_destroy (node);

	/* row = e_table_get_cursor_row (E_TABLE (group_table)); */
	etm = E_TABLE_MODEL (E_TABLE (group_table)->model);
	row = group_row;

	e_table_model_row_deleted (etm, row);
	e_table_model_changed (etm);
}


void
e_table_change_group (xmlNodePtr parent, gchar *field, gchar *val)
{
	ETableModel *etm;
	gint row;
	xmlNodePtr node;

	g_return_if_fail (parent != NULL);

	etm = E_TABLE (group_table)->model;
	/* row = e_table_get_cursor_row (E_TABLE (group_table)); */
	row = group_row;

	node = xml_element_find_first (parent, field);
	if (!node)
	{
		g_warning ("e_table_change_group: can't get field %s.", field);
		return;
	}

	xml_element_set_content (node, val);

	e_table_model_row_changed (etm, row);
	e_table_model_changed (etm);
	e_table_set_cursor_row (E_TABLE (user_table), row);
}

xmlNodePtr
e_table_add_group (gchar *new_name)
{
	ETableModel *etm;
	ETable *table;
	xmlNodePtr root, group;
	gint row;

	table = E_TABLE (group_table);
	etm = E_TABLE_MODEL (table->model);

	root = E_TABLE_SIMPLE (etm)->data;

	group = xml_element_add (root, "group");

	xml_element_add_with_content (group, "key", find_new_key (GROUP));
	xml_element_add_with_content (group, "name", new_name);
	xml_element_add_with_content (group, "password", "x");
	xml_element_add_with_content (group, "gid", find_new_id (GROUP));
	xml_element_add (group, "users");

	row = xml_parent_childs (root);
	e_table_model_append_row (etm, NULL, row);
	
	e_table_model_changed (etm); 
	e_table_model_row_inserted (etm, row);

	return group;
}

