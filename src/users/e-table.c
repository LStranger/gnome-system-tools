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
#include "xst.h"
#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-table/e-table-memory.h>
#include <gal/e-table/e-table-memory-callbacks.h>
#include <gal/e-table/e-cell-text.h>
#include <gal/e-paned/e-hpaned.h>

#include "e-table.h"
#include "user_group.h"
#include "callbacks.h"
#include "user-group-xml.h"

#define USER_TABLE_SPEC "users.etspec"
#define GROUP_TABLE_SPEC "group.etspec"
#define NET_USER_TABLE_SPEC "net_user.etspec"
#define NET_GROUP_TABLE_SPEC "net_group.etspec"

extern XstTool *tool;

GtkWidget *user_table;
GtkWidget *group_table;
GtkWidget *net_group_table;
GtkWidget *net_user_table;

gint active_table;

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
	XstDialog *xd;

	g_return_if_fail (xst_tool_get_access (tool));

	xd = tool->main_dialog;
	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return;

	switch (col) {
	case COL_USER_LOGIN: user_set_value_login (node, val); break;
	case COL_USER_UID: user_set_value_uid_string (node, val); break;
	case COL_USER_HOME: user_set_value_home (node, val); break;
	case COL_USER_SHELL: user_set_value_shell (node, val); break;
	case COL_USER_COMMENT: user_set_value_comment (node, val); break;
	case COL_USER_GROUP: user_set_value_group (node, val); break;
	case COL_USER_GID: user_set_value_gid_string (node, val); break;
	default: g_warning ("user_set_value_at: wrong col nr");	break;
	}
}

static void
group_set_value_at (ETableModel *etm, int col, int row, const void *val, void *data)
{
	xmlNodePtr node;
	XstDialog *xd;

	g_return_if_fail (xst_tool_get_access (tool));

	xd = tool->main_dialog;
	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return;

	switch (col) {
	case COL_GROUP_NAME: group_set_value_name (xd, node, val); break;
	case COL_GROUP_GID: group_set_value_gid (xd, node, val); break;
	default: g_warning ("group_set_value_at: wrong col nr"); break;
	}
}

static gboolean
is_editable (ETableModel *etm, int col, int row, void *model_data)
{
/* Temporarily disabled.
  return (xst_tool_get_access (tool) &&
		   (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED));
*/
	return FALSE;
}

static void *
user_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	switch (col) {
	case COL_USER_LOGIN:   return user_value_login_peek (node); break;
	case COL_USER_UID:     return user_value_uid_string_peek (node); break;
	case COL_USER_HOME:    return user_value_home_peek (node); break;
	case COL_USER_SHELL:   return user_value_shell_peek (node); break;
	case COL_USER_COMMENT: return user_value_comment_peek (node); break;
	case COL_USER_GROUP:   return user_value_group_peek (node); break;
	case COL_USER_GID:     return user_value_gid_string_peek (node); break;

	case COL_USER_COLOR:   return get_row_color (etm, row); break;

	default:
		g_warning ("user_value_at: wrong col nr");
		break;
	}
	return NULL;
}

static void *
group_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	switch (col)
	{
	case COL_GROUP_NAME: return group_value_name_peek (node); break;
	case COL_GROUP_GID:  return group_value_gid_string_peek (node); break;
		
	case COL_USER_COLOR: return get_row_color (etm, row); break;
		
	default:
		g_warning ("group_value_at: wrong col nr");
	}
	
	return NULL;
}

static void *
net_group_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	return group_value_name (node);
}

static void *
net_user_value_at (ETableModel *etm, int col, int row, void *model_data)
{
	xmlNodePtr node;

	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	if (!node)
		return NULL;

	return user_value_login (node);
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
	if (id1 == NULL)
		return 1;
	if (id2 == NULL)
		return 2;

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
	gint min, max;
	gint id = -1;
	
	node = e_table_memory_get_data (E_TABLE_MEMORY (etm), row);
	db_node = get_db_node (node);
	get_min_max (db_node, &min, &max);

	if (!strcmp (db_node->name, "userdb"))
		id = user_value_uid_integer (node);
	else if (!strcmp (db_node->name, "groupdb"))
		id = group_value_gid_integer (node);

	if (id < 0)
		return NULL;

	if (id >= min && id <= max)
		return COLOR_NORMAL;

	return NULL;
}

static gchar *
construct_key (guint tbl, XstDialogComplexity c)
{
	gchar *buf, *key;

	switch (tbl) {
	case TABLE_USER:
		buf = g_strdup ("user_state");
		break;
	case TABLE_GROUP:
		buf = g_strdup ("group_state");
		break;
	default:
		return NULL;
	}

	switch (c) {
	case XST_DIALOG_ADVANCED:
		key = g_strconcat (buf, "_adv", NULL);
		break;
	case XST_DIALOG_BASIC:
	default:
		key = g_strconcat (buf, "_basic", NULL);
		break;
	}

	g_free (buf);
	return key;
}

