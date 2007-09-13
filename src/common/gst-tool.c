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
 * Authors: Jacob Berkman         <jacob@ximian.com>
 *          Hans Petter Jansson   <hpj@ximian.com>
 *          Carlos Garnacho Parro <carlosg@gnome.org>
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>

#ifdef ENABLE_GNOME
#include <libgnomeui/libgnomeui.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-help.h>
#else
#include <stdlib.h>
#endif

#include <string.h>
#include "gst-tool.h"
#include "gst-dialog.h"
#include "gst-platform-dialog.h"

enum {
	PLATFORM_LIST_COL_LOGO,
	PLATFORM_LIST_COL_NAME,
	PLATFORM_LIST_COL_ID,
	PLATFORM_LIST_COL_LAST
};

static void  gst_tool_class_init   (GstToolClass *class);
static void  gst_tool_init         (GstTool      *tool);
static void  gst_tool_finalize     (GObject      *object);

static GObject* gst_tool_constructor (GType                  type,
				      guint                  n_construct_properties,
				      GObjectConstructParam *construct_params);
static void  gst_tool_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec);

static void gst_tool_impl_close    (GstTool *tool);

enum {
	PROP_0,
	PROP_NAME,
	PROP_TITLE,
	PROP_ICON
};

typedef struct _GstAsyncData {
	GstTool *tool;
	OobsObjectAsyncFunc func;
	gpointer data;
} GstAsyncData;

G_DEFINE_ABSTRACT_TYPE (GstTool, gst_tool, G_TYPE_OBJECT);

