/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Tambet Ingo <tambet@ximian.com>
 *          Miguel de Icaza <miguel@ximian.com> (xst_ui_image_widget_create)
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "xst-ui.h"

/* For xst_ui_image_widget_create */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gal/e-table/e-table-specification.h>

#include "checked.xpm"
#include "unchecked.xpm"


GdkPixmap *checked_pixmap = NULL, *unchecked_pixmap = NULL;
GdkBitmap *checked_mask = NULL, *unchecked_mask = NULL;

static gchar *
container_get_label_string (GtkWidget *container)
{
	GList *children;
	gchar *buf = NULL;
	GtkWidget *child;
	
	children = gtk_container_get_children (GTK_CONTAINER (container));
	if (!children)
		return NULL;

	child = children->data;

	if (GTK_IS_LABEL (child))
		gtk_label_get (GTK_LABEL (child), &buf);
	
	return buf;
}


static GtkWidget *
get_list_item_by_name (GList *list, const gchar *label)
{
	GList *items;
	GtkWidget *listitem;
	gchar *buf;
	
	g_return_val_if_fail (list != NULL, NULL);

	for (items = list; items; items = items->next)
	{
		listitem = items->data;
		buf = container_get_label_string (listitem);
		if (buf && !strcmp (buf, label))
		    return listitem;
	}

	return NULL;
}

static GtkWidget *
xst_ui_image_widget_create_canvas (gchar *filename)
{
	GtkWidget *canvas;
	GdkPixbuf *pixbuf;
	double width, height;
	gchar *filename_dup;

	filename_dup = g_strdup (filename);
	pixbuf = gdk_pixbuf_new_from_file (filename_dup, NULL);

	if (!pixbuf) {
		g_warning ("Pixmap %s not found.", filename_dup);
		g_free (filename_dup);
		return NULL;
	}
		
	width  = gdk_pixbuf_get_width  (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	canvas = gnome_canvas_new_aa ();
	GTK_OBJECT_UNSET_FLAGS (GTK_WIDGET (canvas), GTK_CAN_FOCUS);
	gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (canvas)),
			       gnome_canvas_pixbuf_get_type (),
			       "pixbuf", pixbuf,
			       NULL);
	gtk_widget_set_usize (canvas, width, height);

	gdk_pixbuf_unref (pixbuf);
	gtk_widget_show (canvas);
	g_free (filename_dup);

	return canvas;
}
	
/* Stolen and adapted from evolution's e-util/e-gui-utils.c
 * Arturo Espinosa <arturo@ximian.com> */
GtkWidget *
xst_ui_image_widget_create (gchar *name,
			    gchar *string1, gchar *string2,
			    gint int1, gint int2)
{
	GtkWidget *canvas, *alignment;

	/* See the message in xst_tool_init on why we call this funciton with NULL */
	if (!string1)
		return NULL;

	canvas = xst_ui_image_widget_create_canvas (string1);
	g_return_val_if_fail (canvas != NULL, NULL);
	
	alignment = gtk_widget_new (gtk_alignment_get_type(),
				    "child", canvas,
				    "xalign", (double) 0,
				    "yalign", (double) 0,
				    "xscale", (double) 0,
				    "yscale", (double) 0,
				    NULL);
	
	gtk_widget_show (alignment);

	return alignment;
}

void
xst_ui_image_set_pix (GtkWidget *widget, gchar *filename)
{
	GtkWidget *canvas;
	GList *child;

	canvas = xst_ui_image_widget_create_canvas (filename);
	g_return_if_fail (canvas != NULL);
		
	child = gtk_container_get_children (GTK_CONTAINER (widget));
	gtk_container_remove (GTK_CONTAINER (widget), child->data);
	gtk_container_add (GTK_CONTAINER (widget), canvas);
}

GtkWidget *
xst_ui_image_widget_get (GladeXML *gui, gchar *name)
{
	GList *children;
	GtkWidget *container;

	container = glade_xml_get_widget   (gui, "report_pixmap");
	children  = gtk_container_get_children (GTK_CONTAINER (container));
	g_return_val_if_fail (children != NULL, NULL);
	return children->data;
}

/**
 * xst_ui_option_menu_get_selected_row: Returns index of selected label in GtkOptionMenu.
 * @option_menu: GtkOptionMenu to examine.
 *
 * This is a hack that makes it easier (possible) to define GtkOptionMenu choices in Glade
 * without adding special-case code to add signals to every GtkMenuItem that invoke
 * handlers based on the item selected. It is a bit rough, we could probably do without
 * comparing labels and just go directly for widget pointers, which would allow us to deal
 * with any kind of widget. Glade just does labels, though.
 *
 * Since the selected widget is taken out of the menu's list and placed in the OptionMenu
 * container, we temporarily select row 0, and when we find out which row is selected, we
 * re-select it. If anyone knows how to do this better, tell me how.
 *
 * Return value: List index in the range [0 .. n - 1], or -1 if the selected item is not a
 * label.
 **/
