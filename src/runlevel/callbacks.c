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
        
	dialog = gnome_error_dialog_parented (txt, GTK_WINDOW (tool->main_dialog));
	gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
	
	g_free (txt);

	return TRUE;
}

void
callbacks_runlevel_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	gboolean runlevel_toggle_item;
	gint *column;
	
	gint row = atoi(path_str);
	xmlNodePtr node, runlevels,rl;
	gchar *level, *buf;
	
	node = g_array_index (runlevel_array, xmlNodePtr, row);
	runlevels = xst_xml_element_find_first (node, "runlevels");
	
	column = g_object_get_data (G_OBJECT (cell), "column");
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, *column, &runlevel_toggle_item, -1);
	
	if (runlevel_toggle_item == TRUE)  // if we are deleting a service
	{
		for (rl = xst_xml_element_find_first (runlevels, "runlevel"); rl != NULL; rl = xst_xml_element_find_next (rl, "runlevel"))
		{
			level = xst_xml_element_get_content (rl);
			if (*column - 1 == (level[0] - '0'))
			{
				xst_xml_element_destroy (rl);
				break;
			}
		}
		
		runlevel_toggle_item = FALSE;
	}
	else // if we are adding a service
	{
		if (runlevels == NULL){
			runlevels= xst_xml_element_add (node, "runlevels");
		}
		rl = xst_xml_element_add (runlevels, "runlevel");
		buf = g_strdup_printf ("%i",*column-1);
		xst_xml_element_set_content (rl, buf);

		runlevel_toggle_item = TRUE;
	}
	
	xst_dialog_modify (tool->main_dialog);	
	
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, *column, runlevel_toggle_item, -1);
	gtk_tree_path_free (path);
}
