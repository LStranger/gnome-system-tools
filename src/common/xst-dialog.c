/* 
 * Copyright (C) 2001 Ximian, Inc.
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
 * Authors: Jacob Berkman <jacob@ximian.com>
 */

#include <config.h>

#include <gnome.h>
#include "xst-dialog.h"

enum {
	APPLY,
	COMPLEXITY_CHANGE,
	LAST_SIGNAL
};


static GnomeAppClass *parent_class;
static gint xstdialog_signals[LAST_SIGNAL] = { 0 };

GtkWidget *
xst_dialog_get_widget (XstDialog *xd, const char *widget)
{
	GtkWidget *w;

	g_return_val_if_fail (xd != NULL, NULL);
	g_return_val_if_fail (XST_IS_DIALOG (xd), NULL);
	g_return_val_if_fail (xd->gui != NULL, NULL);

	w = glade_xml_get_widget (xd->gui, widget);

	if (!w)
		g_warning (_("Could not find widget: %s"), widget);

	return w;
}

XstDialogComplexity 
xst_dialog_get_complexity (XstDialog *xd)
{
	g_return_val_if_fail (xd != NULL, 0);
	g_return_val_if_fail (XST_IS_DIALOG (xd), 0);

	return xd->complexity;
}

void
xst_dialog_set_complexity (XstDialog *xd, XstDialogComplexity c)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (XST_IS_DIALOG (xd));
	g_return_if_fail (c < XST_TOOL_COMPLEXITY_BASIC);
	g_return_if_fail (c > XST_TOOL_COMPLEXITY_ADVANCED);

	if (xd->complexity == c)
		return;

	xd->complexity = c;
	gtk_signal_emit (GTK_OBJECT (xd), xstdialog_signals[COMPLEXITY_CHANGE]);
}

void
xst_dialog_freeze (XstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (XST_IS_DIALOG (xd));

	xd->frozen = TRUE;
}

void 
xst_dialog_thaw (XstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (XST_IS_DIALOG (xd));

	xd->frozen = FALSE;
}

gboolean
xst_dialog_get_modified (XstDialog *xd)
{
	g_return_val_if_fail (xd != NULL, FALSE);
	g_return_val_if_fail (XST_IS_DIALOG (xd), FALSE);

	return GTK_WIDGET_SENSITIVE (xd->apply_button);
}

void
xst_dialog_modify (XstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (XST_IS_DIALOG (xd));

	if (xd->frozen || !xst_tool_get_access (xd->tool))
		return;

	gtk_widget_set_sensitive (xd->apply_button, TRUE);
}

static void
xst_dialog_destroy (XstTool *tool)
{
	GTK_OBJECT_CLASS (parent_class)->destroy (GTK_OBJECT (tool));
}

static void
xst_dialog_class_init (XstDialogClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	parent_class = gtk_type_class (GNOME_TYPE_APP);

	xstdialog_signals[APPLY] = 
		gtk_signal_new ("apply",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (XstDialogClass, apply),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	xstdialog_signals[COMPLEXITY_CHANGE] =
		gtk_signal_new ("complexity_change",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (XstDialogClass, complexity_change),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);


	gtk_object_class_add_signals (object_class, xstdialog_signals, LAST_SIGNAL);

	object_class->destroy = xst_dialog_destroy;
}

static void
xst_dialog_init (XstDialog *dialog)
{
	/* nothing to do here
	 * exciting stuff happens in _construct
	 */
}

GtkType
xst_dialog_get_type (void)
{
	static GtkType xstdialog_type = 0;

	if (!xstdialog_type) {
		GtkTypeInfo xstdialog_info = {
			"XstDialog",
			sizeof (XstDialog),
			sizeof (XstDialogClass),
			(GtkClassInitFunc) xst_dialog_class_init,
			(GtkObjectInitFunc) xst_dialog_init,
			NULL, NULL, NULL
		};

		xstdialog_type = gtk_type_unique (GNOME_TYPE_APP, &xstdialog_info);
	}

	return xstdialog_type;
}

static void
tool_user_complexity (GtkWidget *w, gpointer data)
{

}

static void
tool_user_apply (GtkWidget *w, gpointer data)
{

}

static void
tool_user_close (GtkWidget *w, gpointer data)
{

}

static void
tool_user_help (GtkWidget *w, gpointer data)
{

}

void
xst_dialog_construct (XstDialog *dialog, XstTool *tool,
		      const char *widget, const char *title)
{
	GladeXML *xml;
	GtkWidget *w, *i;
	char *s;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (XST_IS_DIALOG (dialog));
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (title != NULL);

	dialog->tool = tool;

       	s = g_strdup_printf ("%s-admin", tool->name);
	gnome_app_construct (GNOME_APP (dialog), s, title);
	g_free (s);

	xml = xst_tool_load_glade_common (tool, "tool_vbox");
	w = glade_xml_get_widget (xml, "tool_vbox");
	gnome_app_set_contents (GNOME_APP (dialog), w);

	dialog->gui = xst_tool_load_glade (tool, widget);
	dialog->child = xst_dialog_get_widget (dialog, widget);

	gtk_box_pack_start (GTK_BOX (w), dialog->child, TRUE, TRUE, 0);

	w = glade_xml_get_widget (xml, "help");
	i = gnome_stock_pixmap_widget (w, GNOME_STOCK_PIXMAP_HELP);
	gtk_widget_show (i);
	gtk_container_add (GTK_CONTAINER (w), i);

	glade_xml_signal_connect_data (xml, "tool_user_help",       tool_user_help,       dialog);
	glade_xml_signal_connect_data (xml, "tool_user_complexity", tool_user_complexity, dialog);
	glade_xml_signal_connect_data (xml, "tool_user_apply",      tool_user_apply,      dialog);
	glade_xml_signal_connect_data (xml, "tool_user_close",      tool_user_close,      dialog);	
	
	dialog->apply_button      = glade_xml_get_widget (xml, "apply");
	dialog->complexity_button = glade_xml_get_widget (xml, "complexity");
}

XstDialog *
xst_dialog_new (XstTool *tool, const char *widget, const char *title)
{
	XstDialog *dialog;

	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (title != NULL, NULL);

	dialog = XST_DIALOG (gtk_type_new (XST_TYPE_DIALOG));
	xst_dialog_construct (dialog, tool, widget, title);

	return dialog;
}
