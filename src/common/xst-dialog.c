/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
#include <gmodule.h>
#include "xst-widget.h"
#include "xst-dialog.h"
#include "xst-conf.h"

#ifdef XST_DEBUG
/* define to x for debugging output */
#define d(x) 
#else
#define d(x)
#endif

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
		g_error (_("Could not find widget: %s"), widget);

	return w;
}

static void
apply_widget_policies (XstDialog *xd)
{
	GSList *list;

	/* Hide, show + desensitize or show + sensitize widgets based on access level
	 * and complexity */

	for (list = xd->xst_widget_list; list; list = g_slist_next (list))
	{
		xst_widget_apply_policy (list->data);
	}
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
	char *label[] = {
		N_(" More Options >> "),
		N_(" << Fewer Options "),
		NULL
	};

	g_return_if_fail (xd != NULL);
	g_return_if_fail (XST_IS_DIALOG (xd));
	g_return_if_fail (c >= XST_DIALOG_BASIC);
	g_return_if_fail (c <= XST_DIALOG_ADVANCED);

	if (xd->complexity == c)
		return;

	xd->complexity = c;

	xst_conf_set_integer (xd->tool, "complexity", xd->complexity);

	apply_widget_policies (xd);
	gtk_label_set_text (GTK_LABEL (GTK_BIN (xd->complexity_button)->child), _(label[c]));
	gtk_signal_emit (GTK_OBJECT (xd), xstdialog_signals[COMPLEXITY_CHANGE]);
}

void
xst_dialog_freeze (XstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (XST_IS_DIALOG (xd));
	g_return_if_fail (xd->frozen >= 0);

	d(g_message ("freezing %p", xd));

	if (!xd->frozen)
		gtk_widget_set_sensitive (GTK_WIDGET (xd), FALSE);
	
	xd->frozen++;
}

void 
xst_dialog_thaw (XstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (XST_IS_DIALOG (xd));
	g_return_if_fail (xd->frozen >= 0);

	d(g_message ("thawing %p", xd));

	xd->frozen--;

	if (!xd->frozen)
		gtk_widget_set_sensitive (GTK_WIDGET (xd), TRUE);
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

	d(g_print ("froze: %d\taccess: %d\n", xd->frozen, xst_tool_get_access (xd->tool)));

	if (xd->frozen || !xst_tool_get_access (xd->tool))
		return;

	gtk_widget_set_sensitive (xd->apply_button, TRUE);
}

void
xst_dialog_modify_cb (GtkWidget *w, gpointer data)
{
	xst_dialog_modify (data);
}

static void
xst_dialog_destroy (GtkObject *tool)
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

void
xst_dialog_add_apply_hook (XstDialog *xd, XstDialogHookFunc func, gpointer data)
{
	XstDialogHookEntry *entry;

	entry = g_new0 (XstDialogHookEntry, 1);
	entry->data = data;
	entry->func = func;

	xd->apply_hook_list = g_list_append (xd->apply_hook_list, entry);
}

void
xst_dialog_set_widget_policies (XstDialog *xd, const XstWidgetPolicy *xwp)
{
	XstWidget *xw;
	int i;

	for (i = 0; xwp [i].widget; i++)
	{
		xw = xst_widget_new (xst_dialog_get_widget (xd, xwp [i].widget), xd,
				     xwp [i].basic, xwp [i].advanced, xwp [i].need_access,
				     xwp [i].user_sensitive);

		xd->xst_widget_list = g_slist_append (xd->xst_widget_list, xw);
	}

	apply_widget_policies (xd);
}

void
xst_dialog_set_widget_user_modes (XstDialog *xd, const XstWidgetUserPolicy *xwup)
{
	XstWidget *xw;
	int i;

	for (i = 0; xwup [i].widget; i++)
	{
		xw = xst_dialog_get_xst_widget (xd, xwup [i].widget);

		if (!xw)
		{
			xw = xst_widget_new (xst_dialog_get_widget (xd, xwup [i].widget), xd,
					     XST_WIDGET_MODE_SENSITIVE, XST_WIDGET_MODE_SENSITIVE,
					     FALSE, TRUE);

			xd->xst_widget_list = g_slist_append (xd->xst_widget_list, xw);
		}

		xst_widget_set_user_mode (xw, xwup [i].mode);
	}

	apply_widget_policies (xd);
}