static void
gst_tool_class_init (GstToolClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->set_property = gst_tool_set_property;
	object_class->constructor  = gst_tool_constructor;
	object_class->finalize     = gst_tool_finalize;

	class->close = gst_tool_impl_close;
	class->update_gui = NULL;
	class->update_config = NULL;

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "name",
							      "Tool name",
							      NULL,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_TITLE,
					 g_param_spec_string ("title",
							      "title",
							      "Tool title",
							      NULL,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_ICON,
					 g_param_spec_string ("icon",
							      "icon",
							      "Tool icon",
							      NULL,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static gboolean
report_window_close_cb (GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	gtk_widget_hide (widget);
	return TRUE;
}

static GtkBuilder *
gst_tool_load_common_ui (GstTool *tool)
{
	GtkBuilder *builder;

	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (GST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->common_ui_path != NULL, NULL);

	builder = gtk_builder_new ();

	if (!gtk_builder_add_from_file (builder, tool->common_ui_path, NULL)) {
		g_error ("Could not load %s\n", tool->common_ui_path);
	}

	return builder;
}

static void
gst_tool_init (GstTool *tool)
{
	GdkPixbuf  *pixbuf;
	GtkBuilder *builder;

	tool->icon_theme = gtk_icon_theme_get_default ();
	tool->common_ui_path  = INTERFACES_DIR "/common.ui";

	tool->session = oobs_session_get ();
	tool->gconf_client = gconf_client_get_default ();

	builder = gst_tool_load_common_ui (tool);

	tool->report_window = GTK_WIDGET (gtk_builder_get_object (builder, "report_window"));
	tool->report_label = GTK_WIDGET (gtk_builder_get_object (builder, "report_label"));
	tool->report_progress = GTK_WIDGET (gtk_builder_get_object (builder, "report_progress"));
	tool->report_pixmap = GTK_WIDGET (gtk_builder_get_object (builder, "report_pixmap"));
	g_signal_connect (G_OBJECT (tool->report_window), "delete_event",
			  G_CALLBACK (report_window_close_cb), tool);

	pixbuf = gtk_icon_theme_load_icon (tool->icon_theme, "gnome-system-config", 48, 0, NULL);
	gtk_image_set_from_pixbuf (GTK_IMAGE (tool->report_pixmap), pixbuf);

	if (pixbuf)
		gdk_pixbuf_unref (pixbuf);

	g_object_unref (builder);
}

static void
show_access_denied_dialog (GstTool *tool)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 _("The configuration could not be loaded"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("You are not allowed to access the system configuration."));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static GObject*
gst_tool_constructor (GType                  type,
		      guint                  n_construct_properties,
		      GObjectConstructParam *construct_params)
{
	GObject *object;
	GstTool *tool;
	gchar *widget_name;
	const gchar *platform;
	GtkWidget *dialog;
	OobsResult result;

	object = (* G_OBJECT_CLASS (gst_tool_parent_class)->constructor) (type,
									  n_construct_properties,
									  construct_params);
	tool = GST_TOOL (object);

	if (tool->title)
		g_set_application_name (tool->title);

	if (tool->icon)
		gtk_window_set_default_icon_name (tool->icon);

	if (tool->name) {
		tool->ui_path = g_strdup_printf (INTERFACES_DIR "/%s.ui", tool->name);

		widget_name = g_strdup_printf ("%s_admin", tool->name);
		tool->main_dialog = gst_dialog_new (tool, widget_name, tool->title);
		g_free (widget_name);
	}

	result = oobs_session_get_platform (tool->session, NULL);

	switch (result) {
	case OOBS_RESULT_NO_PLATFORM:
		dialog = gst_platform_dialog_new (tool->session);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		gst_tool_update_config (tool);
		break;
	case OOBS_RESULT_ACCESS_DENIED:
	case OOBS_RESULT_ERROR:
		show_access_denied_dialog (tool);
		exit (-1);
		break;
	default:
		break;
	}

	return object;
}

static void
gst_tool_set_property (GObject      *object,
		       guint         prop_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	GstTool *tool = GST_TOOL (object);

	switch (prop_id) {
	case PROP_NAME:
		tool->name = g_value_dup_string (value);
		break;
	case PROP_TITLE:
		tool->title = g_value_dup_string (value);
		break;
	case PROP_ICON:
		tool->icon = g_value_dup_string (value);
		break;
	}
}

static void
gst_tool_finalize (GObject *object)
{
	GstTool *tool = GST_TOOL (object);

	g_free (tool->name);
	g_free (tool->title);
	g_free (tool->icon);
	g_free (tool->ui_path);

	if (tool->session)
		g_object_unref (tool->session);

	if (tool->main_dialog)
		gtk_widget_destroy (GTK_WIDGET (tool->main_dialog));

	if (tool->report_window)
		gtk_widget_destroy (tool->report_window);

	if (tool->gconf_client)
		g_object_unref (tool->gconf_client);

	(* G_OBJECT_CLASS (gst_tool_parent_class)->finalize) (object);
}

static void
gst_tool_impl_close (GstTool *tool)
{
	gtk_widget_hide (GTK_WIDGET (tool->main_dialog));

	/* process necessary events to hide the dialog */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	/* process pending async requests */
	oobs_session_process_requests (tool->session);


	g_object_unref (tool);
	gtk_main_quit ();
}

void
gst_tool_update_gui (GstTool *tool)
{
	g_return_if_fail (GST_IS_TOOL (tool));

	if (GST_TOOL_GET_CLASS (tool)->update_gui)
		(* GST_TOOL_GET_CLASS (tool)->update_gui) (tool);
}

void
gst_tool_update_config (GstTool *tool)
{
	g_return_if_fail (GST_IS_TOOL (tool));

	if (GST_TOOL_GET_CLASS (tool)->update_config)
		(* GST_TOOL_GET_CLASS (tool)->update_config) (tool);
}

void
gst_tool_close (GstTool *tool)
{
	g_return_if_fail (GST_IS_TOOL (tool));

	if (GST_TOOL_GET_CLASS (tool)->close)
		(* GST_TOOL_GET_CLASS (tool)->close) (tool);
}

gboolean
gst_tool_is_authenticated (GstTool *tool)
{
	/* FIXME */
	return TRUE;
}

void
gst_init_tool (const gchar *app_name, int argc, char *argv [], GOptionEntry *entries)
{
	GOptionContext *context;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (entries) {
		context = g_option_context_new (NULL);
		g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
		g_option_context_add_group (context, gtk_get_option_group (TRUE));
		g_option_context_parse (context, &argc, &argv, NULL);
		g_option_context_free (context);
	}

	gtk_init (&argc, &argv);
}

void
gst_tool_show_help (GstTool *tool, gchar *section)
{
	GdkScreen *screen;
	GError *error = NULL;
	gchar *help_file, *help_file_xml, *command, *uri;
	const gchar **langs, *lang;
	gint i;

	langs = (const gchar **) g_get_language_names ();
	help_file = g_strdup_printf ("%s-admin", tool->name);
	help_file_xml = g_strdup_printf ("%s-admin.xml", tool->name);

	for (i = 0; langs[i]; i++) {
		lang = langs[i];

		if (strchr (lang, '.')) {
			continue;
		}

		uri = g_build_filename(DATADIR,
				       "/gnome/help/",
				       help_file,
				       lang,
				       help_file_xml,
				       NULL);

		if (g_file_test (uri, G_FILE_TEST_EXISTS)) {
                    break;
		}
	}

	if (section) {
		command = g_strconcat ("gnome-help ghelp://", uri, "?", section, NULL);
	} else {
		command = g_strconcat ("gnome-help ghelp://", uri, NULL);
	}

	screen = gtk_window_get_screen (GTK_WINDOW (tool->main_dialog));
	gdk_spawn_command_line_on_screen (screen, command, &error);
	g_free (command);
	g_free (uri);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Could not display help"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	}

	g_free (help_file_xml);
	g_free (help_file);
}

static gboolean
gst_tool_report_progress_animate (GstTool *tool)
{
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (tool->report_progress));
	return TRUE;
}

static gboolean
gst_tool_report_window_timeout (GstTool *tool)
{
	gtk_window_set_transient_for (GTK_WINDOW (tool->report_window), GTK_WINDOW (tool->main_dialog));
	gtk_widget_show (tool->report_window);

	tool->report_timeout_id = 0;
	return FALSE;
}

static void
gst_tool_show_report_window (GstTool *tool, const gchar *report)
{
	gchar *markup;

	if (tool->report_timeout_id != 0)
		return;

	if (report) {
		markup = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", report);
		gtk_label_set_markup (GTK_LABEL (tool->report_label), markup);
		g_free (markup);

		tool->report_timeout_id = g_timeout_add (2000, (GSourceFunc) gst_tool_report_window_timeout, tool);
		tool->report_animate_id = g_timeout_add (150,  (GSourceFunc) gst_tool_report_progress_animate, tool);
	}
}

static void
gst_tool_hide_report_window (GstTool *tool)
{
	if (tool->report_timeout_id) {
		g_source_remove (tool->report_timeout_id);
		tool->report_timeout_id = 0;
	}

	if (tool->report_animate_id) {
		g_source_remove (tool->report_animate_id);
		tool->report_animate_id = 0;
	}

	gtk_widget_hide (tool->report_window);
}

static void
on_commit_finalized (OobsObject *object,
		     OobsResult  result,
		     gpointer    data)
{
	GstAsyncData *user_data = (GstAsyncData *) data;

	gst_tool_hide_report_window (user_data->tool);

	if (user_data->func)
		(* user_data->func) (object, result, user_data->data);

	g_slice_free (GstAsyncData, user_data);
}

void
gst_tool_commit_async (GstTool             *tool,
		       OobsObject          *object,
		       const gchar         *message,
		       OobsObjectAsyncFunc  func,
		       gpointer             data)
{
	GstAsyncData *user_data;

	user_data = g_slice_new (GstAsyncData);
	user_data->tool = tool;
	user_data->func = func;
	user_data->data = data;

	gst_tool_show_report_window (tool, message);
	oobs_object_commit_async (object, on_commit_finalized, user_data);
}

GtkIconTheme*
gst_tool_get_icon_theme (GstTool *tool)
{
	return tool->icon_theme;
}
