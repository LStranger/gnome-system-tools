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
 *          Hans Petter Jansson <hpj@ximian.com>
 *          Carlos Garnacho Parro <garparr@teleline.es>
 */

#include <config.h>

#include "gst-tool.h"
#include "gst-dialog.h"
#include "gst-report-line.h"
#include "gst-report-hook.h"
#include "gst-platform.h"
#include "gst-ui.h"
#include "gst-auth.h"
#include "gst-xml.h"
#include "gst-marshal.h"

#ifdef GST_HAVE_ARCHIVER
#  include <bonobo.h>
#  include <config-archiver/archiver-client.h>
#endif

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

#include <gconf/gconf-client.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_SIZE 2048
#define GST_DEBUG 1
#define GST_TOOL_EOR "<!-- GST: end of request -->"

/* Define this if you want the frontend to run the backend under strace */
/*#define GST_DEBUG_STRACE_BACKEND*/

/* pixmaps used for distros/OS's */
GdkPixbuf *redhat;
GdkPixbuf *debian;
GdkPixbuf *mandrake;
GdkPixbuf *turbolinux;
GdkPixbuf *slackware;
GdkPixbuf *suse;
GdkPixbuf *freebsd;
GdkPixbuf *gentoo;
GdkPixbuf *pld;

enum {
	BOGUS,
	FILL_GUI,
	FILL_XML,
	CLOSE,
	LAST_SIGNAL
};

enum {
	PLATFORM_LIST_COL_LOGO,
	PLATFORM_LIST_COL_NAME,
	PLATFORM_LIST_COL_PLATFORM,

	PLATFORM_LIST_COL_LAST
};

static GtkObjectClass *parent_class;
static gint gsttool_signals [LAST_SIGNAL] = { 0 };

static void     gst_tool_idle_run_directives_remove (GstTool *tool);
static void     gst_tool_idle_run_directives_add    (GstTool *tool);

static gboolean platform_set_current_cb   (GstTool *tool, GstReportLine *rline, gpointer data);
static gboolean platform_unsupported_cb   (GstTool *tool, GstReportLine *rline, gpointer data);
static gboolean report_finished_cb        (GstTool *tool, GstReportLine *rline, gpointer data);

static void report_dispatch (GstTool *tool);

static GstReportHookEntry common_report_hooks[] = {
/*        key                 function                    type                      allow_repeat user_data */
	{ "end",              report_finished_cb,         GST_REPORT_HOOK_LOADSAVE, TRUE,        NULL },
	{ "platform_success", platform_set_current_cb,    GST_REPORT_HOOK_SAVE,     TRUE,        NULL }, 
	{ "platform_unsup",   platform_unsupported_cb,    GST_REPORT_HOOK_LOADSAVE, TRUE,        NULL },
	{ "platform_undet",   platform_unsupported_cb,    GST_REPORT_HOOK_LOADSAVE, TRUE,        NULL },
	{ NULL,               NULL,                       -1,                       FALSE,       NULL }
};

static gchar *location_id = NULL;    /* Location in which we are editing */

/* --- Report hook and signal callbacks --- */

static void
on_platform_list_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GstPlatform *platform;
	gboolean selected;
	GstTool *tool = (GstTool *) data;
	
	if (tool->current_platform)
		gst_platform_free (tool->current_platform);
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PLATFORM_LIST_COL_PLATFORM, &platform, -1);
		tool->current_platform = gst_platform_dup (platform);

		selected = TRUE;
	} else {
		tool->current_platform = NULL;
		selected = FALSE;
	}

	gtk_widget_set_sensitive (tool->platform_ok_button, selected);
}

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

/* --- Other Report Hooks --- */

static gboolean
report_finished_cb (GstTool *tool, GstReportLine *rline, gpointer data)
{
	tool->report_dispatch_pending = FALSE;
	tool->report_finished = TRUE;

	gtk_widget_hide (tool->report_window);
	gtk_main_quit ();
	
	return TRUE;
}

/* --- GstTool --- */

void
gst_tool_add_supported_platform (GstTool *tool, GstPlatform *platform)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (platform != NULL);

	/* Avoid duplicates. Backend shouldn't serve duplicates, but
	 * we're paranoid here. */
	g_return_if_fail
		(g_slist_find_custom (tool->supported_platforms_list, platform,
		    (GCompareFunc) gst_platform_cmp) == NULL);

	tool->supported_platforms_list =
		g_slist_insert_sorted (tool->supported_platforms_list, platform,
		    (GCompareFunc) gst_platform_cmp);
}

