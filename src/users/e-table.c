/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* e-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "global.h"
#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-table/e-table-memory.h>
#include <gal/e-table/e-table-memory-callbacks.h>
#include <gal/e-table/e-cell-text.h>
#include <gal/e-paned/e-hpaned.h>

#include "e-table.h"
#include "user_group.h"
#include "callbacks.h"

extern XstTool *tool;

GtkWidget *user_table;
GtkWidget *group_table;
GtkWidget *net_group_table;
GtkWidget *net_user_table;

gint active_table;

/* e-table specifications. */

const gchar *user_spec = "\
<ETableSpecification cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Users\" expansion=\"1.0\" minimum_width=\"40\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"UID\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"id_compare\"/> \
  <ETableColumn model_col=\"2\" _title=\"Home\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"3\" _title=\"Shell\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"4\" _title=\"Comment\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"5\" _title=\"Group\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
</ETableSpecification>";

const gchar *group_spec = "\
<ETableSpecification cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Groups\" expansion=\"1.0\" minimum_width=\"60\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"GID\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"id_compare\"/> \
</ETableSpecification>";

const gchar *net_group_spec = "\
<ETableSpecification no-headers=\"true\" cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Group\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableState> \
    <column source=\"0\"/> \
    <grouping></grouping> \
  </ETableState> \
</ETableSpecification>";

const gchar *net_user_spec = "\
<ETableSpecification no-headers=\"true\" cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"User\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"string\" compare=\"string\"/> \
  <ETableState> \
    <column source=\"0\"/> \
    <grouping></grouping> \
  </ETableState> \
</ETableSpecification>";

/* e-table states. */

const gchar *adv_user_state = "\
<ETableState> \
  <column source=\"0\"/> \
  <column source=\"1\"/> \
  <column source=\"2\"/> \
  <column source=\"3\"/> \
  <grouping> \
    <leaf column=\"0\" ascending=\"true\"/> \
  </grouping> \
</ETableState>";

const gchar *basic_user_state = "\
<ETableState> \
  <column source=\"0\"/> \
  <column source=\"4\"/> \
  <column source=\"5\"/> \
  <grouping> \
    <leaf column=\"0\" ascending=\"true\"/> \
  </grouping> \
</ETableState>";

const gchar *adv_group_state = "\
<ETableState> \
  <column source=\"0\"/> \
  <column source=\"1\"/> \
  <grouping> \
    <leaf column=\"0\" ascending=\"true\"/> \
  </grouping> \
</ETableState>";

const gchar *basic_group_state = "\
<ETableState> \
  <column source=\"0\"/> \
  <grouping> \
    <leaf column=\"0\" ascending=\"true\"/> \
  </grouping> \
</ETableState>";

/* Static functions */
static gchar *get_row_color (ETableModel *etm, gint row);

static int
col_count (ETableModel *etm, void *data)
{
        return 0;
}

static int
user_col_count (ETableModel *etm, void *data)
{
        return COL_USER_LAST;
}

static int
group_col_count (ETableModel *etm, void *data)
{
	return COL_GROUP_LAST;
}

static void *
duplicate_value (ETableModel *etm, int col, const void *value, void *data)
{
        return g_strdup (value);
}

static void
free_value (ETableModel *etm, int col, void *value, void *data)
{
        g_free (value);
}

static void *
initialize_value (ETableModel *etm, int col, void *data)
{
        return g_strdup ("");
}

static gboolean
value_is_empty (ETableModel *etm, int col, const void *value, void *data)
{
        return !(value && *(char *)value);
}

static char *
value_to_string (ETableModel *etm, int col, const void *value, void *data)
{
	return (gchar *)value;
}

static void
set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
}

static void
user_set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
	xmlNodePtr node;
	gchar *field;

	g_return_if_fail (xst_tool_get_access (tool));

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return;

	switch (col)
	{
		case COL_USER_LOGIN:
			if (!check_user_login (node, (gpointer)val))
				return;

			field = g_strdup ("login");
			break;
		case COL_USER_UID:
			if (!check_user_uid (node, (gpointer)val))
				return;

			field = g_strdup ("uid");
			break;
		case COL_USER_HOME:
			if (!check_user_home (node, (gpointer)val))
				return;

			field = g_strdup ("home");
			break;
		case COL_USER_SHELL:
			if (!check_user_shell (node, (gpointer)val))
				return;

			field = g_strdup ("shell");
			break;
		case COL_USER_COMMENT:
			if (!check_user_comment (node, (gpointer)val))
				return;

			field = g_strdup ("comment");
			break;
		default:
			g_warning ("user_set_value_at: wrong col nr");
			return;
	}

	xst_xml_set_child_content (node, field, (gpointer)val);
	g_free (field);

	xst_dialog_modify (tool->main_dialog);
}

