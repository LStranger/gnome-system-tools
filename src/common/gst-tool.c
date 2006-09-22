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
#endif

#include <string.h>

#include "gst-tool.h"
#include "gst-dialog.h"
#include "gst-platform-dialog.h"
#include "gst-marshal.h"


#define GST_TOOL_GET_PRIVATE (o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_TOOL, GstToolPrivate))

typedef struct _GstToolPrivate GstToolPrivate;

struct _GstToolPrivate {

};

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

G_DEFINE_ABSTRACT_TYPE (GstTool, gst_tool, G_TYPE_OBJECT);

/* FIXME
static GstReportHookEntry common_report_hooks[] = {
/*        key                 function                    type                      allow_repeat user_data */
/*	{ "end",              report_finished_cb,         GST_REPORT_HOOK_LOADSAVE, TRUE,        NULL },
	{ "platform_success", platform_set_current_cb,    GST_REPORT_HOOK_SAVE,     TRUE,        NULL }, 
	{ "platform_unsup",   platform_unsupported_cb,    GST_REPORT_HOOK_LOADSAVE, TRUE,        NULL },
	{ "platform_undet",   platform_unsupported_cb,    GST_REPORT_HOOK_LOADSAVE, TRUE,        NULL },
	{ NULL,               NULL,                       -1,                       FALSE,       NULL }
};
*/

static gchar *location_id = NULL;    /* Location in which we are editing */

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
	g_type_class_add_private (object_class,
				  sizeof (GstToolPrivate));
}

static gboolean
report_window_close_cb (GtkWidget *widget, GdkEventAny *event, gpointer data)
{
	gtk_widget_hide (widget);
	return TRUE;
}

static void
gst_tool_init (GstTool *tool)
{
	GdkPixbuf *pixbuf;
	GladeXML  *xml;

	tool->icon_theme = gtk_icon_theme_get_default ();
	tool->glade_common_path  = INTERFACES_DIR "/common.glade";

	tool->session = oobs_session_get ();
	tool->gconf_client = gconf_client_get_default ();

	xml = gst_tool_load_glade_common (tool, "report_window");

	tool->report_window = glade_xml_get_widget (xml, "report_window");
	tool->report_label = glade_xml_get_widget (xml, "report_label");
	tool->report_progress = glade_xml_get_widget (xml, "report_progress");
	tool->report_pixmap = glade_xml_get_widget (xml, "report_pixmap");
	g_signal_connect (G_OBJECT (tool->report_window), "delete_event",
			  G_CALLBACK (report_window_close_cb), tool);

	pixbuf = gtk_icon_theme_load_icon (tool->icon_theme, "gnome-system-config", 48, 0, NULL);
	gtk_image_set_from_pixbuf (GTK_IMAGE (tool->report_pixmap), pixbuf);

	if (pixbuf)
		gdk_pixbuf_unref (pixbuf);

	g_object_unref (xml);
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
		tool->glade_path = g_strdup_printf (INTERFACES_DIR "/%s.glade", tool->name);

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
	/* FIXME: free stuff */
	(* G_OBJECT_CLASS (gst_tool_parent_class)->finalize) (object);
}

static void
gst_tool_impl_close (GstTool *tool)
{
	/* FIXME: add things to do when closing the tool */
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


/* --- Report hook and signal callbacks --- */
static void
on_platform_list_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GstTool *tool = GST_TOOL (data);
	gboolean selected;

	selected = gtk_tree_selection_get_selected (selection, NULL, NULL);
	gtk_widget_set_sensitive (tool->platform_ok_button, selected);
}

/*
static gboolean
platform_set_current_cb (GstTool *tool, GstReportLine *rline, gpointer data)
{
	GstPlatform *platform;

	platform = gst_platform_new_from_report_line (rline);
	g_return_val_if_fail (platform != NULL, TRUE);

	if (tool->current_platform)
		gst_platform_free (tool->current_platform);

	tool->current_platform = platform;
	tool->run_again = FALSE;
	return TRUE;
}

static gboolean
platform_unsupported_cb (GstTool *tool, GstReportLine *rline, gpointer data)
{
	tool->run_again = TRUE;

	return TRUE;
}
*/

/* --- GstTool --- */