void
gst_tool_clear_supported_platforms (GstTool *tool)
{
	GSList *list;
	GstPlatform *platform;

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
gst_tool_get_access (GstTool *tool)
{
	return tool->root_access != ROOT_ACCESS_NONE;
}

static void
report_clear_lines (GstTool *tool)
{
	GSList *list;
	GstReportLine *rline;

	for (list = tool->report_line_list; list; list = g_slist_next (list))
	{
		rline = (GstReportLine *) list->data;
		gst_report_line_free (rline);
	}

	if (tool->report_line_list)
	{
		g_slist_free (tool->report_line_list);
		tool->report_line_list = NULL;
	}
}

static void
report_dispatch (GstTool *tool)
{
	GSList *list;
	GstReportLine *rline;

	for (list = tool->report_line_list; list; list = g_slist_next (list))
	{
		rline = (GstReportLine *) list->data;

		if (gst_report_line_get_handled (rline))
			continue;

		if (strcmp (gst_report_line_get_key (rline), "progress"))
			gst_tool_invoke_report_hooks (tool, tool->report_hook_type, rline);
		
		gst_report_line_set_handled (rline, TRUE);
	}

	if (tool->report_finished && tool->input_id)
	{
		gdk_input_remove (tool->input_id);
		tool->input_id = 0;
	}
}

static void
report_progress_tick (gpointer data, gint fd, GdkInputCondition cond)
{
	GstTool *tool;
	GstReportLine *rline;
	gchar *buffer;
	gint i = 0;

	tool = GST_TOOL (data);
	if (tool->input_block)
		return;

	if (!tool->line)
		tool->line = g_string_new ("");

	while ((buffer = gst_tool_read_from_backend (tool))) {
		for (i = 0; (i < strlen (buffer)); i++) {
			gchar c = buffer [i];

			if (c == '\n') {
				/* End of line */

				/* Report line; add to list */
				rline = gst_report_line_new_from_string (tool->line->str);

				if (rline)
					tool->report_line_list = g_slist_append (
						tool->report_line_list, rline);

				g_string_assign (tool->line, "");

				if (!tool->report_dispatch_pending) {
					report_dispatch (tool);
					if (tool->report_finished) {
						i++;
						goto full_break;
					}
				}
			} else {
				/* Add character to end of current line */
				g_string_append_c (tool->line, buffer [i]);
			}
		}
	}

full_break:
	if (tool->report_finished) {
		if ((strlen (buffer) - i) > 0)
			tool->xml_document = g_string_new (&buffer [i]);
		
		g_string_free (tool->line, TRUE);
		tool->line = NULL;
	}
	
	if (!tool->report_dispatch_pending)
		report_dispatch (tool);
	
	if (tool->report_finished && tool->input_id)
	{
		gdk_input_remove (tool->input_id);
		tool->input_id = 0;
	}
}

static gboolean
report_window_close_cb (GtkWidget *window, GdkEventAny *ev, gpointer data)
{
	GstTool *tool = data;
	gtk_widget_hide (tool->report_window);
	return TRUE;
}

static void
report_progress (GstTool *tool, const gchar *label)
{
	tool->timeout_done = FALSE;
	tool->report_list_visible = FALSE;
	tool->report_finished = FALSE;
	tool->report_dispatch_pending = FALSE;

	gst_tool_clear_supported_platforms (tool);
	report_clear_lines (tool);

	if (label) {
		gtk_label_set_text (GTK_LABEL (tool->report_label), label);
		gtk_widget_show_all (tool->report_window);
		gtk_widget_show_now (tool->report_window);

		/* This ensures the report_window will be hidden on time.
		 * I'ts OK for long-lived directives, and short lived shouldn't
		 * be using the report window anyways. */
		while (gtk_events_pending ()) {
			usleep (1);
			gtk_main_iteration ();
		}
	}

	tool->input_id = gtk_input_add_full (tool->backend_master_fd, GDK_INPUT_READ,
					     report_progress_tick, NULL, tool, NULL);
	tool->input_block = FALSE;

	gtk_main ();

	if (tool->input_id)
		gtk_input_remove (tool->input_id);
}

static GSList *
gst_tool_get_supported_platforms (GstTool *tool)
{
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
}

static void
gst_tool_create_distro_images (void)
{
	debian = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/debian.png", NULL);
	redhat = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/redhat.png", NULL);
	mandrake = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/mandrake.png", NULL);
	turbolinux = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/turbolinux.png", NULL);
	slackware = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/slackware.png", NULL);
	suse = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/suse.png", NULL);
	freebsd = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/freebsd.png", NULL);
	gentoo = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/gentoo.png", NULL);
	pld = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/pld.png", NULL);
}

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
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tool->platform_list));

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

