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
#include <gal/e-table/e-tree-simple.h>
#include <gal/e-table/e-tree-model.h>
#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-table/e-cell-text.h>
#include <gal/e-paned/e-hpaned.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "e-table.h"
#include "user_group.h"
#include "callbacks.h"

GtkWidget *user_table;
GtkWidget *group_table;
GtkWidget *net_group_table;
GtkWidget *net_user_table;

/* e-table specifications. */

const gchar *user_spec = "\
<ETableSpecification cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Users\" expansion=\"1.0\" minimum_width=\"40\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"UID\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"my_cell\" compare=\"id_compare\"/> \
  <ETableColumn model_col=\"2\" _title=\"Home\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"3\" _title=\"Shell\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"4\" _title=\"Comment\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
</ETableSpecification>";

const gchar *group_spec = "\
<ETableSpecification cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Groups\" expansion=\"1.0\" minimum_width=\"60\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"GID\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"my_cell\" compare=\"id_compare\"/> \
</ETableSpecification>";

const gchar *net_group_spec = "\
<ETableSpecification no-headers=\"true\" cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Group\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"tree-string\" compare=\"string\"/> \
  <ETableState> \
    <column source=\"0\"/> \
    <grouping></grouping> \
  </ETableState> \
</ETableSpecification>";

const gchar *net_user_spec = "\
<ETableSpecification no-headers=\"true\" cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"User\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"tree-string\" compare=\"string\"/> \
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


static int
col_count (ETableModel *etm, void *data)
{
        return 1;
}

static void *
duplicate_value (ETableModel *etm, int col, const void *value, void *data)
{
        return g_strdup (value);
}

static void
freeze_value (ETableModel *etm, int col, void *value, void *data)
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
        return g_strdup (value);
}

static GdkPixbuf *
icon_at (ETreeModel *etm, ETreePath *path, void *model_data)
{
        return NULL;
}

static void *
value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	return NULL;
}

static void
set_value_at (ETreeModel *etm, ETreePath *path, int col, const void *val, void *data)
{
}

static void
user_set_value_at (ETreeModel *etm, ETreePath *path, int col, const void *val, void *data)
{
	xmlNodePtr node;
	gchar *field;

	g_return_if_fail (tool_get_access ());

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return;

	switch (col)
	{
		case 0:
			if (!check_user_login (node, (gpointer)val))
				return;

			field = g_strdup ("login");
			break;
		case 1:
			if (!check_user_uid (node, (gpointer)val))
				return;

			field = g_strdup ("uid");
			break;
		case 2:
			if (!check_user_home (node, (gpointer)val))
				return;

			field = g_strdup ("home");
			break;
		case 3:
			if (!check_user_shell (node, (gpointer)val))
				return;

			field = g_strdup ("shell");
			break;
		case 4:
			if (!check_user_comment (node, (gpointer)val))
				return;

			field = g_strdup ("comment");
			break;
		default:
			g_warning ("user_set_value_at: wrong col nr: %d", col);
			return;
	}

	xml_set_child_content (node, field, (gpointer)val);
	g_free (field);

	tool_set_modified(TRUE);
}

static void
group_set_value_at (ETreeModel *etm, ETreePath *path, int col, const void *val, void *data)
{
	xmlNodePtr node;
	gchar *field;

	g_return_if_fail (tool_get_access ());

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return;

	switch (col)
	{
		case 0:
			if (!check_group_name (node, (gpointer)val))
				return;

			field = g_strdup ("name");
			break;
		case 1:
			if (!check_group_gid (node, (gpointer)val))
				return;

			field = g_strdup ("gid");
			break;
		default:
			g_warning ("group_set_value_at: wrong col nr: %d", col);
			return;
	}

	xml_set_child_content (node, field, (gpointer)val);
	g_free (field);

	tool_set_modified (TRUE);
}

static gboolean
is_editable (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	return (tool_get_access () && (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED));
}

static void *
user_value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	xmlNodePtr node;
	gchar *field;

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return NULL;

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
			g_warning ("user_value_at: wrong col nr: %d", col);
			return NULL;
	}

	node = xml_element_find_first (node, field);
	g_free (field);

	if (!node)
		return NULL;

	return xml_element_get_content (node);
}

static void *
group_value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	xmlNodePtr node;
	gchar *field;

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return NULL;

	switch (col)
	{
		case 0:
			field = g_strdup ("name");
			break;
		case 1:
			field = g_strdup ("gid");
			break;
		default:
			g_warning ("group_value_at: wrong col nr: %d", col);
			return NULL;
	}

	node = xml_element_find_first (node, field);
	g_free (field);

	if (!node)
		return NULL;

	return xml_element_get_content (node);
}