void
gst_tool_add_supported_platform (GstTool *tool, GstPlatform *platform)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (platform != NULL);

	/* Avoid duplicates. Backend shouldn't serve duplicates, but
	 * we're paranoid here. */
	/* FIXME
	g_return_if_fail
		(g_slist_find_custom (tool->supported_platforms_list, platform,
		    (GCompareFunc) gst_platform_cmp) == NULL);

	tool->supported_platforms_list =
		g_slist_insert_sorted (tool->supported_platforms_list, platform,
		    (GCompareFunc) gst_platform_cmp);
	*/
}

void
gst_tool_clear_supported_platforms (GstTool *tool)
{
	GSList *list;
	GstPlatform *platform;

	/* FIXME

	g_return_if_fail (tool != NULL);

	for (list = tool->supported_platforms_list; list;
	     list = g_slist_next (list))
	{
		platform = (GstPlatform *) list->data;
		gst_platform_free (platform);
	}

	if (tool->supported_platforms_list)
	{
		g_slist_free (tool->supported_platforms_list);
		tool->supported_platforms_list = NULL;
	}
	*/
}

GladeXML *
gst_tool_load_glade_common (GstTool *tool, const gchar *widget)
{
	GladeXML *xml;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (GST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->glade_common_path != NULL, NULL);

	xml = glade_xml_new (tool->glade_common_path, widget, NULL);

	if (!xml) {
		g_error ("Could not load %s\n", tool->glade_common_path);
	}

	return xml;
}

GladeXML *
gst_tool_load_glade (GstTool *tool, const gchar *widget)
{
	GladeXML *xml;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (GST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->glade_path != NULL, NULL);

	xml = glade_xml_new (tool->glade_path, widget, NULL);

	if (!xml) {
		g_error ("Could not load %s\n", tool->glade_path);
	}
	
	return xml;
}

GstDialog *
gst_tool_get_dialog (GstTool *tool)
{
	return tool->main_dialog;
}

gboolean
gst_tool_is_authenticated (GstTool *tool)
{
	/* FIXME */
	return TRUE;
}

static GSList *
gst_tool_get_supported_platforms (GstTool *tool)
{
/*
	xmlDoc *doc;
	xmlNode *root, *node;
	GSList *list;
	GstPlatform *plat;
	
	doc = gst_tool_run_get_directive (tool, NULL, "platforms", NULL);
	list = NULL;

	g_return_val_if_fail (doc != NULL, NULL);
	root = gst_xml_doc_get_root (doc);
	for (node = gst_xml_element_find_first (root, "platform"); node;
	     node = gst_xml_element_find_next (node, "platform")) {
		plat = gst_platform_new_from_node (node);
		if (!plat)
			continue;
		list = g_slist_append (list, plat);
	}

	return list;
*/
}

/*
static void
gst_tool_fill_platform_tree (GstTool *tool)
{
	GSList *list;
	GstPlatform *platform;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *label = NULL;

	gst_tool_create_distro_images ();

	/* Fill in the platform GtkTreeView */
/*	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tool->platform_list));

	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	tool->supported_platforms_list = gst_tool_get_supported_platforms (tool);
	
	for (list = tool->supported_platforms_list; list; list = g_slist_next (list)) 
	{
		platform = (GstPlatform *) list->data;
		
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 
		                    PLATFORM_LIST_COL_LOGO, gst_platform_get_pixmap (platform), 
		                    PLATFORM_LIST_COL_NAME, gst_platform_get_name (platform), 
		                    PLATFORM_LIST_COL_PLATFORM, platform,
		                    -1);
	}
}

static void
gst_tool_fill_remote_hosts_list (GstTool *tool, gchar *hosts_list[])
{
	gchar **host;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tool->remote_hosts_list));
	GtkTreeIter iter;

	for (host = hosts_list; *host != NULL; host++) {
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
				    0, *host,
				    -1);
	}
}
*/

static const gchar*
gst_tool_run_platform_dialog (GstTool *tool)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gchar *platform;
	gint result;
	
	//gst_tool_fill_platform_tree (tool);
	gtk_widget_set_sensitive (tool->platform_ok_button, FALSE);
	
	result = gtk_dialog_run (GTK_DIALOG (tool->platform_dialog));
	gtk_widget_hide (tool->platform_dialog);

	if (result != GTK_RESPONSE_OK)
		exit (0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tool->platform_list));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    PLATFORM_LIST_COL_ID, &platform,
				    -1);

		return platform;
	}

	return NULL;
}