static void
gst_tool_run_platform_dialog (GstTool *tool)
{
	gint result;
	
	gst_tool_fill_platform_tree (tool);
	gtk_widget_set_sensitive (tool->platform_ok_button, FALSE);
	
	result = gtk_dialog_run (GTK_DIALOG (tool->platform_dialog));
	gtk_widget_hide (tool->platform_dialog);

	if (result != GTK_RESPONSE_OK)
		exit (0);
}

void
gst_tool_process_startup (GstTool *tool)
{
	/* let's process those startup reports. */
	tool->report_hook_type = GST_REPORT_HOOK_LOAD;

	report_progress (tool, NULL);

	/* stable version users should pass here only once,
	   but if there's an inconsistency the dialog will repeat */
	while (tool->run_again) {
		gst_tool_run_platform_dialog (tool);
		g_assert (tool->current_platform);

		if (tool->root_access == ROOT_ACCESS_SIMULATED)
			tool->root_access = ROOT_ACCESS_SIMULATED_DISABLED;

		gst_tool_run_set_directive (tool, NULL, NULL, "platform_set",
					    gst_platform_get_key (tool->current_platform), NULL);

		if (tool->root_access == ROOT_ACCESS_SIMULATED_DISABLED)
			tool->root_access = ROOT_ACCESS_SIMULATED;
	}
}

static gint
gst_tool_idle_run_directives (gpointer data)
{
	GstTool           *tool = data;
	GstDirectiveEntry *entry;
	
	while (tool->directive_queue)
	{
		entry = tool->directive_queue->data;
		tool->directive_queue = g_slist_remove (tool->directive_queue, entry);
		entry->callback (entry);
		g_free (entry);
	}

	tool->directive_queue_idle_id = 0;

	return FALSE;
}

static void
gst_tool_idle_run_directives_add (GstTool *tool)
{
	if (!tool->directive_queue_idle_id && !tool->directive_running)
		tool->directive_queue_idle_id = 
			gtk_idle_add_priority (G_PRIORITY_LOW, gst_tool_idle_run_directives, tool);
}

static void
gst_tool_idle_run_directives_remove (GstTool *tool)
{
	if (tool->directive_queue_idle_id) {
		gtk_idle_remove (tool->directive_queue_idle_id);
		tool->directive_queue_idle_id = 0;
	}
}

static void
gst_tool_kill_backend_cb (GstDirectiveEntry *entry)
{
	GstTool *tool = entry->tool;
	
	/* The backend was never called. No problem! */
	/* For further reference check http://www.xs4all.nl/~pjbrink/apekool/050/045-en.html */
	if (tool->backend_pid > 0) {
		if (tool->root_access == ROOT_ACCESS_SIMULATED)
			tool->root_access = ROOT_ACCESS_SIMULATED_DISABLED;

		gst_tool_run_set_directive (tool, NULL, NULL, "end", NULL);
		
		if (tool->root_access == ROOT_ACCESS_SIMULATED_DISABLED)
			tool->root_access = ROOT_ACCESS_SIMULATED;

		waitpid (tool->backend_pid, NULL, 0);
	}
}

static void
gst_tool_kill_tool_cb (GstDirectiveEntry *entry)
{
	GstTool *tool = entry->tool;

	gst_tool_kill_backend_cb (entry);
	
	g_signal_emit_by_name (G_OBJECT (tool), "destroy");
}

/* kills the whole tool */
static void
gst_tool_kill_tool (GstTool *tool, gpointer data)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));

	gst_tool_queue_directive (tool, gst_tool_kill_tool_cb, NULL, NULL, NULL, "end");
}

/* only kills the backend */
static void
gst_tool_kill_backend (GstTool *tool, gpointer data)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));

	gst_tool_queue_directive (tool, gst_tool_kill_backend_cb, NULL, NULL, NULL, "end");
}