static GtkWidget *
create_container (void)
{
	GtkWidget *paned, *container;

	paned = e_hpaned_new ();
	container = tool_widget_get ("network_placeholder");

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
	ECell *ec;

	extras = e_table_extras_new ();
	e_table_extras_add_compare (extras, "id_compare", id_compare);

	ec = e_cell_text_new (NULL, GTK_JUSTIFY_CENTER);
	e_table_extras_add_cell (extras, "my_cell", ec);
	
	return extras;
}
static void
user_cursor_change (ETable *table, gint row, gpointer user_data)
{
	ETreePath *path;
	ETreeModel *model;
	gchar *buf, *label;
	xmlNodePtr node;

	model = E_TREE_MODEL (table->model);
	path = e_tree_model_node_at_row (model, row);
	node = e_tree_model_node_get_data (model, path);

	node = xml_element_find_first (node, "login");
	buf = xml_element_get_content (node);

	label = g_strconcat ("Settings for user ", buf, NULL);
	gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")), label);
	g_free (label);
	g_free (buf);

	user_actions_set_sensitive (TRUE);
}

static void
group_cursor_change (ETable *table, gint row, gpointer user_data)
{
	ETreePath *path;
	ETreeModel *model;
	gchar *buf, *label;
	xmlNodePtr node;

	model = E_TREE_MODEL (table->model);
	path = e_tree_model_node_at_row (model, row);
	node = e_tree_model_node_get_data (model, path);

	node = xml_element_find_first (node, "name");
	buf = xml_element_get_content (node);

	label = g_strconcat ("Settings for group ", buf, NULL);
	gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")), label);
	g_free (label);
	g_free (buf);

	group_actions_set_sensitive (TRUE);
}

static void
net_group_cursor_change (ETable *table, gint row, gpointer user_data)
{
}

static void
net_user_cursor_change (ETable *table, gint row, gpointer user_data)
{
}

static void
create_user_table (void)
{
	ETable *table;
	ETreeModel *model;
	ETreePath *root_path;
	ETableExtras *extras;
	GtkWidget *container;

	extras = create_extras ();

	model = e_tree_simple_new (col_count,
				   duplicate_value,
				   freeze_value,
				   initialize_value,
				   value_is_empty,
				   value_to_string,
				   icon_at,
				   user_value_at,
				   user_set_value_at,
				   is_editable,
				   NULL);

	root_path = e_tree_model_node_insert (model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (model, FALSE);

	user_table = e_table_scrolled_new (E_TABLE_MODEL (model), extras, user_spec,
			basic_user_state);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", user_cursor_change, NULL);
	gtk_signal_connect (GTK_OBJECT (table), "double_click", on_user_settings_clicked, NULL);

	container = tool_widget_get ("users_holder");
	gtk_container_add (GTK_CONTAINER (container), user_table);
	gtk_widget_show (user_table);
}

static void
create_group_table (void)
{
	ETable *table;
	ETreeModel *model;
	ETreePath *root_path;
	ETableExtras *extras;
	GtkWidget *container;

	extras = create_extras ();

	model = e_tree_simple_new (col_count,
				   duplicate_value,
				   freeze_value,
				   initialize_value,
				   value_is_empty,
				   value_to_string,
				   icon_at,
				   group_value_at,
				   group_set_value_at,
				   is_editable,
				   NULL);

	root_path = e_tree_model_node_insert (model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (model, FALSE);

	group_table = e_table_scrolled_new (E_TABLE_MODEL (model), extras, group_spec,
			basic_group_state);

	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", group_cursor_change, NULL);
	gtk_signal_connect (GTK_OBJECT (table), "double_click", on_group_settings_clicked, NULL);

	container = tool_widget_get ("groups_holder");
	gtk_container_add (GTK_CONTAINER (container), group_table);
	gtk_widget_show (group_table);
}

static void
create_network_group_table (GtkWidget *paned)
{
	ETable *table;
	ETreeModel *model;
	ETreePath *root_path;

	model = e_tree_simple_new (col_count,
				   duplicate_value,
				   freeze_value,
				   initialize_value,
				   value_is_empty,
				   value_to_string,
				   icon_at,
				   value_at,
				   set_value_at,
				   is_editable,
				   NULL);

	root_path = e_tree_model_node_insert (model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (model, FALSE);

	net_group_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, net_group_spec, NULL);
	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_group_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", net_group_cursor_change, NULL);

	gtk_container_add (GTK_CONTAINER (paned), net_group_table);
	gtk_widget_show (net_group_table);
}

static void
create_network_user_table (GtkWidget *paned)
{
	ETable *table;
	ETreeModel *model;
	ETreePath *root_path;

	model = e_tree_simple_new (col_count,
				   duplicate_value,
				   freeze_value,
				   initialize_value,
				   value_is_empty,
				   value_to_string,
				   icon_at,
				   value_at,
				   set_value_at,
				   is_editable,
				   NULL);

	root_path = e_tree_model_node_insert (model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (model, FALSE);

	net_user_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, net_user_spec, NULL);
	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_user_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_change", net_user_cursor_change, NULL);

	gtk_container_add (GTK_CONTAINER (paned), net_user_table);
	gtk_widget_show (net_user_table);
}

void
clear_table (ETreeModel *model, ETreePath *root_path)
{
	gint num_children, i;
	ETreePath **paths;

	g_return_if_fail (model != NULL);
	g_return_if_fail (root_path != NULL);

	num_children = e_tree_model_node_get_children (model, root_path, &paths);

	e_tree_model_freeze (model);
	for (i = 0; i < num_children; i++)
		e_tree_model_node_remove (model, paths[i]);

	e_tree_model_thaw (model);
}

void
clear_all_tables (void)
{
	ETable *table[4];
	ETreeModel *model;
	ETreePath *path;
	gint i;

	table[0] = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
	table[1] = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
	table[2] = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_group_table));
	table[3] = NULL;

	for (i = 0; table[i]; i++)
	{
		model = E_TREE_MODEL (table[i]->model);
		path = e_tree_model_get_root (model);
		clear_table (model, path);
	}
}

