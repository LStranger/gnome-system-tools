/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* e-table.c: this file is part of runlevel-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho <garparr@teleline.es>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <gtk/gtk.h>

#include "xst.h"

#include "table.h"
#include "callbacks.h"

typedef struct TreeItem_ TreeItem;
	
struct TreeItem_
{
	const char *service;
	GdkPixbuf *level_0;
	GdkPixbuf *level_1;
	GdkPixbuf *level_2;
	GdkPixbuf *level_3;
	GdkPixbuf *level_4;
	GdkPixbuf *level_5;
	GdkPixbuf *level_6;
	
	TreeItem *children;
};

extern XstTool *tool;

GtkWidget *runlevel_table;
GArray *runlevel_array;

// Images used in table
GdkPixbuf *start_icon;
GdkPixbuf *stop_icon;
GdkPixbuf *do_nothing_icon;

static void
add_columns (GtkTreeView *treeview)
{
	gint i, level,*col;
	gchar *label;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	
	// Service name column
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
	                                             -1,
	                                             "Service",
	                                             renderer,
	                                             "text",
	                                             COL_SERVICE,
	                                             NULL);
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), COL_SERVICE);
	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (column), 200);
	
	// Runlevel columns
	for (i=1; i<=7; i++)
	{
		renderer = gtk_cell_renderer_pixbuf_new ();
		g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
		
		col = (gint *) g_malloc (sizeof (gint));
		*col = i;
		g_object_set_data (G_OBJECT (renderer), "column", col);

		level = i-1;
		label = g_strdup_printf ("Level %i",level);
		
		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(treeview),
		                                            -1,
		                                            label,
		                                            renderer,
		                                            "pixbuf",
		                                            i,
		                                            NULL);
	}
}

static GtkTreeModel*
create_model (void)
{
	GtkTreeStore *model;

	model = gtk_tree_store_new (COL_LEVEL6 + 1,
	                            G_TYPE_STRING,
	                            GDK_TYPE_PIXBUF,
	                            GDK_TYPE_PIXBUF,
	                            GDK_TYPE_PIXBUF,
	                            GDK_TYPE_PIXBUF,
	                            GDK_TYPE_PIXBUF,
	                            GDK_TYPE_PIXBUF,
	                            GDK_TYPE_PIXBUF);
	return GTK_TREE_MODEL(model);
}

static void
pixmaps_create (void)
{
	start_icon = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/gnome-light-on.png", NULL);
	stop_icon = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/gnome-light-off.png", NULL);
	do_nothing_icon = gtk_widget_render_icon (runlevel_table, GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU, NULL);
}

GtkWidget*
table_create (void)
{
	GtkTreeModel *model;
	
	
	model = create_model ();
	
	runlevel_table = gtk_tree_view_new_with_model (model);
	g_object_unref (G_OBJECT (model));
	
	g_signal_connect (G_OBJECT (runlevel_table), "cursor-changed", G_CALLBACK (callbacks_runlevel_toggled), model);
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (runlevel_table), TRUE);
	
	add_columns (GTK_TREE_VIEW (runlevel_table));

	// Creates the pixmaps that are going to be used in the table
	pixmaps_create();

	return runlevel_table;
}

static gchar*
table_value_service(xmlNodePtr node)
{
	gchar *buf;
	g_return_val_if_fail (node != NULL, NULL);
	buf = xst_xml_get_child_content (node, "name");
	if (!buf)
		buf = xst_xml_get_child_content (node, "script");
	return buf;
}

static GdkPixbuf*
table_value_runlevel (xmlNodePtr node,gint runlevel)
{
	xmlNodePtr runlevels, rl;
	gchar *action, *number;
	g_return_val_if_fail (node != NULL, NULL);
	
	runlevels = xst_xml_element_find_first (node, "runlevels");
	
	if (runlevels == NULL)
		return do_nothing_icon;

	for (rl = xst_xml_element_find_first (runlevels, "runlevel"); rl != NULL; rl = xst_xml_element_find_next (rl, "runlevel"))
	{
		number = xst_xml_get_child_content (rl, "number");
		if (atoi (number) == runlevel)
		{
			action = xst_xml_get_child_content (rl, "action");
			if (strcmp (action, "start") == 0)
			{
				g_free (number);
				g_free (action);
				return start_icon;
			}
			else
			{
				g_free (number);
				g_free (action);
				return stop_icon;
			}
		}
		g_free (number);
	}
	return do_nothing_icon;
}