static gboolean
gst_tool_end_of_request (GString *string)
{
	gchar *str, *eorpos;
	gint len, i;

	str = string->str;
	len = strlen (str);

	eorpos = g_strrstr (str, GST_TOOL_EOR);

	if (eorpos == NULL)
		return FALSE;

	g_string_truncate (string, strlen (str) - strlen (eorpos));

	return TRUE;
}

static xmlDoc *
gst_tool_read_xml_from_backend (GstTool *tool)
{
	gchar *buffer;
	int t;
	xmlDoc *xml;

	if (!tool->xml_document)
		return NULL;

	while (! gst_tool_end_of_request (tool->xml_document)) {
		buffer = gst_tool_read_from_backend (tool);
		g_string_append (tool->xml_document, buffer);
	}

	if (tool->xml_document->str[0] == '<') {
		xml = xmlParseDoc (tool->xml_document->str);
	} else {
		xml = NULL;
	}

	g_string_free (tool->xml_document, TRUE);
	tool->xml_document = NULL;

	return xml;
}

static void
gst_tool_default_set_directive_callback (GstDirectiveEntry *entry)
{
	gst_tool_run_set_directive (entry->tool, entry->in_xml, entry->report_sign,
				    entry->directive, NULL);
}

/* All parameters except directive can be NULL.
   NULL report_sign makes no report window to be shown when directive is run.
   NULL callback runs the default directive callback, which runs a set directive.
   data is for closure. in_xml is an input xml that may be required by the directive. */
void
gst_tool_queue_directive (GstTool *tool, GstDirectiveFunc callback, gpointer data,
			  xmlDoc *in_xml, gchar *report_sign, gchar *directive)
{
	GstDirectiveEntry *entry;

	g_return_if_fail (tool != NULL);
	g_return_if_fail (GST_IS_TOOL (tool));

	entry = g_new0 (GstDirectiveEntry, 1);
	g_return_if_fail (entry != NULL);
	
	entry->tool        = tool;
	entry->callback    = callback? callback: gst_tool_default_set_directive_callback;
	entry->data        = data;
	entry->in_xml      = in_xml;
	entry->report_sign = report_sign;
	entry->directive   = directive;

	tool->directive_queue = g_slist_append (tool->directive_queue, entry);
	gst_tool_idle_run_directives_add (tool);
}

/* As specified, escapes :: to \:: and \ to \\ and joins the strings. */
static GString *
gst_tool_join_directive (const gchar *directive, va_list ap)
{
	GString *str;
	gchar   *arg, *s;
	
	str = g_string_new (directive);
	while ((arg = va_arg (ap, char *)) != NULL) {
		g_string_append (str, "::");
		for (s = arg; *s; s++) {
			switch (*s) {
			case '\\':
				g_string_append_c (str, '\\');
				break;
			case ':':
				if (*(s + 1) == ':')
					g_string_append_c (str, '\\');
			}
			g_string_append_c (str, *s);
		}
	}
	va_end (ap);

	return str;
}

static void
gst_tool_send_directive (GstTool *tool, const gchar *directive, va_list ap)
{
	GString *directive_line;
	FILE *f;
	gchar *buffer;

	g_return_if_fail (tool->backend_pid >= 0);

       	directive_line = gst_tool_join_directive (directive, ap);
	g_string_append_c (directive_line, '\n');

	gst_tool_write_to_backend (tool, directive_line->str);

	g_string_free (directive_line, TRUE);
}

static xmlDoc *
gst_tool_run_get_directive_va (GstTool *tool, const gchar *report_sign, const gchar *directive, va_list ap)
{
	xmlDoc *xml;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (GST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->directive_running == FALSE, NULL);

	tool->directive_running = TRUE;
	gst_tool_idle_run_directives_remove (tool);

	gst_tool_send_directive (tool, directive, ap);

	/* FIXME: Instead of doing the following, we should pass around a value describing
	 * tool I/O mode (as opposed to a string to report_progress ()). */
	tool->report_hook_type = GST_REPORT_HOOK_LOAD;

	xmlSubstituteEntitiesDefault (TRUE);

	/* LibXML support for parsing from memory is good, but parsing from
	 * opened filehandles is not supported unless you write your own feed
	 * mechanism. Let's just load it all into memory, then. Also, refusing
	 * enormous documents can be considered a plus. </dystopic> */
	
	if (tool->xml_document == NULL)
		tool->xml_document = g_string_new ("");

	if (location_id == NULL)
		report_progress (tool, report_sign? _(report_sign): NULL);

	xml = gst_tool_read_xml_from_backend(tool);

	tool->directive_running = FALSE;
	gst_tool_idle_run_directives_add (tool);
	
	return xml;
}