void
populate_table (ETreeModel *model, ETreePath *root_path, xmlNodePtr root_node)
{
	xmlNodePtr node;

	g_return_if_fail (model != NULL);
	g_return_if_fail (root_path != NULL);
	g_return_if_fail (root_node != NULL);

	e_tree_model_freeze (model);
	for (node = root_node->childs; node; node = node->next)
	{
		if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
			if (!check_node_complexity (node))
				continue;

		e_tree_model_node_insert (model, root_path, -1, node);
	}

	e_tree_model_thaw (model);
}

void
populate_all_tables (void)
{
	ETable *table[4];
	ETreeModel *model;
	ETreePath *path;
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
	{
		model = E_TREE_MODEL (table[i]->model);
		path = e_tree_model_get_root (model);
		populate_table (model, path, node[i]);
	}
}

extern void
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

	/* Popuplate tables */
	populate_all_tables ();
}

void
tables_set_state (gboolean state)
{
	ETable *u_table;
	ETable *g_table;

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

	e_table_model_changed (u_table->model);
	e_table_model_changed (g_table->model);
}

static ETable *
get_current_table (void)
{
	GtkWidget *w0;
	ETable *table = NULL;
	gint active_page;

	w0 = tool_widget_get ("users_admin");
	active_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (w0));

	switch (active_page)
	{
		case 0:
			table = e_table_scrolled_get_table (E_TABLE_SCROLLED (user_table));
			break;
		case 1:
			table = e_table_scrolled_get_table (E_TABLE_SCROLLED (group_table));
			break;
		default:
			return NULL;
	}

	return table;
}

xmlNodePtr
get_selected_node (void)
{
	ETable *table;
	ETreePath *path;
	ETreeModel *model;
	gint row;

	g_return_val_if_fail (table = get_current_table (), NULL);

	if ((row = e_table_get_cursor_row (table)) >= 0)
	{
		model = E_TREE_MODEL (table->model);
		path = e_tree_model_node_at_row (model, row);
		return e_tree_model_node_get_data (model, path);
	}

	return NULL;
}

gboolean
delete_selected_node (void)
{
	ETable *table;
	ETreePath *path;
	ETreeModel *model;
	gint row;

	g_return_val_if_fail (table = get_current_table (), FALSE);

	if ((row = e_table_get_cursor_row (table)) >= 0)
	{
		model = E_TREE_MODEL (table->model);
		path = e_tree_model_node_at_row (model, row);
		e_tree_model_node_remove (model, path);
		return TRUE;
	}

	return FALSE;
}

void
current_table_update_row (void)
{
	ETable *table;
	ETreeModel *model;
	gint row;

	g_return_if_fail (table = get_current_table ());

	model = E_TREE_MODEL (table->model);
	row = e_table_get_cursor_row (table);

	e_table_model_row_changed (E_TABLE_MODEL (model), row);
}

void
current_table_new_row (xmlNodePtr node)
{
	ETable *table;
	ETreeModel *model;
	ETreePath *path;

	g_return_if_fail (node != NULL);
	g_return_if_fail (table = get_current_table ());

	model = E_TREE_MODEL (table->model);
	path = e_tree_model_get_root (model);
	e_tree_model_node_insert (model, path, -1, node);
}

