/* network.c: this file is part of users-admin, a helix-setup-tool frontend 
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
#include <gal/e-table/e-tree-simple.h>
#include <gal/e-table/e-tree-model.h>
#include <gal/e-table/e-table-scrolled.h>
#include <gal/e-paned/e-hpaned.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "callbacks.h"

const gchar *group_spec = 
"<ETableSpecification no-headers=\"true\" cursor-mode=\"line\"> \
  <ETableColumn model_col=\"0\" _title=\"Group\" expansion=\"1.0\" minimum_width=\"20\" resizable=\"true\" cell=\"tree-string\" compare=\"string\"/> \
  <ETableState> \
    <column source=\"0\"/> \
    <grouping></grouping> \
  </ETableState> \
</ETableSpecification>";


GtkWidget *network_group;
GtkWidget *network_user;

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
group_value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	xmlNodePtr node;

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return NULL;

	if (!strcmp (node->name, "groupdb"))
		return g_strdup ("Local");

	node = xml_element_find_first (node, "name");
	if (!node)
		return NULL;

	return xml_element_get_content (node);
}

static void *
user_value_at (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
	xmlNodePtr node;

	node = e_tree_model_node_get_data (etm, path);
	if (!node)
		return NULL;

	return xml_element_get_content (node);
}

static void
set_value_at (ETreeModel *etm, ETreePath *path, int col, const void *val, void *data)
{
}

static gboolean
is_editable (ETreeModel *etm, ETreePath *path, int col, void *model_data)
{
        return FALSE;
}

static void
on_group_cursor_change (ETable *table, int row, gpointer user_data)
{
	ETable *u_table;
	ETreeModel *group_model, *user_model;
	ETreePath *path, *user_root, **paths;
	xmlNodePtr node, n0;
	gint i, num_children;
	gchar *name, *txt;

	group_model = E_TREE_MODEL (table->model);

	u_table = E_TABLE_SCROLLED (network_user)->table;
	user_model = E_TREE_MODEL (u_table->model);

	path = e_tree_model_node_at_row (group_model, row);
	node = e_tree_model_node_get_data (group_model, path);

	user_root = e_tree_model_get_root (user_model);
	num_children = e_tree_model_node_get_children (user_model, user_root, &paths);

	e_tree_model_freeze (user_model);

	for (i = 0; i < num_children; i++)
		e_tree_model_node_remove (user_model, paths[i]);

	node = xml_element_find_first (node, "users");
	if (!node)
		return;

	for (n0 = xml_element_find_first (node, "user"); n0; 
			n0 = xml_element_find_next (n0, "user"))
	{
		e_tree_model_node_insert (user_model, user_root, -1, n0);
	}

	e_tree_model_thaw (user_model);

	name = group_value_at (group_model, path, row, NULL);
	txt = g_strdup_printf ("Settings for group %s", name);
	gtk_frame_set_label (GTK_FRAME (tool_widget_get ("network_settings_frame")), txt);
	g_free (txt);
}

static void
on_user_cursor_change (ETable *table, int row, gpointer user_data)
{
	ETreeModel *group_model;
	ETreePath *path;
	gchar *name, *txt;
	ETable *g;

	g = E_TABLE_SCROLLED (network_group)->table;
	e_table_selection_model_clear (g->selection);

	group_model = E_TREE_MODEL (table->model);
	path = e_tree_model_node_at_row (group_model, row);

	name = user_value_at (group_model, path, row, NULL);
	txt = g_strdup_printf ("Settings for user %s", name);
	gtk_frame_set_label (GTK_FRAME (tool_widget_get ("network_settings_frame")), txt);
	g_free (txt);
}

static void
create_group_table (GtkWidget *paned)
{
	ETreeModel *group_model;
	ETreePath *root_node;

	group_model = e_tree_simple_new (col_count,
					 duplicate_value,
					 freeze_value,
					 initialize_value,
					 value_is_empty,
					 value_to_string,
					 icon_at,
					 group_value_at,
					 set_value_at,
					 is_editable,
					 NULL);

	root_node = e_tree_model_node_insert (group_model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (group_model, FALSE);

	network_group = e_table_scrolled_new (E_TABLE_MODEL (group_model), NULL, group_spec, NULL);
	gtk_signal_connect (GTK_OBJECT (E_TABLE_SCROLLED (network_group)->table), "cursor_change",
			GTK_SIGNAL_FUNC (on_group_cursor_change), NULL);

	gtk_container_add (GTK_CONTAINER (paned), network_group);
	gtk_widget_show (network_group);
}

static void
create_user_table (GtkWidget *paned)
{
	ETreeModel *user_model;
	ETreePath *root_node;

	user_model = e_tree_simple_new (col_count,
					 duplicate_value,
					 freeze_value,
					 initialize_value,
					 value_is_empty,
					 value_to_string,
					 icon_at,
					 user_value_at,
					 set_value_at,
					 is_editable,
					 NULL);

	root_node = e_tree_model_node_insert (user_model, NULL, 0, g_strdup (""));
	e_tree_model_root_node_set_visible (user_model, FALSE);

	network_user = e_table_scrolled_new (E_TABLE_MODEL (user_model), NULL, group_spec, NULL);
	gtk_signal_connect (GTK_OBJECT (E_TABLE_SCROLLED (network_user)->table), "cursor_change",
			GTK_SIGNAL_FUNC (on_user_cursor_change), NULL);

	gtk_container_add (GTK_CONTAINER (paned), network_user);
	gtk_widget_show (network_user);
}

static void
create_tables (GtkWidget *paned)
{
	create_group_table (paned);
	create_user_table (paned);
}

extern void
network_create (void)
{
	GtkWidget *paned, *container;

	paned = e_hpaned_new ();
	container = tool_widget_get ("network_placeholder");

	gtk_container_add (GTK_CONTAINER (container), paned);
	gtk_widget_show (paned);

	create_tables (paned);
}

extern void
network_populate (xmlNodePtr xml_root)
{
	ETable *table;
	ETreeModel *model;
	ETreePath *path_root, *path;
	xmlNodePtr node, n0;

	g_return_if_fail (xml_root != NULL);

	table = E_TABLE_SCROLLED (network_group)->table;
	model = E_TREE_MODEL (table->model);
	path_root = e_tree_model_get_root (model);

	e_tree_model_freeze (model);

	node = xml_element_find_first (xml_root, "groupdb");
	if (!node)
	{
		g_warning ("network_populate: couldn't find groupdb node.");
		return;
	}

	path = e_tree_model_node_insert (model, path_root, -1, node);

	for (n0 = xml_element_find_first (node, "group"); n0; 
			n0 = xml_element_find_next (n0, "group"))
	{
		e_tree_model_node_insert (model, path, -1, n0);
	}

	e_tree_model_thaw (model);
}