xmlDoc *
gst_tool_run_get_directive (GstTool *tool, const gchar *report_sign, const gchar *directive, ...)
{
	va_list ap;
	
	va_start (ap, directive);
	return gst_tool_run_get_directive_va (tool, report_sign, directive, ap);
}

static xmlDoc *
gst_tool_run_set_directive_va (GstTool *tool, xmlDoc *xml,
			       const gchar *report_sign, const gchar *directive, va_list ap)
{
	FILE *f;
	xmlDoc *xml_out;

	int n;
	gchar buf;

	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (GST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->directive_running == FALSE, NULL);

	tool->directive_running = TRUE;
	gst_tool_idle_run_directives_remove (tool);

	/* don't actually run if we are just pretending */
	if (tool->root_access == ROOT_ACCESS_SIMULATED) {
		g_warning (_("Skipping %s directive..."), directive);
		tool->directive_running = FALSE;
		return NULL;
	}

	gst_tool_send_directive (tool, directive, ap);

	if (xml) {
		gst_tool_write_xml_to_backend (tool, xml);
		gst_tool_write_to_backend (tool, "\n");
		gst_tool_write_to_backend (tool, GST_TOOL_EOR);
		gst_tool_write_to_backend (tool, "\n");
	}


	/* FIXME: Instead of doing the following, we should pass around a value describing
	 * tool I/O mode (as opposed to a string to report_progress ()). */
	tool->report_hook_type = GST_REPORT_HOOK_SAVE;

	if (location_id == NULL)
		report_progress (tool, report_sign? _(report_sign): NULL);


	/* This is tipicaly to just read the end of request string,
	   but a set directive may return some XML too. */
	xml_out = gst_tool_read_xml_from_backend (tool);

	tool->directive_running = FALSE;
	gst_tool_idle_run_directives_add (tool);

	return xml_out;
}

xmlDoc *
gst_tool_run_set_directive (GstTool *tool, xmlDoc *xml,
			    const gchar *report_sign, const gchar *directive, ...)
{
	va_list ap;
	
	va_start (ap, directive);
	return gst_tool_run_set_directive_va (tool, xml, report_sign, directive, ap);
}

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
#ifndef GST_HAVE_ARCHIVER
		g_assert_not_reached ();
#else	
		CORBA_Environment ev;
		ConfigArchiver_Archive archive;
		ConfigArchiver_Location location;
		gchar *backend_id;

		CORBA_exception_init (&ev);

		archive = bonobo_get_object ("archive:global-archive", "IDL:ConfigArchiver/Archive:1.0", &ev);

		if (BONOBO_EX (&ev) || archive == CORBA_OBJECT_NIL) {
			g_critical ("Could not resolve the archive moniker");
			return FALSE;
		}

		location = ConfigArchiver_Archive_getLocation (archive, location_id, &ev);

		if (BONOBO_EX (&ev) || location == CORBA_OBJECT_NIL) {
			g_critical ("Could not get the location %s", location_id);
			return FALSE;
		}

		backend_id = strrchr (tool->script_path, '/');

		if (backend_id != NULL)
			backend_id++;
		else
			backend_id = tool->script_path;

		tool->config = location_client_load_rollback_data (location, NULL, 0, backend_id, TRUE, &ev);

		bonobo_object_release_unref (location, NULL);
		bonobo_object_release_unref (archive, NULL);

		CORBA_exception_free (&ev);
#endif	
	}


	if (tool->config){
		g_signal_emit (GTK_OBJECT (tool), gsttool_signals[FILL_GUI], 0);
	}
	return tool->config != NULL;
}