static void
group_set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
	xmlNodePtr node;
	gchar *field;

	g_return_if_fail (xst_tool_get_access (tool));

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return;

	switch (col)
	{
		case COL_GROUP_NAME:
			if (!check_group_name (node, (gpointer)val))
				return;

			field = g_strdup ("name");
			break;
		case COL_GROUP_GID:
			if (!check_group_gid (node, (gpointer)val))
				return;

			field = g_strdup ("gid");
			break;
		default:
			g_warning ("group_set_value_at: wrong col nr");
			return;
	}

	xst_xml_set_child_content (node, field, (gpointer)val);
	g_free (field);

	xst_dialog_modify (tool->main_dialog);
}

static gboolean
is_editable (ETableModel *etm, int col, int row, void *model_data)
{
	return (xst_tool_get_access (tool) &&
		   (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED));
}

gchar *
user_value_group (xmlNodePtr user_node)
{
	gchar *gid, *buf;
	xmlNodePtr group_node;

	gid = xst_xml_get_child_content (user_node, "gid");
	group_node = get_corresp_field (get_db_node (user_node));
	group_node = get_node_by_data (group_node, "gid", gid);
	if (!group_node)
		return gid;
	
	buf = xst_xml_get_child_content (group_node, "name");
	if (!buf)
		return gid;
	
	g_free (gid);
	return buf;
}

static void *
user_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;
	gchar *field;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	switch (col)
	{
	case COL_USER_LOGIN:
		field = g_strdup ("login");
		break;
	case COL_USER_UID:
		field = g_strdup ("uid");
		break;
	case COL_USER_HOME:
		field = g_strdup ("home");
		break;
	case COL_USER_SHELL:
		field = g_strdup ("shell");
		break;
	case COL_USER_COMMENT:
		field = g_strdup ("comment");
		break;
	case COL_USER_GROUP:
		return user_value_group (node);
		break;
	case COL_USER_COLOR:
		return get_row_color (etm, row);
		break;
	default:
		g_warning ("user_value_at: wrong col nr");
		return NULL;
	}

	node = xst_xml_element_find_first (node, field);
	g_free (field);

	if (!node)
		return NULL;

	return xst_xml_element_get_content (node);
}

static void *
group_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;
	gchar *field;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	switch (col)
	{
	case COL_GROUP_NAME:
		field = g_strdup ("name");
		break;
	case COL_GROUP_GID:
		field = g_strdup ("gid");
		break;
	case COL_USER_COLOR:
		return get_row_color (etm, row);
		break;
	default:
		g_warning ("group_value_at: wrong col nr");
		return NULL;
	}

	node = xst_xml_element_find_first (node, field);
	g_free (field);

	if (!node)
		return NULL;

	return xst_xml_element_get_content (node);
}

static void *
net_group_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	return xst_xml_get_child_content (node, "name");
}

static void *
net_user_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	return xst_xml_get_child_content (node, "login");
}

static GtkWidget *
create_container (void)
{
	GtkWidget *paned, *container;

	paned = e_hpaned_new ();
	container = xst_dialog_get_widget (tool->main_dialog, "network_placeholder");

	gtk_container_add (GTK_CONTAINER (container), paned);
	e_paned_set_position (E_PANED (paned), 160); /* 160 is the width of left pane. */
	gtk_widget_show (paned);

	return (paned);
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
/*	ECell *ec; */

	extras = e_table_extras_new ();
	e_table_extras_add_compare (extras, "id_compare", id_compare);

	/* Sometimes we want to center cols, sometimes we don't... */
/*	ec = e_cell_text_new (NULL, GTK_JUSTIFY_CENTER);
	gtk_object_set (GTK_OBJECT (ec), "color_column", COL_USER_COLOR, NULL);
	e_table_extras_add_cell (extras, "my_cell", ec);
*/	
	return extras;
}

static void
cursor_change (ETable *et, gint row, gpointer user_data)
{
	gint table;

	table = GPOINTER_TO_INT (E_TABLE_MEMORY_CALLBACKS (et->model)->data);

	set_active_table (table);
	actions_set_sensitive (table, TRUE);
}

static void
net_group_cursor_change (ETable *table, gint row, gpointer user_data)
{
	ETable *u_table;
	xmlNodePtr node, u_node;
	gchar *buf, *user;

	/* Get group node */
	node = e_table_memory_get_data (E_TABLE_MEMORY (table->model), row);

	/* Get users table */
	u_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_user_table));
	
	/* Clear net_user_table */
	e_table_memory_clear (E_TABLE_MEMORY (u_table->model));
	
	/* Get group users */
	node = xst_xml_element_find_first (node, "users");
	for (node = node->childs; node; node = node->next)
	{
		user = xst_xml_element_get_content (node);

		if (!user)
			continue;

		u_node = get_nis_user_root_node ();
		for (u_node = u_node->childs; u_node; u_node = u_node->next)
		{
			buf = xst_xml_get_child_content (u_node, "login");

			if (!buf)
				continue;

			if (strcmp (buf, user))
			{
				g_free (buf);
				continue;
			}

			e_table_memory_insert (E_TABLE_MEMORY (u_table->model), -1, u_node);
			break;
		}

		g_free (user);
	}
}