void
gst_tool_process_startup (GstTool *tool)
{
	/* let's process those startup reports. */
/*
	tool->report_hook_type = GST_REPORT_HOOK_LOAD;

	report_progress (tool);

	/* stable version users should pass here only once,
	   but if there's an inconsistency the dialog will repeat */
/*	while (tool->run_again) {
		gst_tool_run_platform_dialog (tool);
		g_assert (tool->current_platform);

		if (tool->root_access == ROOT_ACCESS_SIMULATED)
			tool->root_access = ROOT_ACCESS_SIMULATED_DISABLED;

		gst_tool_run_set_directive (tool, NULL, NULL, "platform_set",
					    gst_platform_get_key (tool->current_platform), NULL);

		if (tool->root_access == ROOT_ACCESS_SIMULATED_DISABLED)
			tool->root_access = ROOT_ACCESS_SIMULATED;
	}
*/
}

static void
gst_tool_kill_backend_cb (GstDirectiveEntry *entry)
{
/*
	GstTool *tool = entry->tool;

	/* removes the g_timeout that's waiting for the backend to die */
/*	g_source_remove (tool->timeout_id);

	/* The backend was never called. No problem! */
	/* For further reference check http://www.xs4all.nl/~pjbrink/apekool/050/045-en.html */
/*	if (tool->backend_pid > 0) {
		if (tool->root_access == ROOT_ACCESS_SIMULATED)
			tool->root_access = ROOT_ACCESS_SIMULATED_DISABLED;

		gst_tool_run_set_directive (tool, NULL, NULL, "end", NULL);
		
		if (tool->root_access == ROOT_ACCESS_SIMULATED_DISABLED)
			tool->root_access = ROOT_ACCESS_SIMULATED;

		waitpid (tool->backend_pid, NULL, 0);
	}
*/
}

static void
gst_tool_kill_tool_cb (GstDirectiveEntry *entry)
{
/*
	GstTool *tool = entry->tool;

	gst_tool_kill_backend_cb (entry);

	g_object_unref (G_OBJECT (tool));
	exit (0);
*/
}

/* kills the whole tool */
static void
gst_tool_kill_tool (GstTool *tool, gpointer data)
{
/*
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));

	gst_tool_queue_directive (tool, gst_tool_kill_tool_cb, NULL, NULL, NULL, "end");
*/
}

/* only kills the backend */
static void
gst_tool_kill_backend (GstTool *tool, gpointer data)
{
/*
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));

	gst_tool_queue_directive (tool, gst_tool_kill_backend_cb, NULL, NULL, NULL, "end");
*/
}

/*
gboolean
gst_tool_load (GstTool *tool)
{
	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (GST_IS_TOOL (tool), FALSE);	
	g_return_val_if_fail (tool->script_path, FALSE);
  
	if (tool->config) {
		xmlFreeDoc (tool->config);
		xmlFreeDoc (tool->original_config);
		tool->config = NULL;
		tool->original_config = NULL;
	}

	if (tool->run_again)
		return TRUE;

	if (location_id == NULL) {
		tool->config = gst_tool_run_get_directive (tool, NULL, "get", NULL);
		tool->original_config = xmlCopyDoc (tool->config, 1);
	} else {
		g_assert_not_reached ();
	}

	if (tool->config)
		g_signal_emit (G_OBJECT (tool), gsttool_signals[FILL_GUI], 0);

	return tool->config != NULL;
}
*/

gboolean
gst_tool_save (GstTool *tool)
{
/*
	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (GST_IS_TOOL (tool), FALSE);
	g_return_val_if_fail (tool->root_access != ROOT_ACCESS_NONE, FALSE);

	gst_dialog_freeze_visible (tool->main_dialog);

	g_signal_emit (G_OBJECT (tool), gsttool_signals[FILL_XML], 0);

	if (location_id == NULL) {
		if (restore)
			gst_tool_run_set_directive (tool, tool->original_config, NULL, "set", NULL);
		else
			gst_tool_run_set_directive (tool, tool->config, NULL, "set", NULL);
	}
	
	gst_dialog_thaw_visible (tool->main_dialog);
*/
	return TRUE;  /* FIXME: Determine if it really worked. */
}