XstWidget *
xst_dialog_get_xst_widget (XstDialog *xd, const gchar *name)
{
	XstWidget *xw = NULL;
	GtkWidget *widget;
	GSList *list;

	g_return_val_if_fail (xd != NULL, NULL);

	widget = xst_dialog_get_widget (xd, name);

	for (list = xd->xst_widget_list; list; list = g_slist_next (list))
	{
		if (((XstWidget *) list->data)->widget == widget)
		{
			xw = list->data;
			break;
		}
	}

	if (xw == NULL)
		g_warning ("Widget %s not found in policy table.", name);
	return (xw);
}

void
xst_dialog_widget_set_user_mode (XstDialog *xd, const gchar *name, XstWidgetMode mode)
{
	XstWidget *xw;

	g_return_if_fail (xd != NULL);

	xw = xst_dialog_get_xst_widget (xd, name);
	g_return_if_fail (xw != NULL);
	
	xst_widget_set_user_mode (xw, mode);
}

void
xst_dialog_widget_set_user_sensitive (XstDialog *xd, const gchar *name, gboolean state)
{
	XstWidget *xw;

	g_return_if_fail (xd != NULL);
	
	xw = xst_dialog_get_xst_widget (xd, name);
	g_return_if_fail (xw != NULL);

	xst_widget_set_user_sensitive (xw, state);
}

static void
complexity_cb (GtkWidget *w, gpointer data)
{
	XstDialog *dialog;

	g_return_if_fail (data != NULL);
	g_return_if_fail (XST_IS_DIALOG (data));

	dialog = XST_DIALOG (data);

	switch (dialog->complexity) {
	case XST_DIALOG_BASIC:
		xst_dialog_set_complexity (dialog, XST_DIALOG_ADVANCED);
		break;
	case XST_DIALOG_ADVANCED:
		xst_dialog_set_complexity (dialog, XST_DIALOG_BASIC);
		break;
	default:
		break;
	}
}

static void
apply_cb (GtkWidget *w, gpointer data)
{
	XstDialog *dialog;
	XstDialogHookEntry *hookentry;
	GList *l;

	g_return_if_fail (data != NULL);
	g_return_if_fail (XST_IS_DIALOG (data));

	dialog = XST_DIALOG (data);

	for (l = dialog->apply_hook_list; l; l = l->next) {
		hookentry = l->data;
		if (! (hookentry->func) (dialog, hookentry->data))
			return;
	}
	
	gtk_signal_emit (GTK_OBJECT (dialog), xstdialog_signals[APPLY]);

	gtk_widget_set_sensitive (dialog->apply_button, FALSE);
}