static gchar *
get_row_color (ETableModel *etm, gint row)
{
	xmlNodePtr node, db_node;
	gchar *buf = NULL;
	gint id, min, max;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	db_node = get_db_node (node);
	get_min_max (db_node, &min, &max);

	if (!strcmp (db_node->name, "userdb"))
		buf = xst_xml_get_child_content (node, "uid");

	else if (!strcmp (db_node->name, "groupdb"))
		buf = xst_xml_get_child_content (node, "gid");

	if (!buf)
		return NULL;

	id = atoi (buf);
	g_free (buf);

	if (id >= min && id <= max)
		return COLOR_NORMAL;

	return NULL;
}

static void
create_user_table (void)
{
	ETable *table;
	ETableModel *model;
	ETableExtras *extras;
	GtkWidget *container;

	extras = create_extras ();

	model = e_table_memory_callbacks_new (user_col_count,
					      user_value_at,
					      user_set_value_at,
					      is_editable,
					      duplicate_value,
					      free_value,
					      initialize_value,
					      value_is_empty,
					      value_to_string,
					      GINT_TO_POINTER (TABLE_USER));

	user_table = e_table_scrolled_new (E_TABLE_MODEL (model), extras, user_spec,
					   basic_user_state);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", cursor_change, NULL);
	gtk_signal_connect (GTK_OBJECT (table), "double_click", on_settings_clicked, NULL);

	container = xst_dialog_get_widget (tool->main_dialog, "users_holder");
	gtk_container_add (GTK_CONTAINER (container), user_table);
	gtk_widget_show (user_table);
}

static void
create_group_table (void)
{
	ETable *table;
	ETableModel *model;
	ETableExtras *extras;
	GtkWidget *container;

	extras = create_extras ();

	model = e_table_memory_callbacks_new (group_col_count,
					      group_value_at,
					      group_set_value_at,
					      is_editable,
					      duplicate_value,
					      free_value,
					      initialize_value,
					      value_is_empty,
					      value_to_string,
					      GINT_TO_POINTER (TABLE_GROUP));

	group_table = e_table_scrolled_new (E_TABLE_MODEL (model), extras, group_spec,
					    basic_group_state);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", cursor_change, NULL);
	gtk_signal_connect (GTK_OBJECT (table), "double_click", on_settings_clicked, NULL);

	container = xst_dialog_get_widget (tool->main_dialog, "groups_holder");
	gtk_container_add (GTK_CONTAINER (container), group_table);
	gtk_widget_show (group_table);
}

static void
create_network_group_table (GtkWidget *paned)
{
	ETable *table;
	ETableModel *model;

	model = e_table_memory_callbacks_new (col_count,
					      net_group_value_at,
					      set_value_at,
					      is_editable,
					      duplicate_value,
					      free_value,
					      initialize_value,
					      value_is_empty,
					      value_to_string,
					      GINT_TO_POINTER (TABLE_NET_GROUP));

	net_group_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, net_group_spec, NULL);
	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_group_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", cursor_change, NULL);
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", net_group_cursor_change, NULL);

	gtk_container_add (GTK_CONTAINER (paned), net_group_table);
	gtk_widget_show (net_group_table);
}

static void
create_network_user_table (GtkWidget *paned)
{
	ETable *table;
	ETableModel *model;

	model = e_table_memory_callbacks_new (col_count,
					      net_user_value_at,
					      set_value_at,
					      is_editable,
					      duplicate_value,
					      free_value,
					      initialize_value,
					      value_is_empty,
					      value_to_string,
					      GINT_TO_POINTER (TABLE_NET_USER));

	net_user_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, net_user_spec, NULL);
	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_user_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_activated", cursor_change, NULL);

	gtk_container_add (GTK_CONTAINER (paned), net_user_table);
	gtk_widget_show (net_user_table);
}

void
clear_all_tables (void)
{
	ETable *table[4];
	gint i;

	table[0] = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	table[1] = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
	table[2] = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_group_table));
	table[3] = NULL;

	for (i = 0; table[i]; i++)
		e_table_memory_clear (E_TABLE_MEMORY (table[i]->model));
}

void
populate_table (ETableModel *model, xmlNodePtr root_node)
{
	xmlNodePtr node;

	g_return_if_fail (model != NULL);

	if (!root_node) /* no NIS for example. */
		return;

	e_table_memory_freeze (E_TABLE_MEMORY (model));
	for (node = root_node->childs; node; node = node->next)
	{
		if (check_node_complexity (node))
			e_table_memory_insert (E_TABLE_MEMORY (model), -1, node);
	}

	e_table_memory_thaw (E_TABLE_MEMORY (model));
}