/*
void
gst_tool_load_try (GstTool *tool)
{
	if (!gst_tool_load (tool)) {
		GtkWidget *d;

		d = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					    GTK_DIALOG_MODAL,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_CLOSE,
					    _("The configuration could not be loaded"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d),
							  _("There was an error running the backend script"));
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);
		
		exit (0);
	}
}

static void
tool_main_do (GstTool *tool, gboolean no_main_loop, gboolean show_main_dialog)
{
	if (show_main_dialog)
		gtk_widget_show (GTK_WIDGET (tool->main_dialog));
	
	gst_dialog_freeze_visible (tool->main_dialog);

	if (tool->remote_config == TRUE) {
		/* run the ssh client */
/*		gst_tool_fill_remote_hosts_list (tool, tool->remote_hosts);
		gtk_widget_show (tool->remote_dialog);
	} else {
		/* run the su command */
/*		gst_auth_do_su_authentication (tool);

		gst_tool_process_startup (tool);
		gst_dialog_apply_widget_policies (tool->main_dialog);

		gst_tool_load_try (tool);
		gst_dialog_thaw_visible (tool->main_dialog);
	}

	if (!no_main_loop)
		gtk_main ();
}

void
gst_tool_main (GstTool *tool, gboolean no_main_loop)
{
	tool_main_do (tool, no_main_loop, TRUE);
}

void
gst_tool_main_with_hidden_dialog (GstTool *tool, gboolean no_main_loop)
{
	tool_main_do (tool, no_main_loop, FALSE);
}

static void
gst_tool_finalize (GObject *object)
{
	parent_class->finalize (object);
	gtk_main_quit ();
}

static void
gst_tool_class_init (GstToolClass *class)
{	
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (class);
	parent_class = g_type_class_peek_parent (class);

	gsttool_signals[FILL_GUI] = 
		g_signal_new ("fill_gui",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstToolClass, fill_gui),
			      NULL, NULL,
			      gst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	gsttool_signals[FILL_XML] = 
		g_signal_new ("fill_xml",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstToolClass, fill_xml),
			      NULL, NULL,
			      gst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	gsttool_signals[CLOSE] = 
		g_signal_new ("close",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstToolClass, close),
			      NULL, NULL,
			      gst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	gobject_class->finalize = gst_tool_finalize;
}
*/

/*
static void
on_remote_hosts_list_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GstTool *tool = GST_TOOL (data);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *host;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		/* freeze the dialog */
/*		gtk_widget_set_sensitive (tool->remote_dialog, FALSE);
		
		gtk_tree_model_get (model, &iter, 0, &host, -1);

		if (tool->backend_pid > 0) {
			gst_dialog_ask_apply (tool->main_dialog);
			gst_dialog_set_modified (tool->main_dialog, FALSE);

			/* stops the backend */
/*			gst_tool_kill_backend (tool, NULL);

			/* process the "end" directive */
/*			while (gtk_events_pending())
				gtk_main_iteration();

			/* remove current platform info, in the new host may be another one */
/*			if (tool->current_platform != NULL) {
				gst_platform_free (tool->current_platform);
				tool->current_platform = NULL;
			}
		}

		gst_dialog_freeze_visible (tool->main_dialog);
	
		gst_auth_do_ssh_authentication (tool, host);
		gst_tool_process_startup (tool);

		gst_tool_load_try (tool);
		gst_dialog_apply_widget_policies (tool->main_dialog);

		/* thaw all the dialogs */
/*		gst_dialog_thaw_visible (tool->main_dialog);
		gtk_widget_set_sensitive (tool->remote_dialog, TRUE);
	}
}

static void
gst_tool_create_remote_hosts_list (GtkTreeView *list, GstTool *tool)
{
	GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_store_new (1, G_TYPE_STRING));
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	g_return_if_fail (list != NULL);
	g_return_if_fail (GTK_IS_TREE_VIEW (list));

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
	g_object_unref (model);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list),
						     -1,
						     NULL,
						     renderer,
						     "text", 0,
						     NULL);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (on_remote_hosts_list_selection_changed),
			  (gpointer) tool);
}

static void
gst_tool_type_init (GstTool *tool)
{
	GdkPixbuf *pixbuf;
	GladeXML  *xml;

	tool->icon_theme = gtk_icon_theme_get_default ();
	tool->glade_common_path  = g_strdup_printf ("%s/common.glade", INTERFACES_DIR);

	/* load the report window */
