/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of runlevel-admin, a ximian-setup-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "xst.h"
#include "table.h"
#include "callbacks.h"

extern XstTool *tool;
extern GArray *runlevel_array;

extern GdkPixbuf *start_icon;
extern GdkPixbuf *stop_icon;
extern GdkPixbuf *do_nothing_icon;

void
on_main_dialog_update_complexity (GtkWidget *main_dialog, gpointer data)
{
	XstDialogComplexity complexity;
	complexity = XST_DIALOG (main_dialog)->complexity;
	table_update_state (complexity);
}

gboolean
callbacks_conf_read_failed_hook (XstTool *tool, XstReportLine *rline, gpointer data)
{
	GtkWidget *dialog;
	gchar *txt;

	txt = g_strdup_printf (_("The file ``%s'' is missing or could not be read:\nThe configuration will show empty."), rline->argv[0]);
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, txt);
        gtk_dialog_run (GTK_DIALOG (dialog));
	
	g_free (txt);

	return TRUE;
}

void
callbacks_runlevel_toggled (GtkTreeView *treeview, gpointer data)
{
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter iter;
	GdkPixbuf *image;
	gint *col, row;
	gchar *path_str, *level;
	GList *cell_list;
	GtkCellRendererPixbuf *cell;
	xmlNodePtr node, runlevels,rl;
		
	gtk_tree_view_get_cursor (treeview, &path, &column);
	if (column == NULL)
		return;
	path_str = gtk_tree_path_to_string (path);
	cell_list = gtk_tree_view_column_get_cell_renderers (column);
	cell = g_list_nth_data (cell_list, 0);
	g_list_free (cell_list);
	col = g_object_get_data (G_OBJECT (cell), "column");
	if (col == NULL)
		return;
	row = atoi (path_str);
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, *col, &image, -1);
	
	node = g_array_index (runlevel_array, xmlNodePtr, row);
	runlevels = xst_xml_element_find_first (node, "runlevels");
	
	if (image == start_icon)
	{
		// The state turns to stopped
		image = stop_icon;
		
		for (rl = xst_xml_element_find_first (runlevels, "runlevel"); rl != NULL; rl = xst_xml_element_find_next (rl, "runlevel"))
		{
			level = xst_xml_get_child_content (rl, "number");
			if (*col - 1 == atoi (level))
			{
				xst_xml_set_child_content (rl, "action", "stop");
				break;
			}
		}
	}
	else if (image == stop_icon)
	{
		// The state turns to "do nothing"
		image = do_nothing_icon;

		for (rl = xst_xml_element_find_first (runlevels, "runlevel"); rl != NULL; rl = xst_xml_element_find_next (rl, "runlevel"))
		{
			level = xst_xml_get_child_content (rl, "number");
			if (*col - 1 == atoi (level))
			{
				xst_xml_element_destroy (rl);
				break;
			}
		}
	}
	else
	{
		// The state turns to started
		gchar *buf;
		
		image = start_icon;

		if (runlevels == NULL){
			runlevels= xst_xml_element_add (node, "runlevels");
		}
		rl = xst_xml_element_add (runlevels, "runlevel");
		xst_xml_element_add (rl, "number");
		xst_xml_element_add (rl, "action");
		buf = g_strdup_printf ("%i", *col - 1);
		xst_xml_set_child_content (rl, "number", buf);
		xst_xml_set_child_content (rl, "action", "start");
		g_free (buf);
	}

	xst_dialog_modify (tool->main_dialog);	
	
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, *col, image, -1);
	gtk_tree_path_free (path);
	g_free (path_str);
}
