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

extern XstTool *tool;

GtkWidget *user_table;
GtkWidget *group_table;
GtkWidget *net_group_table;
GtkWidget *net_user_table;

gint active_table;

/* e-table specifications. */

const gchar *user_spec = "\
<ETableSpecification cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Users\" expansion=\"1.0\" minimum_width=\"40\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"1\" _title=\"UID\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"my_cell\" compare=\"id_compare\"/> \
  <ETableColumn model_col=\"2\" _title=\"Home\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"3\" _title=\"Shell\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"4\" _title=\"Comment\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
  <ETableColumn model_col=\"5\" _title=\"Group\" expansion=\"1.0\" minimum_width=\"80\" resizable=\"true\" cell=\"my_cell\" compare=\"string\"/> \
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
static gchar *get_row_color (ETreeModel *etm, ETreePath *path);

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

static void
set_value_at (ETreeModel *etm, ETreePath *path, int col, const void *val, void *data)
{
}

static void
user_set_value_at (ETreeModel *etm, ETreePath *path, int col, const void *val, void *data)
{
	xmlNodePtr node;
	gchar *field;

	g_return_if_fail (xst_tool_get_access (tool));

	node = e_tree_model_node_get_data (etm, path);
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

	xml_set_child_content (node, field, (gpointer)val);
	g_free (field);

	xst_dialog_modify (tool->main_dialog);
}

static void
group_set_value_at (ETreeModel *etm, ETreePath *path, int col, const void *val, void *data)
{
	xmlNodePtr node;
	gchar *field;

	g_return_if_fail (xst_tool_get_access (tool));

	node = e_tree_model_node_get_data (etm, path);
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

	xml_set_child_content (node, field, (gpointer)val);
	g_free (field);

	xst_dialog_modify (tool->main_dialog);
}

static gboolean
is_editable (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	return (xst_tool_get_access (tool) &&
		   (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED));
}

static xmlNodePtr
get_group_by_id (xmlNodePtr user_node)
{
	/* Dumb funcion, just used in user_value_at, to convert GID to group name.
	 It's here only because I wanted keep user_value_at short. */
	gchar *gid;
	xmlNodePtr group_node;

	gid = xml_get_child_content (user_node, "gid");
	group_node = get_corresp_field (get_db_node (user_node));
	group_node = get_node_by_data (group_node, "gid", gid);

	g_free (gid);
	return (group_node);
}

static void *
user_value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	xmlNodePtr node, gnode;
	gchar *field;

	node = e_tree_model_node_get_data (etm, path);
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
		gnode = get_group_by_id (node);
		if (gnode)
		{
			field = g_strdup ("name");
			node = gnode;
		}

		else
			field = g_strdup ("gid");

		break;
	case COL_USER_COLOR:
		return get_row_color (etm, path);
		break;
	default:
		g_warning ("user_value_at: wrong col nr");
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
	case COL_GROUP_NAME:
		field = g_strdup ("name");
		break;
	case COL_GROUP_GID:
		field = g_strdup ("gid");
		break;
	case COL_USER_COLOR:
		return get_row_color (etm, path);
		break;
	default:
		g_warning ("group_value_at: wrong col nr");
		return NULL;
	}

	node = xml_element_find_first (node, field);
	g_free (field);

	if (!node)
		return NULL;

	return xml_element_get_content (node);
}

static void *
net_group_value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	xmlNodePtr node;

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return NULL;

	return xml_get_child_content (node, "name");
}

static void *
net_user_value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	xmlNodePtr node;

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return NULL;

	return xml_get_child_content (node, "login");
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
	ECell *ec;

	extras = e_table_extras_new ();
	e_table_extras_add_compare (extras, "id_compare", id_compare);

	ec = e_cell_text_new (NULL, GTK_JUSTIFY_CENTER);
	gtk_object_set (GTK_OBJECT (ec), "color_column", COL_USER_COLOR, NULL);
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

	active_table = TABLE_USER;
	
	model = E_TREE_MODEL (table->model);
	path = e_tree_model_node_at_row (model, row);
	node = e_tree_model_node_get_data (model, path);

	node = xml_element_find_first (node, "login");
	buf = xml_element_get_content (node);

	label = g_strconcat (_("Settings for user "), buf, NULL);
	gtk_frame_set_label (GTK_FRAME (xst_dialog_get_widget (tool->main_dialog,
												"user_settings_frame")), label);
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

	active_table = TABLE_GROUP;
	
	model = E_TREE_MODEL (table->model);
	path = e_tree_model_node_at_row (model, row);
	node = e_tree_model_node_get_data (model, path);

	node = xml_element_find_first (node, "name");
	buf = xml_element_get_content (node);

	label = g_strconcat (_("Settings for group "), buf, NULL);
	gtk_frame_set_label (GTK_FRAME (xst_dialog_get_widget (tool->main_dialog,
												"group_settings_frame")), label);
	g_free (label);
	g_free (buf);

	group_actions_set_sensitive (TRUE);
}

