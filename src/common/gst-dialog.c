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
#include "gst-widget.h"
#include "gst-dialog.h"
#include "gst-conf.h"
#include "gst-marshal.h"

#ifdef GST_DEBUG
/* define to x for debugging output */
#define d(x) x
#else
#define d(x)
#endif

enum {
	BOGUS,
	APPLY,
	LAST_SIGNAL
};

static GnomeAppClass *parent_class;
static gint gstdialog_signals[LAST_SIGNAL] = { 0 };

GtkWidget *
gst_dialog_get_widget (GstDialog *xd, const char *widget)
{
	GtkWidget *w;

	g_return_val_if_fail (xd != NULL, NULL);
	g_return_val_if_fail (GST_IS_DIALOG (xd), NULL);
	g_return_val_if_fail (xd->gui != NULL, NULL);

	w = glade_xml_get_widget (xd->gui, widget);

	if (!w)
		g_error (_("Could not find widget: %s"), widget);

	return w;
}

void
gst_dialog_apply_widget_policies (GstDialog *xd)
{
	GSList *list;

	/* Hide, show + desensitize or show + sensitize widgets based on access level */
	for (list = xd->gst_widget_list; list; list = g_slist_next (list))
	{
		gst_widget_apply_policy (list->data);
	}
}

GstDialogComplexity 
gst_dialog_get_complexity (GstDialog *xd)
{
	g_return_val_if_fail (xd != NULL, 0);
	g_return_val_if_fail (GST_IS_DIALOG (xd), 0);

	return GST_DIALOG_ADVANCED;
}

void
gst_dialog_freeze (GstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (GST_IS_DIALOG (xd));

	d(g_message ("freezing %p", xd));

	xd->frozen = TRUE;
}

void 
gst_dialog_thaw (GstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (GST_IS_DIALOG (xd));

	d(g_message ("thawing %p", xd));

	xd->frozen = FALSE;
}

void
gst_dialog_freeze_visible (GstDialog *xd)
{
	GdkCursor *cursor;
	
	g_return_if_fail (xd != NULL);
	g_return_if_fail (GST_IS_DIALOG (xd));
	g_return_if_fail (xd->frozen >= 0);

	if (GTK_WIDGET (xd)->window) {
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (GTK_WIDGET (xd)->window, cursor);
		gdk_cursor_unref (cursor);
	}

	if (!xd->frozen)
		gtk_widget_set_sensitive (GTK_WIDGET (xd), FALSE);
	
	gst_dialog_freeze (xd);
}

void
gst_dialog_thaw_visible (GstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (GST_IS_DIALOG (xd));
	g_return_if_fail (xd->frozen >= 0);

	if (GTK_WIDGET (xd)->window)
		gdk_window_set_cursor (GTK_WIDGET (xd)->window, NULL);

	gst_dialog_thaw (xd);

	if (!xd->frozen)
		gtk_widget_set_sensitive (GTK_WIDGET (xd), TRUE);
}

gboolean
gst_dialog_get_modified (GstDialog *xd)
{
	g_return_val_if_fail (xd != NULL, FALSE);
	g_return_val_if_fail (GST_IS_DIALOG (xd), FALSE);

	return xd->is_modified;
}

void
gst_dialog_set_modified (GstDialog *xd, gboolean state)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (GST_IS_DIALOG (xd));

	xd->is_modified = state;
}

void
gst_dialog_modify (GstDialog *xd)
{
	g_return_if_fail (xd != NULL);
	g_return_if_fail (GST_IS_DIALOG (xd));

	d(g_print ("froze: %d\taccess: %d\n", xd->frozen, gst_tool_get_access (xd->tool)));

	if (xd->frozen || !gst_tool_get_access (xd->tool))
		return;

	gst_dialog_set_modified (xd, TRUE);
}

void
gst_dialog_modify_cb (GtkWidget *w, gpointer data)
{
	gst_dialog_modify (data);
}

static void
gst_dialog_class_init (GstDialogClass *class)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (class);
	parent_class = g_type_class_peek_parent (class);

	gstdialog_signals[APPLY] = 
		g_signal_new ("apply",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstDialogClass, apply),
			      NULL, NULL,
			      gst_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static void
gst_dialog_init (GstDialog *dialog)
{
	/* nothing to do here
	 * exciting stuff happens in _construct
	 */
}
	
GtkType
gst_dialog_get_type (void)
{
	static GType gstdialog_type = 0;

	if (!gstdialog_type) {
		static const GTypeInfo gstdialog_info = {
			sizeof (GstDialogClass),
			NULL, /* base_init */
			NULL, /* base finalize */
			(GClassInitFunc) gst_dialog_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (GstDialog),
			0, /* n_preallocs */
			(GInstanceInitFunc) gst_dialog_init
		};

		gstdialog_type = g_type_register_static (GNOME_TYPE_APP, "GstDialog", &gstdialog_info, 0);
	}

	return gstdialog_type;
}