/*	xml = gst_tool_load_glade_common (tool, "report_window");

	tool->report_window = glade_xml_get_widget (xml, "report_window");
	g_signal_connect (G_OBJECT (tool->report_window), "delete_event",
			  G_CALLBACK (report_window_close_cb), tool);

	tool->report_label    = glade_xml_get_widget (xml, "report_label");
	tool->report_progress = glade_xml_get_widget (xml, "report_progress");
	tool->report_pixmap   = glade_xml_get_widget (xml, "report_pixmap");

	pixbuf = gtk_icon_theme_load_icon (tool->icon_theme, "gnome-system-config", 48, 0, NULL);
	gtk_image_set_from_pixbuf (GTK_IMAGE (tool->report_pixmap), pixbuf);
	gdk_pixbuf_unref (pixbuf);

	/* load the platforms list */
/*	xml = gst_tool_load_glade_common (tool, "platform_dialog");
	tool->platform_list = glade_xml_get_widget (xml, "platform_list");
	gst_tool_create_platform_list (GTK_TREE_VIEW (tool->platform_list), tool);

	tool->platform_dialog    = glade_xml_get_widget (xml, "platform_dialog");
	tool->platform_ok_button = glade_xml_get_widget (xml, "platform_ok_button");

	/* load the remote config dialog */
/*	xml = gst_tool_load_glade_common (tool, "remote_dialog");
	tool->remote_dialog = glade_xml_get_widget (xml, "remote_dialog");
	tool->remote_hosts_list = glade_xml_get_widget (xml, "remote_hosts_list");
	gst_tool_create_remote_hosts_list (GTK_TREE_VIEW (tool->remote_hosts_list), tool);
}

GtkType
gst_tool_get_type (void)
{
	static GType gsttool_type = 0;

	if (gsttool_type == 0) {
		GTypeInfo gsttool_info = {
			sizeof (GstToolClass),
			NULL, /* base_init */
/*			NULL, /* base finalize */
/*			(GClassInitFunc) gst_tool_class_init,
			NULL, /* class_finalize */
/*			NULL, /* class_data */
/*			sizeof (GstTool),
			0, /* n_preallocs */
/*			(GInstanceInitFunc) gst_tool_type_init
		};
		
		gsttool_type = g_type_register_static (G_TYPE_OBJECT, "GstTool", &gsttool_info, 0);
	}

	return gsttool_type;
}
*/


/**
 * gst_tool_construct:
 * @tool: 
 * @name: 
 * @title: 
 * 
 * 
 * 
 * Return Value: FALSE on error
 **/
/*
void
gst_tool_construct (GstTool *tool, const char *name, const char *title)
{
	char *s, *t, *u;
	GConfClient *client = gconf_client_get_default ();

	gchar *remote_config_key;
	gchar *remote_hosts_key;
	gchar *hosts;
	gboolean do_remote_conf;

	g_return_if_fail (name != NULL);

	tool->name        = g_strdup (name);
	tool->glade_path  = g_strdup_printf ("%s/%s.glade",     INTERFACES_DIR, name);
	tool->script_path = g_strdup_printf ("%s/%s-conf",      SCRIPTS_DIR,    name);
	tool->script_name = g_strdup_printf ("%s-conf",         name);

	s = g_strdup_printf ("%s_admin", name);
	t = g_strdup_printf ("%s", title);
	u = g_strdup_printf (PIXMAPS_DIR "/%s.png", name);

	tool->main_dialog = gst_dialog_new (tool, s, t);

	gtk_window_set_default_icon_from_file (u, NULL);

	g_free (s);
	g_free (t);
	g_free (u);

	g_signal_connect (G_OBJECT (tool->main_dialog),
			  "apply",
			  G_CALLBACK (gst_tool_save_cb),
			  tool);

	tool->report_hook_list = NULL;
	memset (&tool->report_hook_defaults, 0, sizeof (GstReportHook *) * GST_MAJOR_MAX);
	gst_tool_add_report_hooks (tool, common_report_hooks);

	tool->backend_pid = -1;
	tool->write_fd = -1;
	tool->read_fd = -1;
	gst_tool_set_close_func (tool, gst_tool_kill_tool, NULL);

	tool->directive_running = FALSE;
	tool->directive_queue   = NULL;
	tool->directive_queue_idle_id = 0;

	/* get the remote configuration gconf keys */