static void
table_structure_change (ETableHeader *eth, gpointer user_data)
{
	ETable *et;
	gchar *state, *key;

	et = E_TABLE (user_data);
	state = e_table_get_state (et);
	
	key = construct_key (GPOINTER_TO_INT (E_TABLE_MEMORY_CALLBACKS (et->model)->data),
			     xst_dialog_get_complexity (tool->main_dialog));

	xst_conf_set_string (tool, key, state);

	g_free (key);
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

static void
create_user_table (void)
{
	ETable *table;
	ETableModel *model;
	ETableExtras *extras;
	GtkWidget *container;
	gchar *spec, *state;

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

	spec = xst_conf_get_string (tool, "user_spec");
	if (!spec) {
		spec = xst_ui_load_etspec (tool->etspecs_common_path, USER_TABLE_SPEC);
		if (!spec)
			g_error ("create_user_table: Couldn't create table.");
		xst_conf_set_string (tool, "user_spec", spec);
	}

	if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED)
		state = xst_conf_get_string (tool, "user_state_adv");
	else
		state = xst_conf_get_string (tool, "user_state_basic");
	
	if (!state) {
		state = g_strdup (basic_user_state);
		xst_conf_set_string (tool, "user_state_basic", basic_user_state);
		xst_conf_set_string (tool, "user_state_adv", adv_user_state);
	}

	user_table = e_table_scrolled_new (E_TABLE_MODEL (model), extras, spec, state);
	
	g_free (spec);
	g_free (state);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	table_connect_signals (table);
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", cursor_change, NULL);
	gtk_signal_connect (GTK_OBJECT (table), "double_click", on_user_settings_clicked, NULL);

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
	gchar *spec, *state;

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

	spec = xst_conf_get_string (tool, "group_spec");
	if (!spec) {
		spec = xst_ui_load_etspec (tool->etspecs_common_path, GROUP_TABLE_SPEC);
		if (!spec)
			g_error ("create_user_table: Couldn't create table.");
		xst_conf_set_string (tool, "group_spec", spec);
	}

	if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED)
		state = xst_conf_get_string (tool, "group_state_adv");
	else
		state = xst_conf_get_string (tool, "group_state_basic");
	
	if (!state) {
		state = g_strdup (basic_group_state);
		xst_conf_set_string (tool, "group_state_basic", basic_group_state);
		xst_conf_set_string (tool, "group_state_adv", adv_group_state);
	}

	group_table = e_table_scrolled_new (E_TABLE_MODEL (model), extras, spec, state);
	
	g_free (spec);
	g_free (state);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
	table_connect_signals (table);
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
	gchar *spec;
	
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

	spec = xst_conf_get_string (tool, "net_group_spec");
	if (!spec) {
		spec = xst_ui_load_etspec (tool->etspecs_common_path, NET_GROUP_TABLE_SPEC);
		if (!spec)
			g_error ("create_user_table: Couldn't create table.");
		xst_conf_set_string (tool, "net_group_spec", spec);
	}
	net_group_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, spec, NULL);

	g_free (spec);

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
	gchar *spec;
	
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

	spec = xst_conf_get_string (tool, "net_user_spec");
	if (!spec) {
		spec = xst_ui_load_etspec (tool->etspecs_common_path, NET_USER_TABLE_SPEC);
		if (!spec)
			g_error ("create_user_table: Couldn't create table.");
		xst_conf_set_string (tool, "net_user_spec", spec);
	}
	net_user_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, spec, NULL);

	g_free(spec);
	
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
		if (check_node_visibility (node))
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
tables_update_complexity (XstDialogComplexity complexity)
{
	ETable *u_table, *g_table;
	gchar *user_state, *group_state;
	
	u_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	g_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));

	switch (complexity) {
	case XST_DIALOG_BASIC:
		user_state =  xst_conf_get_string (tool, "user_state_basic");
		group_state = xst_conf_get_string (tool, "group_state_basic");
		break;
	case XST_DIALOG_ADVANCED:
		user_state =  xst_conf_get_string (tool, "user_state_adv");
		group_state = xst_conf_get_string (tool, "group_state_adv");
		break;
	default:
		g_warning ("update_notebook_complexity: Unsupported complexity.");
		return;
	}

	e_table_set_state (u_table, user_state);
	table_connect_signals (u_table);
	
	e_table_set_state (g_table, group_state);
	table_connect_signals (g_table);

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