gint
xst_ui_option_menu_get_selected_row (GtkOptionMenu *option_menu)
{
	GtkWidget *selected, *found;
	GList *menu_items;
	gchar *label;
	gint row;

	g_return_val_if_fail (option_menu != NULL, -1);
	g_return_val_if_fail (GTK_IS_OPTION_MENU (option_menu), -1);
	
	selected = GTK_WIDGET (GTK_BIN (option_menu)->child);
	if (!GTK_IS_LABEL (selected))
		return -1;

	gtk_label_get (GTK_LABEL (selected), &label);
	gtk_option_menu_set_history (option_menu, 0);
	menu_items = GTK_MENU_SHELL (gtk_option_menu_get_menu (option_menu))->children;

	found = get_list_item_by_name (menu_items, label);
	if (!found)
		return 0;

	row = g_list_index (menu_items, found);
	gtk_option_menu_set_history (option_menu, row);
	return row;
}

/**
 * xst_ui_option_menu_get_selected_string: Returns string of selected label in GtkOptionMenu.
 * @option_menu: GtkOptionMenu to examine.
 * 
 * It only works for GtkMenues conataining GtkLabels.
 * 
 * Return Value: Pointer to gchar containing the label of the active (selected) item in
 * GtkOptionMenu. NULL if active item isn't GtkLabel. Must be g_free()'d.
 **/
gchar *
xst_ui_option_menu_get_selected_string (GtkOptionMenu *option_menu)
{
	gchar *buf;

	g_return_val_if_fail (option_menu != NULL, NULL);
	g_return_val_if_fail (GTK_IS_OPTION_MENU (option_menu), NULL);
	
	if (GTK_BIN (option_menu)->child) {
		GtkWidget *child = GTK_BIN (option_menu)->child;
		
		if (GTK_IS_LABEL (child)) {
			gtk_label_get (GTK_LABEL (child), &buf);
			return g_strdup (buf);
		}		
	}

	return NULL;
}

void
xst_ui_option_menu_set_selected_string (GtkOptionMenu *option_menu, const gchar *string)
{
	GtkWidget *menu, *found;
	GList *menu_items;
	gint row;
	gchar *active_label;

	g_return_if_fail (option_menu != NULL);
	g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
	g_return_if_fail (string != NULL);
	
	menu = gtk_option_menu_get_menu (option_menu);

	/* Check if the active menu item is the one we want to set.
	   If so, do nothing. */
	active_label = container_get_label_string (GTK_WIDGET (option_menu));
	if (active_label && !strcmp (active_label, string))
		return;
	
	menu_items = GTK_MENU_SHELL (menu)->children;
	if (!menu_items)
		return;

	found = get_list_item_by_name (menu_items, string);
	if (!found) {
		g_warning ("xst_ui_option_menu_set_selected_string: "
			   "'%s' not in menu.", string);
		return;
	}

	row = g_list_index (menu_items, found);
	gtk_option_menu_set_history (option_menu, row);
}

/**
 * xst_ui_option_menu_add_string: Add new GtkLabel child to GtkOptionMenu.
 * @option_menu:  GtkOptionMenu to examine. 
 * @string: Pointer to gchar containing the label.
 * 
 * Simple wrapper to make life easier when dealing with GtkOptionMenus containing GtkLabels.
 * 
 * Return Value: the newly created GtkMenuItem.
 **/
GtkWidget *
xst_ui_option_menu_add_string (GtkOptionMenu *option_menu, const gchar *string)
{
	GtkWidget *menu, *item;
	gboolean new_menu = FALSE;

	g_return_val_if_fail (option_menu != NULL, NULL);
	g_return_val_if_fail (GTK_IS_OPTION_MENU (option_menu), NULL);

	menu = gtk_option_menu_get_menu (option_menu);

	if (!GTK_MENU_SHELL (menu)->children) {
		gtk_option_menu_remove_menu (option_menu);
		menu = gtk_menu_new ();
		new_menu = TRUE;
	}
	
	item = gtk_menu_item_new_with_label (string);
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	if (new_menu)
		gtk_option_menu_set_menu (option_menu, menu);
	
	return item;
}

/**
 * xst_ui_option_menu_remove_string: Remove GtkLabel child from GtkOptionMenu.
 * @option_menu: GtkOptionMenu to examine.
 * @string: Pointer to gchar containing the label.
 * 
 * Simple wrapper to make life easier when dealing with GtkOptionMenus containing GtkLabels.
 **/
void
xst_ui_option_menu_remove_string (GtkOptionMenu *option_menu, const gchar *string)
{
	GtkWidget *found = NULL;
	gchar *buf = xst_ui_option_menu_get_selected_string (option_menu);
	GtkWidget *menu = gtk_option_menu_get_menu (option_menu);

	g_return_if_fail (option_menu != NULL);
	g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
	g_return_if_fail (string != NULL);
	
	if (!strcmp (buf, string))
		found = gtk_menu_get_active (GTK_MENU (menu));
	else {
		GList *menu_items = GTK_MENU_SHELL (menu)->children;
		found = get_list_item_by_name (menu_items, string);
		g_free (menu_items);
	}
	
	if (found)
		gtk_widget_destroy (found);
}