/*	remote_config_key = g_strjoin ("/", GST_GCONF_ROOT, "global", "remote_configuration", NULL);
	remote_hosts_key = g_strjoin ("/", GST_GCONF_ROOT, "global", "remote_hosts_list", NULL);

	do_remote_conf = gconf_client_get_bool (client, remote_config_key, NULL);
	hosts = gconf_client_get_string (client, remote_hosts_key, NULL);

	tool->remote_config = ((do_remote_conf) && (hosts != NULL) && (strcmp (SSH_PATH, "") != 0));
	if (tool->remote_config && hosts)
		tool->remote_hosts = g_strsplit (hosts, ",", -1);

	g_free (hosts);
	g_free (remote_config_key);
	g_free (remote_hosts_key);
}


GstTool *
gst_tool_new (void)
{
	GstTool *tool;

	tool = GST_TOOL (g_type_create_instance (GST_TYPE_TOOL));

	return tool;
}

void
gst_tool_set_xml_funcs (GstTool *tool, GstXmlFunc load_cb, GstXmlFunc save_cb, gpointer data)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));

	if (load_cb)
		g_signal_connect (G_OBJECT (tool), "fill_gui", G_CALLBACK (load_cb), data);

	if (save_cb)
		g_signal_connect (G_OBJECT (tool), "fill_xml", G_CALLBACK (save_cb), data);
}

void
gst_tool_set_close_func (GstTool *tool, GstCloseFunc close_cb, gpointer data)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));

	if (close_cb)
		g_signal_connect (G_OBJECT (tool), "close", G_CALLBACK (close_cb), data);
}

void
gst_tool_add_report_hooks (GstTool *tool, GstReportHookEntry *report_hook_table)
{
	GstReportHook *hook;
	int i;

	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));
	g_return_if_fail (report_hook_table != NULL);

	for (i = 0; report_hook_table [i].key; i++)
        {
		hook = gst_report_hook_new_from_entry (&report_hook_table [i]);

		if (!hook)
			continue;

		tool->report_hook_list = g_slist_append (tool->report_hook_list, hook);
	}
}

void
gst_tool_set_default_hook (GstTool *tool, GstReportHookEntry *entry, GstReportMajor major)
{
	g_return_if_fail (major < GST_MAJOR_MAX);
	
	if (tool->report_hook_defaults[major])
		g_free (tool->report_hook_defaults[major]);

	tool->report_hook_defaults[major] = gst_report_hook_new_from_entry (entry);
}

static gboolean
gst_tool_call_report_hook (GstTool *tool, GstReportHook *hook, GstReportLine *rline)
{
	gboolean res;
	
	hook->invoked = TRUE;
	tool->input_block = TRUE;
	res = hook->func (tool, rline, hook->data);
	tool->input_block = FALSE;
	return res;
}

static void
gst_tool_invoke_default_hook (GstTool *tool, GstReportLine *rline)
{
	GstReportHook *hook;
	
	if (tool->report_hook_defaults[rline->major]) {
		hook = tool->report_hook_defaults[rline->major];
		if ((hook->allow_repeat || !hook->invoked) &&
		    (hook->type == GST_REPORT_HOOK_LOADSAVE ||
		     hook->type == tool->report_hook_type)) {
			gst_tool_call_report_hook (tool, hook, rline);
		}
	}
}

void
gst_tool_invoke_report_hooks (GstTool *tool, GstReportHookType type, GstReportLine *rline)
{
	GSList *list;
	GstReportHook *hook;
	const gchar *key;
	gboolean invoked;

	key = gst_report_line_get_key (rline);
	invoked = FALSE;

	for (list = tool->report_hook_list; list; list = g_slist_next (list))
	{
		hook = (GstReportHook *) list->data;

		if (!strcmp (hook->key, key) && (hook->allow_repeat || !hook->invoked) &&
		    (hook->type == GST_REPORT_HOOK_LOADSAVE || hook->type == type)) {
			invoked = TRUE;
			if (gst_tool_call_report_hook (tool, hook, rline))
				return;
		}
	}

	if (!invoked)
		gst_tool_invoke_default_hook (tool, rline);
}

void
gst_tool_reset_report_hooks (GstTool *tool)
{
	GSList *list;

	for (list = tool->report_hook_list; list; list = g_slist_next (list))
		((GstReportHook *) list->data)->invoked = FALSE;
}

static void
try_show_usage_warning (void)
{
	gchar *key, *version_key;
	gboolean value;
	gchar *version;
	GConfClient *client;
	GError *error = NULL;
	gchar *title         = g_strdup_printf (_("Welcome to the %s prerelease of the GNOME System Tools"), VERSION);
	gchar *warning_title = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", title);
	gchar *warning       = _("This is still a work in progress, and so it may have serious bugs. "
				 "Due to the nature of these tools, bugs may render your computer "
				 "<span weight=\"bold\">practically useless</span>, costing time, effort and sanity points. "
				 "You have been warned. Thank you for trying out this prerelease of "
				 "the GNOME System Tools!");

	client = gconf_client_get_default ();

	version_key = g_strjoin ("/", GST_GCONF_ROOT, "global", "last_version", NULL);
	key = g_strjoin ("/", GST_GCONF_ROOT, "global", "show_warning", NULL);


	value = gconf_client_get_bool (client, key, &error);
	version = gconf_client_get_string (client, version_key, &error);

	if ((value == TRUE) || version && (strcmp (version, VERSION) != 0))
	{
		GtkWidget *dialog, *label, *title_label, *image, *hbox, *vbox;
		GtkWidget *checkbox;
		gboolean dont_ask_again;
		
		dialog = gtk_dialog_new_with_buttons (_("Warning"),
						      NULL,
						      GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
						      GTK_STOCK_OK, GTK_RESPONSE_OK,
						      NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		title_label = gtk_label_new (warning_title);
		gtk_label_set_line_wrap (GTK_LABEL (title_label), TRUE);
		gtk_label_set_use_markup (GTK_LABEL (title_label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (title_label), 0, 0);
		
		label = gtk_label_new (warning);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (title_label), 0, 0);
		
		image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		gtk_misc_set_alignment (GTK_MISC (image), 0, 0);
		
		checkbox = gtk_check_button_new_with_label (_("Don't show me this again"));

		hbox = gtk_hbox_new (FALSE, 12);
		vbox = gtk_vbox_new (FALSE, 12);

		gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);

		gtk_box_pack_start (GTK_BOX (vbox), title_label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), checkbox, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		
		gtk_widget_show_all (dialog);
		
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox)) == TRUE) {
				gconf_client_set_bool (client, key, FALSE, &error);
				gconf_client_set_string (client, version_key, VERSION, &error);
			}
		}
		
		gtk_widget_hide (dialog);
		gtk_widget_destroy (dialog);
	}

	g_free (warning_title);
	g_free (title);
	g_free (key);
}

*/