gboolean
gst_tool_save (GstTool *tool, gboolean restore)
{
#ifdef GST_HAVE_ARCHIVER
	CORBA_Environment ev;
	ConfigArchiver_Archive archive;
	ConfigArchiver_Location location;
	gchar *backend_id;
#endif	

	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (GST_IS_TOOL (tool), FALSE);
	g_return_val_if_fail (tool->root_access != ROOT_ACCESS_NONE, FALSE);

	gst_dialog_freeze_visible (tool->main_dialog);

	g_signal_emit (GTK_OBJECT (tool), gsttool_signals[FILL_XML], 0);
#ifdef GST_DEBUG
	/* don't actually save if we are just pretending */
	if (tool->root_access == ROOT_ACCESS_SIMULATED) {
		g_warning (_("Skipping actual save..."));
		return TRUE;
	}
#endif

#ifdef GST_HAVE_ARCHIVER
	CORBA_exception_init (&ev);

	/* Archive data with the archiver */
	archive = bonobo_get_object ("archive:global-archive", "IDL:ConfigArchiver/Archive:1.0", &ev);

	if (BONOBO_EX (&ev) || archive == CORBA_OBJECT_NIL) {
		g_critical ("Could not resolve the archive moniker");
	} else {
		if (location_id == NULL)
			location = ConfigArchiver_Archive__get_currentLocation (archive, &ev);
		else
			location = ConfigArchiver_Archive_getLocation (archive, location_id, &ev);

		if (BONOBO_EX (&ev) || location == CORBA_OBJECT_NIL) {
			g_critical ("Could not get location %s", location_id);
		} else {
			backend_id = strrchr (tool->script_path, '/');

			if (backend_id != NULL)
				backend_id++;
			else
				backend_id = tool->script_path;

			location_client_store_xml (location, backend_id, tool->config, ConfigArchiver_STORE_MASK_PREVIOUS, &ev);

			bonobo_object_release_unref (location, &ev);
		}

		bonobo_object_release_unref (archive, &ev);
	}

	CORBA_exception_free (&ev);

#endif
	
	if (location_id == NULL) {
		if (restore)
			gst_tool_run_set_directive (tool, tool->original_config, NULL, "set", NULL);
		else
			gst_tool_run_set_directive (tool, tool->config, NULL, "set", NULL);
	}
	
	gst_dialog_thaw_visible (tool->main_dialog);

	return TRUE;  /* FIXME: Determine if it really worked. */
}

void
gst_tool_save_cb (GtkWidget *w, GstTool *tool)
{
	gst_tool_save (tool, FALSE);
}

void
gst_tool_restore_cb (GtkWidget *w, GstTool *tool)
{
	gst_tool_save (tool, TRUE);
}

void
gst_tool_load_try (GstTool *tool)
{
	if (!gst_tool_load (tool)) {
		GtkWidget *d;

		d = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_CLOSE,
					    _("There was an error running the backend script,\n"
					      "and the configuration could not be loaded."));

		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);
		
		exit (0);
	}
}

static void
tool_main_do (GstTool *tool, gboolean no_main_loop, gboolean show_main_dialog)
{
	gst_dialog_freeze_visible (tool->main_dialog);

	if (show_main_dialog)
		gtk_widget_show (GTK_WIDGET (tool->main_dialog));
	
	if (tool->remote_config == TRUE) {
		/* run the ssh client */
		gst_tool_fill_remote_hosts_list (tool, tool->remote_hosts);
		gtk_widget_show (tool->remote_dialog);
	} else {
		/* run the su command */
		gst_dialog_freeze_visible (tool->main_dialog);

		gst_auth_do_su_authentication (tool);

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
gst_tool_destroy (GtkObject *object)
{
	GstTool *tool;

	tool = GST_TOOL (object);
	
	parent_class->destroy (object);
	gtk_main_quit ();
}

static void
gst_tool_class_init (GstToolClass *klass)
{	
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;

	parent_class = g_type_class_peek_parent (klass);

	gsttool_signals[FILL_GUI] = 
		g_signal_new ("fill_gui",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstToolClass, fill_gui),
			      NULL, NULL,
			      gst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	gsttool_signals[FILL_XML] = 
		g_signal_new ("fill_xml",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstToolClass, fill_xml),
			      NULL, NULL,
			      gst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	gsttool_signals[CLOSE] = 
		g_signal_new ("close",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GstToolClass, close),
			      NULL, NULL,
			      gst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	object_class->destroy = gst_tool_destroy;
}

static void
gst_tool_create_platform_list (GtkTreeView *list, GstTool *tool)
{
	GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_store_new (PLATFORM_LIST_COL_LAST, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER));
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	g_return_if_fail (list != NULL);
	g_return_if_fail (GTK_IS_TREE_VIEW (list));

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
	g_object_unref (model);
	
	/* Insert the pixmaps cell */
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list),
	                                             -1,
	                                             "_Platform",
	                                             renderer,
	                                             "pixbuf", PLATFORM_LIST_COL_LOGO,
						     NULL);

	/* Insert the text cell */
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (list), PLATFORM_LIST_COL_LOGO);
	
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);

	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text", PLATFORM_LIST_COL_NAME);
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (on_platform_list_selection_changed),
			  (gpointer) tool);
}