static TreeItem*
get_node_data (xmlNodePtr service)
{
	TreeItem *item = g_malloc (sizeof(TreeItem));
	
	item->service = table_value_service (service);
	
	item->level_0 = table_value_runlevel (service, 0);
	item->level_1 = table_value_runlevel (service, 1);
	item->level_2 = table_value_runlevel (service, 2);
	item->level_3 = table_value_runlevel (service, 3);
	item->level_4 = table_value_runlevel (service, 4);
	item->level_5 = table_value_runlevel (service, 5);
	item->level_6 = table_value_runlevel (service, 6);
	
	return item;
}

void 
table_populate (xmlNodePtr root)
{
	xmlNodePtr service,services;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(runlevel_table));
	
	g_return_if_fail (root != NULL);
	
	runlevel_array = g_array_new (FALSE, FALSE, sizeof (xmlNodePtr));
	
	services = xst_xml_element_find_first (root, "services");
	
	for (service = xst_xml_element_find_first (services, "service"); service != NULL; service = xst_xml_element_find_next (service, "service"))
	{
		TreeItem *item;
		
		item = get_node_data (service);
		
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model),
		                    &iter,
		                    COL_SERVICE, item->service,
		                    COL_LEVEL0, item->level_0,
		                    COL_LEVEL1, item->level_1,
		                    COL_LEVEL2, item->level_2,
		                    COL_LEVEL3, item->level_3,
		                    COL_LEVEL4, item->level_4,
		                    COL_LEVEL5, item->level_5,
		                    COL_LEVEL6, item->level_6,
		                    -1);
		g_array_append_val (runlevel_array, service);
	}
	
}

void
table_construct (XstTool *tool)
{
	GtkWidget *sw;
	GtkWidget *list;

	sw = xst_dialog_get_widget (tool->main_dialog, "runlevel_table");
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_ETCHED_IN);

	list = table_create ();
	gtk_widget_show_all (list);
	gtk_container_add (GTK_CONTAINER (sw), list);
}

void
table_update_state (XstDialogComplexity complexity)
{
	gchar *default_runlevel, *rl;
	GtkTreeViewColumn *column;
	GtkTreeView *treeview = GTK_TREE_VIEW (runlevel_table);
	gint i;
	
	default_runlevel = xst_conf_get_string (tool, "default_runlevel");
	if (complexity == XST_DIALOG_BASIC)
	{
		for (i=0; i<=6; i++)
		{
			rl = g_strdup_printf ("%i",i);
			column = gtk_tree_view_get_column (treeview,i+1);
			
			if ((default_runlevel == NULL) || strcmp (default_runlevel, rl)!=0) {
				gtk_tree_view_column_set_visible (column, FALSE);
			} else if (strcmp (default_runlevel, rl) == 0) {
				gtk_tree_view_column_set_visible (column, TRUE);
			}
			
			g_free (rl);
		}
	}
	else
	{
		for (i=0; i<=6; i++)
		{
			column = gtk_tree_view_get_column (treeview,i+1);
			gtk_tree_view_column_set_visible (column, TRUE);
		}
	}
	
	g_free (default_runlevel);
}

void 
table_update_headers (xmlNodePtr root)
{
	XstDialogComplexity complexity;
	gchar *buf, *default_runlevel;
	
	complexity = XST_DIALOG (tool->main_dialog)->complexity;
        
	buf = xst_xml_get_child_content (root, "runlevel");
        
	default_runlevel = g_strdup (buf);
	xst_conf_set_string (tool, "default_runlevel", default_runlevel);
	if (complexity == XST_DIALOG_BASIC) {
		table_update_state (complexity);
	}
		
	g_free (default_runlevel);
	g_free (buf);
}