static void
net_group_cursor_change (ETable *table, gint row, gpointer user_data)
{
	ETable *u_table;
	ETreeModel *model, *u_model;
	ETreePath *path, *u_root;
	xmlNodePtr node, u_node;
	gchar *name, *buf, *user;

	active_table = TABLE_NET_GROUP;

	/* Get group name */
	model = E_TREE_MODEL (table->model);
	path = e_tree_model_node_at_row (model, row);
	node = e_tree_model_node_get_data (model, path);
	name = xml_get_child_content (node, "name");

	/* Set desc */
	buf = g_strconcat (_("Settings for group "), name, NULL);
	gtk_frame_set_label (GTK_FRAME (xst_dialog_get_widget (tool->main_dialog,
												"network_settings_frame")), buf);
	g_free (buf);
	
	/* Get users table */
	u_table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_user_table));
	u_model = E_TREE_MODEL (u_table->model);
	u_root = e_tree_model_get_root (u_model);
	
	/* Clear net_user_table */
	clear_table (u_model, u_root);
	
	/* Get group users */
	node = xml_element_find_first (node, "users");
	for (node = node->childs; node; node = node->next)
	{
		user = xml_element_get_content (node);

		if (!user)
			continue;

		u_node = get_nis_user_root_node ();
		for (u_node = u_node->childs; u_node; u_node = u_node->next)
		{
			buf = xml_get_child_content (u_node, "login");

			if (!buf)
				continue;

			if (strcmp (buf, user))
			{
				g_free (buf);
				continue;
			}

			e_tree_model_node_insert (u_model, u_root, -1, u_node);
			break;
		}

		g_free (user);
	}
	
	net_actions_set_sensitive (TRUE);
}

static void
net_user_cursor_change (ETable *table, gint row, gpointer user_data)
{
	ETreePath *path;
	ETreeModel *model;
	gchar *buf, *label;
	xmlNodePtr node;

	active_table = TABLE_NET_USER;
	
	model = E_TREE_MODEL (table->model);
	path = e_tree_model_node_at_row (model, row);
	node = e_tree_model_node_get_data (model, path);

	node = xml_element_find_first (node, "login");
	buf = xml_element_get_content (node);

	label = g_strconcat (_("Settings for user "), buf, NULL);
	gtk_frame_set_label (GTK_FRAME (xst_dialog_get_widget (tool->main_dialog,
												"network_settings_frame")), label);
	g_free (label);
	g_free (buf);

	net_actions_set_sensitive (TRUE);
}

static gchar *
get_row_color (ETreeModel *etm, ETreePath *path)
{
	xmlNodePtr node, db_node;
	gchar *buf = NULL;
	gint id, min, max;

	node = e_tree_model_node_get_data (etm, path);
	db_node = get_db_node (node);
	get_min_max (db_node, &min, &max);

	if (!strcmp (db_node->name, "userdb"))
		buf = xml_get_child_content (node, "uid");

	else if (!strcmp (db_node->name, "groupdb"))
		buf = xml_get_child_content (node, "gid");

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
	ETreeModel *model;
	ETreePath *root_path;
	ETableExtras *extras;
	GtkWidget *container;

	extras = create_extras ();

	model = e_tree_simple_new (user_col_count,
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
	gtk_signal_connect (GTK_OBJECT (table), "double_click", on_settings_clicked, NULL);

	container = xst_dialog_get_widget (tool->main_dialog, "users_holder");
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

	model = e_tree_simple_new (group_col_count,
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
	gtk_signal_connect (GTK_OBJECT (table), "double_click", on_settings_clicked, NULL);

	container = xst_dialog_get_widget (tool->main_dialog, "groups_holder");
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
				   net_group_value_at,
				   set_value_at,
				   is_editable,
				   NULL);

	root_path = e_tree_model_node_insert (model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (model, FALSE);

	net_group_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, net_group_spec, NULL);
	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_group_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_activated", net_group_cursor_change, NULL);

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
				   net_user_value_at,
				   set_value_at,
				   is_editable,
				   NULL);

	root_path = e_tree_model_node_insert (model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (model, FALSE);

	net_user_table = e_table_scrolled_new (E_TABLE_MODEL (model), NULL, net_user_spec, NULL);
	table = e_table_scrolled_get_table (E_TABLE_SCROLLED (net_user_table));
	gtk_signal_connect (GTK_OBJECT (table), "cursor_activated", net_user_cursor_change, NULL);

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
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_BASIC)
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
	ETable *table = NULL;

	switch (active_table)
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
delete_selected_node (gint tbl)
{
	ETable *table;
	ETreePath *path;
	ETreeModel *model;
	gint row;

	g_return_val_if_fail (table = get_table (tbl), FALSE);

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
current_table_update_row (ug_data *ud)
{
	ETable *table;
	ETreeModel *model;
	gint row;

	g_return_if_fail (ud != NULL);
	
	table = get_table (ud->table);

	model = E_TREE_MODEL (table->model);
	row = e_table_get_cursor_row (table);

	e_table_model_row_changed (E_TABLE_MODEL (model), row);
}

void
current_table_new_row (ug_data *ud)
{
	ETable *table;
	ETreeModel *model;
	ETreePath *path;

	g_return_if_fail (ud != NULL);

	table = get_table (ud->table);

	model = E_TREE_MODEL (table->model);
	path = e_tree_model_get_root (model);
	e_tree_model_node_insert (model, path, -1, ud->node);
}