void
populate_all_tables (void)
{
	ETable *table[4];
	xmlNodePtr node[4];
	gint i;

	table[0] = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	table[1] = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
	table[2] = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_group_table));
	table[3] = NULL;

	node[0] = get_user_root_node ();
	node[1] = get_group_root_node ();
	node[2] = get_nis_group_root_node ();
	node[3] = NULL;

	for (i = 0; table[i]; i++)
		populate_table (table[i]->model, node[i]);
}

extern guint
create_tables (void)
{
	GtkWidget *paned;

	paned = create_container ();

	/* Main tables */
	create_user_table ();
	create_group_table ();

	/* Network tables */
	create_network_group_table (paned);
	create_network_user_table (paned);

	return FALSE;
}

extern void
destroy_tables (void)
{
	gtk_widget_destroy (user_table);
	gtk_widget_destroy (group_table);
	gtk_widget_destroy (net_group_table);
	gtk_widget_destroy (net_user_table);
}

static gboolean
table_set_cursor_node (ETable *table, xmlNodePtr old_node)
{
	xmlNodePtr node;
	gboolean found;
	gint row;

	if (!old_node)
		return FALSE;
	
	found = FALSE;
	row = 0;
	while (TRUE)
	{
		node = e_table_memory_get_data (E_TABLE_MEMORY (table->model), row);
		if (!node)
			break;

		if (node == old_node)
		{
			e_table_set_cursor_row (table, row);
			found = TRUE;
			break;
		}
		row++;
	}

	return found;
}

void
tables_update_content (void)
{
	ETable *u_table, *g_table;
	xmlNodePtr u_node, g_node;
	gint row;
	guint saved_table;

	/* Save it, cause table_set_cursor_node changes it. */
	saved_table = active_table;
	
	u_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	g_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));

	u_node = g_node = NULL;
	
	/* Get selected user node */
	if ((row = e_table_get_cursor_row (u_table)) >= 0)
		u_node = e_table_memory_get_data (E_TABLE_MEMORY (u_table->model), row);

	/* Get selected group node */
	if ((row = e_table_get_cursor_row (g_table)) >= 0)
		g_node = e_table_memory_get_data (E_TABLE_MEMORY (g_table->model), row);
	
	clear_all_tables ();
	populate_all_tables ();

	actions_set_sensitive (TABLE_USER,  table_set_cursor_node (u_table, u_node));
	actions_set_sensitive (TABLE_GROUP, table_set_cursor_node (g_table, g_node));

	/* Restore active_table */
	set_active_table (saved_table);
}

void
tables_set_state (gboolean state)
{
	ETable *u_table, *g_table;
	
	u_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	g_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));

	if (state)
	{
		e_table_set_state (u_table, adv_user_state);
		e_table_set_state (g_table, adv_group_state);
	}
	else
	{
		e_table_set_state (u_table, basic_user_state);
		e_table_set_state (g_table, basic_group_state);
	}

	tables_update_content ();
}

static ETable *
get_table (gint tbl)
{
	ETable *table = NULL;

	switch (tbl)
	{
	case TABLE_USER:
		table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
		break;
	case TABLE_GROUP:
		table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
		break;
	case TABLE_NET_GROUP:
		table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_group_table));
		break;
	case TABLE_NET_USER:
		table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_user_table));
		break;
		
	default:
		return NULL;
	}
	
	return table;
}

static ETable *
get_current_table (void)
{
	return get_table (active_table);
}

xmlNodePtr
get_selected_node (void)
{
	ETable *table;
	gint row;

	g_return_val_if_fail (table = get_current_table (), NULL);

	if ((row = e_table_get_cursor_row (table)) >= 0)
		return e_table_memory_get_data (E_TABLE_MEMORY (table->model), row);
	
	return NULL;
}

gboolean
delete_selected_node (gint tbl)
{
	ETable *table;
	gint row;

	g_return_val_if_fail (table = get_table (tbl), FALSE);

	if ((row = e_table_get_cursor_row (table)) >= 0)
	{
		e_table_memory_remove (E_TABLE_MEMORY (table->model), row);
		return TRUE;
	}

	return FALSE;
}

void
current_table_update_row (gint tbl)
{
	ETable *table;

	table = get_table (tbl);
	e_table_model_row_changed (table->model,
				   e_table_get_cursor_row (table));
}

void
current_table_new_row (xmlNodePtr node, gint tbl)
{
	ETable *table;

	g_return_if_fail (node != NULL);

	table = get_table (tbl);
	e_table_memory_insert (E_TABLE_MEMORY (table->model), -1, node);
}

void
set_active_table (guint tbl)
{
	active_table = tbl;
}