void
gst_init_tool (const gchar *app_name, int argc, char *argv [], GOptionEntry *entries)
{
#ifdef ENABLE_GNOME
	GnomeProgram   *program;
#endif
	GOptionContext *context;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (entries) {
		context = g_option_context_new (NULL);
		g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
		g_option_context_add_group (context, gtk_get_option_group (TRUE));
		g_option_context_parse (context, &argc, &argv, NULL);
	}

#ifdef ENABLE_GNOME
 	program = gnome_program_init (app_name, VERSION,
				      LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PARAM_APP_DATADIR, DATADIR,
				      GNOME_PARAM_HUMAN_READABLE_NAME,
				      _("GNOME System Tools"),
				      NULL);
#else
	gtk_init (&argc, &argv);	
#endif
}

void
gst_tool_show_help (GstTool *tool, gchar *section)
{
#ifdef ENABLE_GNOME
	GError    *error = NULL;
	gchar     *help_file;

	help_file = g_strdup_printf ("%s-admin", tool->name);
	gnome_help_display (help_file, section, &error);

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

	g_free (help_file);
#endif
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

	g_return_if_fail (tool->report_timeout_id == 0);

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
	gst_tool_hide_report_window (GST_TOOL (data));
}

void
gst_tool_commit_async (GstTool     *tool,
		       OobsObject  *object,
		       const gchar *message)
{
	gst_tool_show_report_window (tool, message);
	oobs_object_commit_async (object, on_commit_finalized, tool);
}

GtkIconTheme*
gst_tool_get_icon_theme (GstTool *tool)
{
	return tool->icon_theme;
}
