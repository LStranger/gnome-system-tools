/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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
 * Authors: Chema Celorio <chema@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>

#include "gst.h"

#include "callbacks.h"
#include "transfer.h"
#include "hosts.h"

/* Yes, I don't like globals & externs we should really have
   an GstHostsPageInfo struct with all the stuff but it does
   not work with our signals connecting system */
extern GstTool *tool;

static char *
fixdown_text_list (char *s)
{
	char *s2;

	g_return_val_if_fail (s != NULL, NULL);

	for (s2 = (gchar *) strchr (s, ' '); s2; s2 = (gchar *) strchr (s2, ' '))
		*s2 = '\n';

	return s;
}

/**
 * gst_hosts_ip_is_in_list:
 * @ip_str: 
 * 
 * Determines is @ip_str is already in the list. We should keep a GList if
 * the ip's in the lists really, not have to query the view to get the data
 * 
 * Return Value: TRUE, if in the list, FALSE otherwise.
 **/
gboolean
gst_hosts_ip_is_in_list (const gchar *ip_str)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gboolean         valid;
	gchar           *buf;
	GstStatichostUI *ui;

	if (!ip_str || strlen (ip_str) == 0)
		return FALSE;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP, &buf, -1);

		if (!strcmp (buf, ip_str)) {
			g_free (buf);
			return TRUE;
		}

		g_free (buf);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	return FALSE;
}

void
gst_hosts_update_sensitivity (void)
{
	gboolean delete;
	gboolean add;
	gboolean ip_is_in_list;
	gchar *ip_str;
	gchar *alias_str;
	GstStatichostUI *ui;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);

	/* Get the texts */
	ip_str    = gtk_editable_get_chars (GTK_EDITABLE (ui->ip), 0, -1);
	alias_str = gst_ui_text_view_get_text (GTK_TEXT_VIEW (ui->alias));
	ip_is_in_list = gst_hosts_ip_is_in_list (ip_str);

	/* DELETE : You can delete if the row is selected and the ip is in the list of ip's
	 * and also that the ip is not the loopback ip address. FIXME
	 * ADD: You can add when the ip is not in the clist already,
	 */
	delete = (ip_is_in_list) && strcmp (ip_str, "127.0.0.1");
	add = (strlen(ip_str) > 0) && (!ip_is_in_list);

	/* Set the states */
	gtk_widget_set_sensitive (ui->button_delete, delete);
	gtk_widget_set_sensitive (ui->button_add,    add);

	g_free (ip_str);
	g_free (alias_str);
}

static GtkTreeModel *
statichost_list_model_new (void)
{
	GtkListStore *store;

	store = gtk_list_store_new (STATICHOST_LIST_COL_LAST,
				    G_TYPE_STRING,
				    G_TYPE_STRING);

	return GTK_TREE_MODEL (store);
}

static void
statichost_list_add_columns (GtkTreeView *treeview)
{
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	GtkTreeModel      *model = gtk_tree_view_get_model (treeview);

	/* IP */
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("IP Address"), cell,
							"text", STATICHOST_LIST_COL_IP, NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_sort_column_id (col, 0);
	gtk_tree_view_append_column (treeview, col);

	/* Aliases */
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Aliases"), cell,
							"text", STATICHOST_LIST_COL_ALIAS, NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_sort_column_id (col, 1);
	gtk_tree_view_append_column (treeview, col);
}

static void
statichost_list_select_row (GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter      iter;
	GtkTreeModel    *model;
	gchar           *buf;
	gint             pos;
	GstStatichostUI *ui;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		/* Selection exists */
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP, &buf, -1);
		
		pos = 0;
		gtk_editable_delete_text (GTK_EDITABLE (ui->ip), 0, -1);
		gtk_editable_insert_text (GTK_EDITABLE (ui->ip), buf, strlen (buf), &pos);
		g_free (buf);

		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_ALIAS, &buf, -1);
		buf = fixdown_text_list (buf);

		/* block the "changed" signal from the alias text view */
		g_signal_handlers_block_by_func (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (ui->alias))),
						 G_CALLBACK (on_hosts_alias_changed), NULL);

		gst_ui_text_view_clear (GTK_TEXT_VIEW (ui->alias));
		gtk_text_buffer_set_text (GTK_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (ui->alias))),
					  buf, -1);
		g_free (buf);

		/* unblock again the "changed" signal again */
		g_signal_handlers_unblock_by_func (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (ui->alias))),
						   G_CALLBACK (on_hosts_alias_changed), NULL);

	} else {
		/* Unselect row */
		gst_ui_text_view_clear (GTK_TEXT_VIEW (ui->alias));

		/* Load aliases into entry widget */
		gtk_editable_delete_text (GTK_EDITABLE (ui->ip), 0, -1);
	}

	gst_hosts_update_sensitivity ();
}