static void
on_remote_hosts_list_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GstTool *tool = GST_TOOL (data);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *host;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		/* freeze the dialog */
		gtk_widget_set_sensitive (tool->remote_dialog, FALSE);
		
		gtk_tree_model_get (model, &iter, 0, &host, -1);

		if (tool->backend_pid > 0) {
			gst_dialog_ask_apply (tool->main_dialog);
			gst_dialog_set_modified (tool->main_dialog, FALSE);

			/* removes the g_timeout that's waiting for the backend to die */
			g_source_remove (tool->timeout_id);

			/* stops the backend */
			gst_tool_kill_backend (tool, NULL);

			/* process the "end" directive */
			while (gtk_events_pending())
				gtk_main_iteration();

			/* remove current platform info, in the new host may be another one */
			if (tool->current_platform != NULL) {
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
		gst_dialog_thaw_visible (tool->main_dialog);
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
	GladeXML *xml;

	tool->glade_common_path  = g_strdup_printf ("%s/common.glade", INTERFACES_DIR);

	tool->etspecs_common_path = g_strdup (ETSPECS_DIR);

	tool->report_gui = xml  = gst_tool_load_glade_common (tool, "report_window");

	tool->report_window     = glade_xml_get_widget (xml, "report_window");
	g_signal_connect (GTK_OBJECT (tool->report_window), "delete_event",
			    G_CALLBACK (report_window_close_cb), tool);

	tool->report_label      = glade_xml_get_widget (xml, "report_label");

	xml = gst_tool_load_glade_common (tool, "platform_dialog");
	tool->platform_list = glade_xml_get_widget (xml, "platform_list");
	gst_tool_create_platform_list (GTK_TREE_VIEW (tool->platform_list), tool);

	tool->platform_dialog    = glade_xml_get_widget (xml, "platform_dialog");
	tool->platform_ok_button = glade_xml_get_widget (xml, "platform_ok_button");

	xml = gst_tool_load_glade_common (tool, "remote_dialog");
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
			NULL, /* base finalize */
			(GClassInitFunc) gst_tool_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (GstTool),
			0, /* n_preallocs */
			(GInstanceInitFunc) gst_tool_type_init
		};
		
		gsttool_type = g_type_register_static (GTK_TYPE_OBJECT, "GstTool", &gsttool_info, 0);
	}

	return gsttool_type;
}

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
void
gst_tool_construct (GstTool *tool, const char *name, const char *title)
{
	char *s, *t, *u;
	GConfClient *client = gconf_client_get_default ();
	GError *error;

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
	t = g_strdup_printf (_("%s - GNOME System Tools"), title);
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
	g_signal_connect (G_OBJECT (tool->main_dialog),
			  "restore",
			  G_CALLBACK (gst_tool_restore_cb),
			  tool);

	tool->report_hook_list = NULL;
	memset (&tool->report_hook_defaults, 0, sizeof (GstReportHook *) * GST_MAJOR_MAX);
	gst_tool_add_report_hooks (tool, common_report_hooks);

	tool->backend_pid = -1;
	tool->backend_master_fd = -1;
	gst_tool_set_close_func (tool, gst_tool_kill_tool, NULL);

	tool->directive_running = FALSE;
	tool->directive_queue   = NULL;
	tool->directive_queue_idle_id = 0;

	/* get the remote configuration gconf keys */
	remote_config_key = g_strjoin ("/", GST_GCONF_ROOT, "global", "remote-configuration", NULL);
	remote_hosts_key = g_strjoin ("/", GST_GCONF_ROOT, "global", "remote-hosts-list", NULL);

	do_remote_conf = gconf_client_get_bool (client, remote_config_key, &error);
	hosts = gconf_client_get_string (client, remote_hosts_key, &error);

	tool->remote_config = ((do_remote_conf) && (hosts != NULL) && (strcmp (SSH_PATH, "") != 0));
	if (hosts != NULL) 
		tool->remote_hosts = g_strsplit (hosts, ",", -1);
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
		g_signal_connect (GTK_OBJECT (tool), "close", G_CALLBACK (close_cb), data);

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
	gchar *warning = g_strdup_printf(_("Welcome to the %s prerelease of the "
		  "GNOME System Tools.\n\n"
		  "This is still a work in progress, and so it may have serious bugs.\n"
		  "Due to the nature of these tools, bugs may render your computer\n"
		  "PRACTICALLY USELESS, costing time, effort and sanity points.\n\n"
		  "You have been warned. Thank you for trying out this prerelease of\n"
		  "the GNOME System Tools!\n\n"
		  "--\nThe GNOME System Tools team"), VERSION);

	client = gconf_client_get_default ();

	version_key = g_strjoin ("/", GST_GCONF_ROOT, "global", "last-version", NULL);
	key = g_strjoin ("/", GST_GCONF_ROOT, "global", "show_warning", NULL);


	value = gconf_client_get_bool (client, key, &error);
	version = gconf_client_get_string (client, version_key, &error);

	if ((value == TRUE) || version && (strcmp (version, VERSION) != 0))
	{
		GtkWidget *dialog, *label, *image, *hbox;
		GtkWidget *checkbox;
		gboolean dont_ask_again;
		
		dialog = gtk_dialog_new_with_buttons (_("Warning"),
						      NULL,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_OK, GTK_RESPONSE_OK,
						      NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);		
		
		label = gtk_label_new (warning);
		image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		checkbox = gtk_check_button_new_with_label (_("Don't show me this again"));

		hbox = gtk_hbox_new (FALSE, 5);

		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 5);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

		gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 150);
		
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 5);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), checkbox, FALSE, FALSE, 5);
		
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

	g_free (warning);
	g_free (key);
}