static void
dialog_close (XstDialog *dialog)
{
	g_return_if_fail (dialog != NULL);
	g_return_if_fail (XST_IS_DIALOG (dialog));

	if (xst_dialog_get_modified (dialog)) {
		/* Changes have been made. */
		GtkWidget *w;
		
		w = gnome_question_dialog_parented (
			_("There are changes which haven't been applied.\n"
			  "Apply them now?"),
			NULL, NULL, GTK_WINDOW (dialog));

		if (!gnome_dialog_run_and_close (GNOME_DIALOG (w)))
			apply_cb (NULL, dialog);
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	if (dialog == dialog->tool->main_dialog)
		gtk_signal_emit_by_name (GTK_OBJECT (dialog->tool), "destroy");
}

static void
dialog_delete_event_cb (GtkWidget *w, GdkEvent *event, gpointer data)
{
	dialog_close (data);
}

static void
close_cb (GtkWidget *w, gpointer data)
{
	dialog_close (data);
}

static void
help_cb (GtkWidget *w, gpointer data)
{
	GnomeHelpMenuEntry help_entry = { NULL, "index.html" };
	XstDialog *dialog;

	g_return_if_fail (data != NULL);
	g_return_if_fail (XST_IS_DIALOG (data));

	dialog = XST_DIALOG (data);

	help_entry.name = g_strdup_printf ("%s-admin", dialog->tool->name);

	gnome_help_display (NULL, &help_entry);

	g_free (help_entry.name);
}

void
xst_dialog_enable_complexity (XstDialog *dialog)
{
	g_return_if_fail (dialog != NULL);
	g_return_if_fail (XST_IS_DIALOG (dialog));

	gtk_widget_set_sensitive (dialog->complexity_button, TRUE);
}

void
xst_dialog_connect_signals (XstDialog *dialog, XstDialogSignal *signals)
{       
	GtkWidget *w;
	guint sig;
	int i;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (XST_IS_DIALOG (dialog));

	for (i=0; signals[i].widget; i++) {
		w = xst_dialog_get_widget (dialog, signals[i].widget);
		sig = gtk_signal_connect (GTK_OBJECT (w),
					  signals[i].signal_name,
					  signals[i].func,
					  (gpointer)dialog);
		if (!sig)
			g_error (_("Error connection signal `%s' in widget `%s'"),
				 signals[i].signal_name, signals[i].widget);
	}
}

void
xst_dialog_construct (XstDialog *dialog, XstTool *tool,
		      const char *widget, const char *title)
{
	GladeXML *xml;
	GtkWidget *w, *i;
	gint val;
	char *s;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (XST_IS_DIALOG (dialog));
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (title != NULL);

	dialog->tool = tool;
	dialog->apply_hook_list = NULL;

	s = g_strdup_printf ("%s-admin", tool->name);
	gnome_app_construct (GNOME_APP (dialog), s, title);
	g_free (s);

	xml = xst_tool_load_glade_common (tool, "tool_vbox");
	
	w = glade_xml_get_widget (xml, "tool_vbox");
	gnome_app_set_contents (GNOME_APP (dialog), w);

	dialog->gui   = xst_tool_load_glade (tool, NULL);
	dialog->child = xst_dialog_get_widget (dialog, widget);

	gtk_widget_ref (dialog->child);
	gtk_widget_unparent (dialog->child);
	gtk_box_pack_start (GTK_BOX (w), dialog->child, TRUE, TRUE, 0);

	w = glade_xml_get_widget (xml, "help");
	i = gnome_stock_pixmap_widget (w, GNOME_STOCK_PIXMAP_HELP);
	gtk_widget_show (i);
	gtk_container_add (GTK_CONTAINER (w), i);

	
	w = glade_xml_get_widget (xml, "help");
	gtk_signal_connect (GTK_OBJECT (w), "clicked", help_cb, dialog);

	w = glade_xml_get_widget (xml, "complexity");
	gtk_signal_connect (GTK_OBJECT (w), "clicked", complexity_cb, dialog);

	dialog->complexity_button = w;

	w = glade_xml_get_widget (xml, "apply");
	gtk_signal_connect (GTK_OBJECT (w), "clicked", apply_cb, dialog);

	dialog->apply_button = w;

	w = glade_xml_get_widget (xml, "close");
	gtk_signal_connect (GTK_OBJECT (w), "clicked", close_cb, dialog);
	
	gtk_widget_set_sensitive (dialog->apply_button,      FALSE);
	gtk_widget_set_sensitive (dialog->complexity_button, FALSE);

	dialog->complexity = XST_DIALOG_NONE;
	val = xst_conf_get_integer (dialog->tool, "complexity");
	if (val < 0)
		val = XST_DIALOG_BASIC;

	xst_dialog_set_complexity (dialog, val);

	gtk_signal_connect (GTK_OBJECT (dialog), "delete_event", dialog_delete_event_cb, dialog);
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