void
gst_dialog_add_apply_hook (GstDialog *xd, GstDialogHookFunc func, gpointer data)
{
	GstDialogHookEntry *entry;

	entry = g_new0 (GstDialogHookEntry, 1);
	entry->data = data;
	entry->func = func;

	xd->apply_hook_list = g_list_append (xd->apply_hook_list, entry);
}

gboolean
gst_dialog_run_apply_hooks (GstDialog *xd)
{
	GstDialogHookEntry *hookentry;
	GList *l;
	
	for (l = xd->apply_hook_list; l; l = l->next) {
		hookentry = l->data;
		if (! (hookentry->func) (xd, hookentry->data))
			return FALSE;
	}

	return TRUE;
}

void
gst_dialog_set_widget_policies (GstDialog *xd, const GstWidgetPolicy *xwp)
{
	GstWidget *xw;
	int i;

	for (i = 0; xwp [i].widget; i++) {
		xw = gst_widget_new (xd, xwp [i]);
	}
}

void
gst_dialog_set_widget_user_modes (GstDialog *xd, const GstWidgetUserPolicy *xwup)
{
	GstWidget *xw;
	int i;

	for (i = 0; xwup [i].widget; i++)
	{
		xw = gst_dialog_get_gst_widget (xd, xwup [i].widget);

		if (!xw)
			xw = gst_widget_new_full (gst_dialog_get_widget (xd, xwup [i].widget), xd,
						  GST_WIDGET_MODE_SENSITIVE, GST_WIDGET_MODE_SENSITIVE,
						  FALSE, TRUE);

		gst_widget_set_user_mode (xw, xwup [i].mode);
	}

	gst_dialog_apply_widget_policies (xd);
}

GstWidget *
gst_dialog_get_gst_widget (GstDialog *xd, const gchar *name)
{
	GstWidget *xw = NULL;
	GtkWidget *widget;
	GSList *list;

	g_return_val_if_fail (xd != NULL, NULL);

	widget = gst_dialog_get_widget (xd, name);

	for (list = xd->gst_widget_list; list; list = g_slist_next (list))
	{
		if (((GstWidget *) list->data)->widget == widget)
		{
			xw = list->data;
			break;
		}
	}

	if (xw == NULL)
		g_warning ("Widget %s not found in policy table.", name);
	return (xw);
}

GstTool *
gst_dialog_get_tool (GstDialog *xd)
{
	return xd->tool;
}

void
gst_dialog_widget_set_user_mode (GstDialog *xd, const gchar *name, GstWidgetMode mode)
{
	GstWidget *xw;

	g_return_if_fail (xd != NULL);

	xw = gst_dialog_get_gst_widget (xd, name);
	g_return_if_fail (xw != NULL);
	
	gst_widget_set_user_mode (xw, mode);
}

void
gst_dialog_widget_set_user_sensitive (GstDialog *xd, const gchar *name, gboolean state)
{
	GstWidget *xw;

	g_return_if_fail (xd != NULL);
	
	xw = gst_dialog_get_gst_widget (xd, name);
	g_return_if_fail (xw != NULL);

	gst_widget_set_user_sensitive (xw, state);
}

static void
apply_config (gpointer data)
{
	GstDialog *dialog;

	g_return_if_fail (data != NULL);
	g_return_if_fail (GST_IS_DIALOG (data));

	dialog = GST_DIALOG (data);

	if (!gst_dialog_run_apply_hooks (dialog))
		return;

	g_signal_emit (G_OBJECT (dialog), gstdialog_signals[APPLY], 0);

	gst_dialog_set_modified (dialog, FALSE);
}

void
gst_dialog_ask_apply (GstDialog *dialog)
{
        g_return_if_fail (dialog != NULL);
        g_return_if_fail (GST_IS_DIALOG (dialog));

        if (gst_dialog_get_modified (dialog)) {
                /* Changes have been made. */
                GtkWidget *w;
                gint       res;

		w = gtk_message_dialog_new (GTK_WINDOW (dialog),
					    GTK_DIALOG_MODAL,
					    GTK_MESSAGE_QUESTION,
					    GTK_BUTTONS_NONE,
					    _("There are changes which have not been applied"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (w),
							  _("Apply them now?"));
		gtk_dialog_add_buttons (GTK_DIALOG (w),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
					NULL);
                res = gtk_dialog_run (GTK_DIALOG (w));
                gtk_widget_destroy (w);
                                                                                
                if (res == GTK_RESPONSE_APPLY)
                        apply_config (dialog);
        }
}

static void
dialog_close (GstDialog *dialog)
{
	g_return_if_fail (dialog != NULL);
	g_return_if_fail (GST_IS_DIALOG (dialog));

	gtk_widget_hide (GTK_WIDGET (dialog));

	if (dialog == dialog->tool->main_dialog)
		g_signal_emit_by_name (G_OBJECT (dialog->tool), "close");
}

static void
cancel_cb (GtkWidget *w, gpointer data)
{
	GstDialog *dialog;

	g_return_if_fail (data != NULL);
	g_return_if_fail (GST_IS_DIALOG (data));

	dialog = GST_DIALOG (data);

	dialog_close (dialog);
}

static void
accept_cb (GtkWidget *w, gpointer data)
{
	GstDialog *dialog;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (GST_IS_DIALOG (data));

	dialog = GST_DIALOG (data);

	if (gst_dialog_get_modified (dialog))
		apply_config (dialog);
	
	dialog_close (dialog);
}

static void
help_cb (GtkWidget *w, gpointer data)
{
	GstDialog *gst_dialog;

	gst_dialog = GST_DIALOG (data);
	gst_tool_show_help (gst_dialog->tool, NULL);
}

static gboolean
dialog_delete_event_cb (GtkWidget *w, GdkEvent *event, gpointer data)
{
	cancel_cb (w, data);
	return TRUE;
}

static gboolean
dialog_key_press_event_cb (GtkWidget *w, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_Escape) {
		cancel_cb (w, data);
		return TRUE;
	}

	return FALSE;
}