static GtkWidget *
statichost_list_new (GstTool *tool)
{
	GtkWidget        *treeview;
	GtkTreeSelection *select;
	GtkTreeModel     *model;

	model = statichost_list_model_new ();

	treeview = gst_dialog_get_widget (tool->main_dialog, "statichost_list");
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (G_OBJECT (model));

	statichost_list_add_columns (GTK_TREE_VIEW (treeview));

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (statichost_list_select_row), NULL);

	gtk_widget_show_all (treeview);

	return treeview;
}

void
hosts_init_gui (GstTool *tool)
{
	GstStatichostUI *ui;

	ui = g_new0 (GstStatichostUI, 1);

	ui->list = statichost_list_new (tool);

	ui->ip = gst_dialog_get_widget (tool->main_dialog, "ip");
	ui->alias = gst_dialog_get_widget (tool->main_dialog, "alias");
	ui->button_delete = gst_dialog_get_widget (tool->main_dialog, "statichost_delete");
	ui->button_add = gst_dialog_get_widget (tool->main_dialog, "statichost_add");

	g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (ui->alias))),
			  "changed", G_CALLBACK (on_hosts_alias_changed), NULL);

	g_object_set_data (G_OBJECT (tool), STATICHOST_UI_STRING, (gpointer) ui);
}

void
hosts_list_clear (GstTool *tool)
{
	GstStatichostUI *ui;
	GtkTreeModel    *model;

	g_return_if_fail (tool != NULL);

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	gtk_list_store_clear (GTK_LIST_STORE (model));

}

void
hosts_list_append (GstTool *tool, const gchar *text[])
{
	GstStatichostUI *ui;
	GtkTreeModel    *model;
	GtkTreeIter      iter;

	g_return_if_fail (text != NULL);
	g_return_if_fail (text[0] != NULL);
	g_return_if_fail (text[1] != NULL);

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	if (!gst_hosts_ip_is_in_list (text[STATICHOST_LIST_COL_IP]))
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    STATICHOST_LIST_COL_IP, text[STATICHOST_LIST_COL_IP],
			    STATICHOST_LIST_COL_ALIAS, text[STATICHOST_LIST_COL_ALIAS],
			    -1);
}

void
hosts_list_remove (GstTool *tool, const gchar *ip)
{
	GstStatichostUI *ui;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gchar           *buf;
	gboolean         valid;

	g_return_if_fail (ip != NULL);

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP, &buf, -1);
		if (!strcmp (ip, buf)) {
			g_free (buf);
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			break;
		}

		g_free (buf);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}

static gboolean
hosts_list_get (const gchar *ip, gchar *ret_buf[])
{
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gboolean          valid;
	GstStatichostUI  *ui;
	gchar            *tmp_ip;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	ret_buf[0] = ret_buf[1] = NULL;

	if (ip == NULL || strlen (ip) == 0) {
		GtkTreeSelection *select;

		select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));
		if (gtk_tree_selection_get_selected (select, &model, &iter))
			gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP, &tmp_ip, -1);
		else {
			g_warning ("host_list_get: none selected, none specified");
			return FALSE;
		}
	} else
		tmp_ip = (gchar *)ip;

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP,
				    &ret_buf[STATICHOST_LIST_COL_IP], -1);

		if (!strcmp (ret_buf[STATICHOST_LIST_COL_IP], tmp_ip)) {
			gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_ALIAS,
					    &ret_buf[STATICHOST_LIST_COL_ALIAS], -1);
			return TRUE;
		}

		g_free (ret_buf[STATICHOST_LIST_COL_IP]);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	return FALSE;
}

gboolean
hosts_list_get_selected (gchar **ip, gchar **alias)
{
	gchar *entry[STATICHOST_LIST_COL_LAST];

	if (hosts_list_get (NULL, entry)) {
		*ip = entry[STATICHOST_LIST_COL_IP];
		*alias = entry[STATICHOST_LIST_COL_ALIAS];

		return TRUE;
	}

	return FALSE;
}

void
hosts_list_save (GstTool *tool, xmlNodePtr root)
{
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GstStatichostUI  *ui;
	xmlNodePtr        node, node2;
	gboolean          valid, col0_added;
	gint              j;
	gchar            *ip, *alias, **col1_elem;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	/* First remove any old branches in the XML tree */
	gst_xml_element_destroy_children_by_name (root, "statichost");

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP, &ip, -1);
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_ALIAS, &alias, -1);

		/* Enclosing element */
		node = gst_xml_element_add (root, "statichost");

		col1_elem = g_strsplit (alias, " ", 0);

		for (j = 0, col0_added = FALSE; col1_elem[j]; j++) {
			if (!strlen (col1_elem[j]))
				continue;
			if (!col0_added) {
				node2 = gst_xml_element_add (node, "ip");
				gst_xml_element_set_content (node2, ip);
				col0_added = TRUE;
			}
			node2 = gst_xml_element_add (node, "alias");
			gst_xml_element_set_content (node2, col1_elem[j]);
		}

		g_free (ip);
		g_free (alias);
		g_strfreev (col1_elem);

		valid = gtk_tree_model_iter_next (model, &iter);
	}
}
