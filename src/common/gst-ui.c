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
 *          Miguel de Icaza <miguel@ximian.com> (gst_ui_image_widget_create)
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "gst-ui.h"

/* For gst_ui_image_widget_create */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

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
gst_ui_image_widget_create_canvas (gchar *filename)
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
gst_ui_image_widget_create (gchar *name,
			    gchar *string1, gchar *string2,
			    gint int1, gint int2)
{
	GtkWidget *canvas, *alignment;

	/* See the message in gst_tool_init on why we call this funciton with NULL */
	if (!string1)
		return NULL;

	canvas = gst_ui_image_widget_create_canvas (string1);
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
gst_ui_image_set_pix (GtkWidget *widget, gchar *filename)
{
	GtkWidget *canvas;
	GList *child;

	canvas = gst_ui_image_widget_create_canvas (filename);
	g_return_if_fail (canvas != NULL);
		
	child = gtk_container_get_children (GTK_CONTAINER (widget));
	gtk_container_remove (GTK_CONTAINER (widget), child->data);
	gtk_container_add (GTK_CONTAINER (widget), canvas);
}

GtkWidget *
gst_ui_image_widget_get (GladeXML *gui, gchar *name)
{
	GList *children;
	GtkWidget *container;

	container = glade_xml_get_widget   (gui, "report_pixmap");
	children  = gtk_container_get_children (GTK_CONTAINER (container));
	g_return_val_if_fail (children != NULL, NULL);
	return children->data;
}

/**
 * gst_ui_entry_set_text:
 * @entry: GtkEntry to set to
 * @str: string to set
 *
 * Simple wrapper around gtk_entry_set_text. Sets entry text to "" if str == NULL
 * 
 **/
void
gst_ui_entry_set_text (void *entry, const gchar *str)
{
	g_return_if_fail (entry != NULL);
	g_return_if_fail (GTK_IS_ENTRY (entry));
	
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}

/**
 * gst_ui_text_view_clear:
 * @view: GtkTextView
 * 
 * Simple wrapper to clear buffer of GtkTextView.
 **/
void
gst_ui_text_view_clear (GtkTextView *view)
{
	GtkTextBuffer *buffer;

	g_return_if_fail (view != NULL);
	g_return_if_fail (GTK_IS_TEXT_VIEW (view));

	buffer = gtk_text_view_get_buffer (view);
	gtk_text_buffer_set_text (buffer, "", 0);
}

/**
 * gst_ui_text_view_get_text:
 * @view: GtkTreeView
 * 
 * Get all text from GtkTextView. Note that using this function
 * makes GtkTextView widget really simple and lame.
 * 
 * Return Value: Text, which should be freed when done using it.
 **/
gchar *
gst_ui_text_view_get_text (GtkTextView *view)
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
 * gst_ui_text_view_add_text:
 * @view: GtkTreeView
 * @text: 
 * 
 * Insert text to the end of GtkTextView's buffer. Note that using this function
 * makes GtkTextView widget really simple and lame.
 *
 **/
void
gst_ui_text_view_add_text (GtkTextView *view, const gchar *text)
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