static void
dialog_connect_signals (GstDialog *dialog, GstDialogSignal *signals, gboolean connect_after)
{       
	GtkWidget *w;
	guint sig;
	int i;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (GST_IS_DIALOG (dialog));

	for (i=0; signals[i].widget; i++) {
		w = gst_dialog_get_widget (dialog, signals[i].widget);

		if (connect_after) {
			sig = g_signal_connect_after (G_OBJECT (w),
						      signals[i].signal_name,
						      G_CALLBACK (signals[i].func),
						      (gpointer)dialog);
		} else {
			sig = g_signal_connect (G_OBJECT (w),
						signals[i].signal_name,
						G_CALLBACK (signals[i].func),
						(gpointer)dialog);
		}
		
		if (!sig)
			g_error (_("Error connection signal `%s' in widget `%s'"),
				 signals[i].signal_name, signals[i].widget);
	}
}

void
gst_dialog_connect_signals (GstDialog *dialog, GstDialogSignal *signals)
{
	dialog_connect_signals (dialog, signals, FALSE);
}

void
gst_dialog_connect_signals_after (GstDialog *dialog, GstDialogSignal *signals)
{
	dialog_connect_signals (dialog, signals, TRUE);
}

void
gst_dialog_construct (GstDialog *dialog, GstTool *tool,
		      const char *widget, const char *title)
{
	GladeXML     *xml;
	GtkWidget    *w;
	char         *s;
	GtkSizeGroup *size_group;

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (GST_IS_DIALOG (dialog));
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (title != NULL);

	dialog->tool = tool;
	dialog->apply_hook_list = NULL;

	s = g_strdup_printf ("%s-admin", tool->name);
	gnome_app_construct (GNOME_APP (dialog), s, title);
	g_free (s);

	xml = gst_tool_load_glade_common (tool, "tool_vbox");
	w = glade_xml_get_widget (xml, "tool_vbox");
	gnome_app_set_contents (GNOME_APP (dialog), w);

	dialog->gui   = gst_tool_load_glade (tool, NULL);
	dialog->child = gst_dialog_get_widget (dialog, widget);

	if (GTK_WIDGET_TOPLEVEL (dialog->child)) {
		g_error ("The widget \"%s\" should not be a toplevel widget in the .glade file\n"
			 "You just need to add the widget inside a GtkWindow so that it can be deparented.", widget);
	}

	gtk_widget_ref (dialog->child);
	gtk_widget_unparent (dialog->child);
	gtk_box_pack_start (GTK_BOX (w), dialog->child, TRUE, TRUE, 0);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

	w = glade_xml_get_widget (xml, "help");
	g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (help_cb), dialog);
	gtk_size_group_add_widget (size_group, w);

	w = glade_xml_get_widget (xml, "cancel");
	g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (cancel_cb), dialog);
	gtk_size_group_add_widget (size_group, w);

	w = glade_xml_get_widget (xml, "accept");
	g_signal_connect (G_OBJECT (w), "clicked", G_CALLBACK (accept_cb), dialog);
	gtk_size_group_add_widget (size_group, w);
	
	gst_dialog_set_modified (dialog, FALSE);

	g_signal_connect (G_OBJECT (dialog), "delete_event", G_CALLBACK (dialog_delete_event_cb), dialog);
	g_signal_connect (G_OBJECT (tool->remote_dialog), "delete_event", G_CALLBACK (dialog_delete_event_cb), dialog);
	g_signal_connect (G_OBJECT (dialog), "key-press-event", G_CALLBACK (dialog_key_press_event_cb), dialog);
}

GstDialog *
gst_dialog_new (GstTool *tool, const char *widget, const char *title)
{
	GstDialog *dialog;

	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (GST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (title != NULL, NULL);

	dialog = GST_DIALOG (g_type_create_instance (GST_TYPE_DIALOG));
	
	gst_dialog_construct (dialog, tool, widget, title);

	return dialog;
}