static void
menu_clear (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (widget);
}

void
xst_ui_option_menu_clear (GtkOptionMenu *option_menu)
{
	GtkWidget *menu;

	g_return_if_fail (option_menu != NULL);
	g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
	
	menu = gtk_option_menu_get_menu (option_menu);
	gtk_container_foreach (GTK_CONTAINER (menu), menu_clear, NULL);
}

/**
 * xst_ui_load_etspec:
 * @common_path: Usually tool->etspecs_common_path
 * @name: name of spec file.
 * 
 * Load ETableSpecification from given filename, return it as a string.
 * Checks if given spec file is valid.
 * 
 * Return Value: String containing ETableSpecification or NULL one error.
 **/
gchar *
xst_ui_load_etspec (const gchar *common_path, const gchar *name)
{
	ETableSpecification *specification;
	gchar *table_spec = NULL;
	gchar *path;

	g_return_val_if_fail (common_path != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	if (common_path[strlen (common_path) - 1] != '/')
		path = g_strconcat (common_path, "/", name, NULL);
	else
		path = g_strconcat (common_path, name, NULL);
	
	specification = e_table_specification_new();
	if (e_table_specification_load_from_file(specification, path))
		table_spec = e_table_specification_save_to_string (specification);
	else
		g_warning ("xst_ui_load_etspec: Cant' load ETable specification from %s.", path);
	
	gtk_object_unref(GTK_OBJECT(specification));
	g_free (path);
	
	return table_spec;
}

/**
 * xst_ui_entry_set_text:
 * @entry: GtkEntry to set to
 * @str: string to set
 *
 * Simple wrapper around gtk_entry_set_text. Sets entry text to "" if str == NULL
 * 
 **/
void
xst_ui_entry_set_text (void *entry, const gchar *str)
{
	g_return_if_fail (entry != NULL);
	g_return_if_fail (GTK_IS_ENTRY (entry));
	
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}

/**
 * xst_ui_logout_dialog:
 * @void: 
 * 
 * Asks user for confirmation and restarts X server.
 * 
 * Return Value: TRUE if logout, FALSE if not.
 **/
gboolean
xst_ui_logout_dialog (const gchar *message)
{
	GtkWidget *d;
	gchar *tmp;

	tmp = message? _(message) :
		_("Plese restart your GNOME session for\nthese changes to take effect.");

	d = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
				    GTK_BUTTONS_OK, tmp);

	gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);

	return FALSE;
	
	/* Cause it isn't working, let's just suggest user to restart X. */	
#if 0	
	GtkWidget *dialog;
	gint retval;
	GnomeClient *client = gnome_master_client ();
	
	dialog = gnome_question_dialog (message, NULL, NULL);

	retval = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));

	if (retval)
		return FALSE;

	gnome_client_request_save (client, GNOME_SAVE_BOTH,
				   TRUE, GNOME_INTERACT_ERRORS, FALSE, TRUE);

	return TRUE;
#endif		
}

/**
 * xst_ui_text_view_clear:
 * @view: GtkTextView
 * 
 * Simple wrapper to clear buffer of GtkTextView.
 **/
void
xst_ui_text_view_clear (GtkTextView *view)
{
	GtkTextBuffer *buffer;

	g_return_if_fail (view != NULL);
	g_return_if_fail (GTK_IS_TEXT_VIEW (view));

	buffer = gtk_text_view_get_buffer (view);
	gtk_text_buffer_set_text (buffer, "", 0);
}

/**
 * xst_ui_text_view_get_text:
 * @view: GtkTreeView
 * 
 * Get all text from GtkTextView. Note that using this function
 * makes GtkTextView widget really simple and lame.
 * 
 * Return Value: Text, which should be freed when done using it.
 **/
gchar *
xst_ui_text_view_get_text (GtkTextView *view)
{
	GtkTextBuffer *buffer;
	GtkTextIter   start_iter;
	GtkTextIter   end_iter;

	g_return_val_if_fail (view != NULL, NULL);
	g_return_val_if_fail (GTK_IS_TEXT_VIEW (view), NULL);

	buffer = gtk_text_view_get_buffer (view);
	gtk_text_buffer_get_start_iter (buffer, &start_iter);
	gtk_text_buffer_get_end_iter (buffer, &end_iter);

	return gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
}

/**
 * xst_ui_text_view_add_text:
 * @view: GtkTreeView
 * @text: 
 * 
 * Insert text to the end of GtkTextView's buffer. Note that using this function
 * makes GtkTextView widget really simple and lame.
 *
 **/
void
xst_ui_text_view_add_text (GtkTextView *view, const gchar *text)
{
	GtkTextBuffer *buffer;
	GtkTextIter   end_iter;

	g_return_if_fail (view != NULL);
	g_return_if_fail (GTK_IS_TEXT_VIEW (view));

	if (text == NULL && strlen (text) == 0)
		return;

	buffer = gtk_text_view_get_buffer (view);
	gtk_text_buffer_get_end_iter (buffer, &end_iter);

	gtk_text_buffer_insert (buffer, &end_iter, text, strlen (text));
}