void
gst_init (const gchar *app_name, int argc, char *argv [], const poptOption options)
{
	GnomeProgram *program;
	struct poptOption gst_options[] =
	{
		{NULL, '\0', 0, NULL, 0}
	};

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#warning FIXME	
#if 0	
        gnomelib_register_popt_table (gst_options, "general GST options");

	if (options == NULL) {
		gnome_init (app_name, VERSION, argc, argv);
	} else {
		poptContext ctx;
		GList *args_list = NULL;
		char **args;
		gint i;
		
		gnome_init_with_popt_table (app_name, VERSION, argc, argv, options, 0, &ctx);

		args = (char**) poptGetArgs(ctx);
	
		for (i = 0; args && args[i]; i++) {
			args_list = g_list_append (args_list, args[i]);
		}
		poptFreeContext (ctx);
	}	
#endif
 	program = gnome_program_init (app_name, VERSION,
				      LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_HUMAN_READABLE_NAME,
				      _("GNOME System Tools"),
				      NULL);
	try_show_usage_warning ();
}

gchar*
gst_tool_read_from_backend (GstTool *tool)
{
	gchar *buffer;
	GString *str = g_string_new ("");
	gchar *ptr;
	gboolean may_exit = FALSE;
	
	buffer = (gchar*) g_malloc0 (sizeof (gchar) * BUFFER_SIZE);

	do {
		usleep (32000);
		
		while (fgets (buffer, BUFFER_SIZE, tool->backend_stream) != NULL) {
			g_string_append (str, buffer);
			may_exit = TRUE;
		}
		
		while (gtk_events_pending ())
			gtk_main_iteration ();
	} while (!may_exit);

	ptr = str->str;
	g_string_free (str, FALSE);

	return ptr;
}

void
gst_tool_write_xml_to_backend (GstTool *tool, xmlDoc *doc)
{
	gint size;
	xmlChar *xml;
	gchar *string;

	xmlDocDumpMemory (doc, &xml, &size);
	string = (gchar *) xml;
	
	gst_tool_write_to_backend (tool, string);

	xmlFree (xml);

	usleep (32000);
}

void
gst_tool_write_to_backend (GstTool *tool, gchar *string)
{
	gint ntotal = 0;
	gint nread = 0;
	gint ret;
	gchar *p;

	do {
		ret = fputc (string [nread], tool->backend_stream);

		if (ret >= 0)
			nread++;
	} while (nread < strlen (string));

	while (fflush (tool->backend_stream) != 0);
}
