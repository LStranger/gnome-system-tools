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
 */

#include <config.h>

#include "xst-tool.h"
#include "xst-dialog.h"
#include "xst-report-line.h"
#include "xst-report-hook.h"
#include "xst-platform.h"
#include "xst-ui.h"
#include "xst-su.h"
#include "xst-xml.h"
#include "xst-marshal.h"

#ifdef XST_HAVE_ARCHIVER
#  include <bonobo.h>
#  include <config-archiver/archiver-client.h>
#endif

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define XST_DEBUG 1
/* Define this if you want the frontend to run the backend under strace */
/*#define XST_DEBUG_STRACE_BACKEND*/

enum {
	BOGUS,
	FILL_GUI,
	FILL_XML,
	CLOSE,
	LAST_SIGNAL
};

enum {
	PLATFORM_LIST_COL_NAME,
	PLATFORM_LIST_COL_PLATFORM,

	PLATFORM_LIST_COL_LAST
};

static enum {
	ROOT_ACCESS_NONE,
	ROOT_ACCESS_SIMULATED,
	ROOT_ACCESS_SIMULATED_DISABLED,
	ROOT_ACCESS_REAL
} root_access = ROOT_ACCESS_NONE;

static GtkObjectClass *parent_class;
static gint xsttool_signals [LAST_SIGNAL] = { 0 };
static const gchar *XST_TOOL_EOR = "\n<!-- XST: end of request -->\n";

static void     xst_tool_idle_run_directives_remove (XstTool *tool);
static void     xst_tool_idle_run_directives_add    (XstTool *tool);

static gboolean platform_set_current_cb   (XstTool *tool, XstReportLine *rline, gpointer data);
static gboolean platform_unsupported_cb   (XstTool *tool, XstReportLine *rline, gpointer data);
static gboolean report_finished_cb        (XstTool *tool, XstReportLine *rline, gpointer data);

static void report_dispatch (XstTool *tool);

static XstReportHookEntry common_report_hooks[] = {
/*        key                 function                    type                      allow_repeat user_data */
	{ "end",              report_finished_cb,         XST_REPORT_HOOK_LOADSAVE, TRUE,        NULL },
	{ "platform_success", platform_set_current_cb,    XST_REPORT_HOOK_SAVE,     FALSE,       NULL }, 
	{ "platform_unsup",   platform_unsupported_cb,    XST_REPORT_HOOK_LOADSAVE, FALSE,       NULL },
	{ "platform_undet",   platform_unsupported_cb,    XST_REPORT_HOOK_LOADSAVE, FALSE,       NULL },
	{ NULL,               NULL,                       -1,                       FALSE,       NULL }
};

static gchar *location_id = NULL;    /* Location in which we are editing */

/* --- Report hook and signal callbacks --- */

static void
platform_list_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	XstPlatform *platform;
	gboolean selected;
	XstTool *tool;

	tool = (XstTool *) data;

	if (tool->current_platform)
			xst_platform_free (tool->current_platform);

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PLATFORM_LIST_COL_PLATFORM, &platform, -1);
		tool->current_platform = xst_platform_dup (platform);
		selected = TRUE;
	} else {
		tool->current_platform = NULL;
		selected = FALSE;
	}

	gtk_widget_set_sensitive (tool->platform_ok_button, selected);
}

static gboolean
platform_set_current_cb (XstTool *tool, XstReportLine *rline, gpointer data)
{
	XstPlatform *platform;

	platform = xst_platform_new_from_report_line (rline);
	g_return_val_if_fail (platform != NULL, TRUE);

	if (tool->current_platform)
		xst_platform_free (tool->current_platform);

	tool->current_platform = platform;
	tool->run_again = FALSE;
	return TRUE;
}

static gboolean
platform_unsupported_cb (XstTool *tool, XstReportLine *rline, gpointer data)
{
	tool->run_again = TRUE;

	return TRUE;
}

/* --- Other Report Hooks --- */

static gboolean
report_finished_cb (XstTool *tool, XstReportLine *rline, gpointer data)
{
	tool->report_dispatch_pending = FALSE;
	tool->report_finished = TRUE;

	gtk_widget_hide (tool->report_window);
	gtk_main_quit ();
	
	return TRUE;
}

/* --- XstTool --- */

void
xst_tool_add_supported_platform (XstTool *tool, XstPlatform *platform)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (platform != NULL);

	/* Avoid duplicates. Backend shouldn't serve duplicates, but
	 * we're paranoid here. */
	g_return_if_fail
		(g_slist_find_custom (tool->supported_platforms_list, platform,
		    (GCompareFunc) xst_platform_cmp) == NULL);

	tool->supported_platforms_list =
		g_slist_insert_sorted (tool->supported_platforms_list, platform,
		    (GCompareFunc) xst_platform_cmp);
}

void
xst_tool_clear_supported_platforms (XstTool *tool)
{
	GSList *list;
	XstPlatform *platform;

	g_return_if_fail (tool != NULL);

	for (list = tool->supported_platforms_list; list;
	     list = g_slist_next (list))
	{
		platform = (XstPlatform *) list->data;
		xst_platform_free (platform);
	}

	if (tool->supported_platforms_list)
	{
		g_slist_free (tool->supported_platforms_list);
		tool->supported_platforms_list = NULL;
	}
}

GladeXML *
xst_tool_load_glade_common (XstTool *tool, const gchar *widget)
{
	GladeXML *xml;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->glade_common_path != NULL, NULL);

	xml = glade_xml_new (tool->glade_common_path, widget, NULL);

	if (!xml) {
		g_error ("Could not load %s\n", tool->glade_common_path);
	}

	return xml;
}

GladeXML *
xst_tool_load_glade (XstTool *tool, const gchar *widget)
{
	GladeXML *xml;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->glade_path != NULL, NULL);

	xml = glade_xml_new (tool->glade_path, widget, NULL);

	if (!xml) {
		g_error ("Could not load %s\n", tool->glade_path);
	}
	
	return xml;
}

XstDialog *
xst_tool_get_dialog (XstTool *tool)
{
	return tool->main_dialog;
}

gboolean
xst_tool_get_access (XstTool *tool)
{
	return root_access != ROOT_ACCESS_NONE;
}

#if 0
static void
timeout_cb (gpointer data)
{
	XstTool *tool;

	tool = XST_TOOL (data);

	tool->timeout_done = TRUE;

	if (tool->timeout_done && !tool->report_list_visible)
		gtk_main_quit ();
}

static void
set_arrow (XstTool *tool, GtkArrowType arrow)
{
	gtk_notebook_set_page (GTK_NOTEBOOK (tool->report_notebook),
			       arrow == GTK_ARROW_UP ? 0 : 1);
}

#endif

static void
report_clear_lines (XstTool *tool)
{
	GSList *list;
	XstReportLine *rline;

	for (list = tool->report_line_list; list; list = g_slist_next (list))
	{
		rline = (XstReportLine *) list->data;
		xst_report_line_free (rline);
	}

	if (tool->report_line_list)
	{
		g_slist_free (tool->report_line_list);
		tool->report_line_list = NULL;
	}
}

static void
report_dispatch (XstTool *tool)
{
	GSList *list;
	XstReportLine *rline;
#if 0
	GtkAdjustment *vadj;
	char *report_text[] = {
		NULL,
		NULL
	};

	if (!GTK_IS_SCROLLED_WINDOW (tool->report_scrolled)) {
		g_warning ("tool->report_scrolled is not a GtkScrolledWindow\n");
		tool->report_dispatch_pending = FALSE;
		tool->report_finished = TRUE;
		return;
	}
	
	vadj = gtk_scrolled_window_get_vadjustment (
		GTK_SCROLLED_WINDOW (tool->report_scrolled));
#endif

	for (list = tool->report_line_list; list; list = g_slist_next (list))
	{
		rline = (XstReportLine *) list->data;

		if (xst_report_line_get_handled (rline))
			continue;

#if 0
		if (!strcmp (xst_report_line_get_key (rline), "progress")) {
			/* Progress update */

			gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress),
						     atoi (xst_report_line_get_message (rline)) / 100.0);
		} else {
			gboolean scroll;
			
			/* Report line */
			
			gtk_entry_set_text (GTK_ENTRY (tool->report_entry),
					    xst_report_line_get_message (rline));
			
			if (vadj->value >= vadj->upper - vadj->page_size)
				scroll = TRUE;
			else
				scroll = FALSE;

			report_text [1] = (char *) xst_report_line_get_message (rline);

			gtk_clist_append (GTK_CLIST (tool->report_list), report_text);

			if (scroll)
				gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);

			xst_tool_invoke_report_hooks (tool, tool->report_hook_type, rline);
		}
#endif

		if (strcmp (xst_report_line_get_key (rline), "progress"))
			xst_tool_invoke_report_hooks (tool, tool->report_hook_type, rline);
		
		xst_report_line_set_handled (rline, TRUE);
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
	XstTool *tool;
	XstReportLine *rline;
	gchar buffer [512];
	gint n, i = 0;

	tool = XST_TOOL (data);
	if (tool->input_block)
		return;

	if (!tool->line)
		tool->line = g_string_new ("");

	while ((n = read (fd, buffer, sizeof (buffer) - 1)) != 0) {
		buffer [n] = 0;
		
		for (i = 0; (i < n); i++) {
			gchar c = buffer [i];
			
			if (c == '\n') {
				/* End of line */
				
				/* Report line; add to list */
				rline = xst_report_line_new_from_string (tool->line->str);
				
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

	if (n <= 0) {
		/* Zero-length read; pipe closed unexpectedly */

		tool->report_finished = TRUE;
	}

	if (tool->report_finished) {
		if (n > 0)
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
	XstTool *tool = data;

	/* A hack to force the window to hide. Seems like the widget is
	 * marked as hidden, while X window doesn't actually do it, so a
	 * widget_show should set things normally. */
	gtk_widget_show (tool->report_window);
	gtk_widget_hide (tool->report_window);
	return TRUE;
}

static void
report_progress (XstTool *tool, const gchar *label)
{
#if 0
	gint cb_id;
#endif	
	tool->timeout_done = FALSE;
	tool->report_list_visible = FALSE;
	tool->report_finished = FALSE;
	tool->report_dispatch_pending = FALSE;

	xst_tool_clear_supported_platforms (tool);
	report_clear_lines (tool);

#if 0
	set_arrow (tool, GTK_ARROW_DOWN);
	gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress), 0.0);
#endif

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
		sleep (1);
	}

	tool->input_id = gtk_input_add_full (tool->backend_read_fd, GDK_INPUT_READ,
					     report_progress_tick, NULL, tool, NULL);
	tool->input_block = FALSE;

	gtk_main ();

	if (tool->input_id)
		gtk_input_remove (tool->input_id);

#if 0
	if (!tool->run_again)
	{
		/* Set progress to 100% and wait a bit before closing display */
		gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress), 1.0);

		cb_id = gtk_timeout_add (1500, (GtkFunction) timeout_cb, tool);
		gtk_main ();
		gtk_timeout_remove (cb_id);
	}

	gtk_clist_clear (GTK_CLIST (tool->report_list));
#endif
}

static GSList *
xst_tool_get_supported_platforms (XstTool *tool)
{
	xmlDoc *doc;
	xmlNode *root, *node;
	GSList *list;
	XstPlatform *plat;
	
	doc = xst_tool_run_get_directive (tool, NULL, "platforms", NULL);
	list = NULL;

	g_return_val_if_fail (doc != NULL, NULL);
	root = xst_xml_doc_get_root (doc);
	for (node = xst_xml_element_find_first (root, "platform"); node;
	     node = xst_xml_element_find_next (node, "platform")) {
		plat = xst_platform_new_from_node (node);
		if (!plat)
			continue;
		list = g_slist_append (list, plat);
	}

	return list;
}

static void
xst_tool_run_platform_dialog (XstTool *tool)
{
	GSList *list;
	gint result;
	XstPlatform *platform;
	char *platform_text;
	GtkTreeView *tree;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkTreeIter iter;

	/* Fill in the platform list */

	tree = GTK_TREE_VIEW (tool->platform_list);
	store = gtk_list_store_new (PLATFORM_LIST_COL_LAST, G_TYPE_STRING, G_TYPE_POINTER);

	tool->supported_platforms_list = xst_tool_get_supported_platforms (tool);

	for (list = tool->supported_platforms_list; list;
	     list = g_slist_next (list))
	{
		platform = (XstPlatform *) list->data;
		platform_text = (char *) xst_platform_get_name (platform);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    PLATFORM_LIST_COL_NAME, platform_text,
				    PLATFORM_LIST_COL_PLATFORM, platform,
				    -1);
	}

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Platform"),
							   renderer,
							   "text",
							   PLATFORM_LIST_COL_NAME,
							   NULL);
	gtk_tree_view_append_column (tree, column);

	gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
	select = gtk_tree_view_get_selection (tree);
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (platform_list_selection_changed),
			  (gpointer) tool);

	gtk_widget_set_sensitive (tool->platform_ok_button, FALSE);

	result = gtk_dialog_run (GTK_DIALOG (tool->platform_dialog));
	if (result != GTK_RESPONSE_OK)
		exit (0);

	gtk_widget_destroy (tool->platform_dialog);
}

static void
xst_tool_process_startup (XstTool *tool)
{
	/* let's process those startup reports. */
	tool->report_hook_type = XST_REPORT_HOOK_LOAD;
	report_progress (tool, NULL);

	/* stable version users should pass here only once,
	   but if there's an inconsistency the dialog will repeat */
	while (tool->run_again) {
		xst_tool_run_platform_dialog (tool);
		g_assert (tool->current_platform);
		
		if (root_access == ROOT_ACCESS_SIMULATED)
			root_access = ROOT_ACCESS_SIMULATED_DISABLED;
		
		xst_tool_run_set_directive (tool, NULL, NULL, "platform_set",
					    xst_platform_get_key (tool->current_platform), NULL);

		if (root_access == ROOT_ACCESS_SIMULATED_DISABLED)
			root_access = ROOT_ACCESS_SIMULATED;
	}
}

static void
xst_tool_init_backend (XstTool *tool)
{
	int fd_read [2], fd_write [2];

	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (tool->script_path != NULL);
	g_return_if_fail (tool->backend_pid < 0);
	
	pipe (fd_read);
	pipe (fd_write);

	tool->backend_pid = fork ();
	if (tool->backend_pid < 0) {
		g_warning ("Unable to fork.");
		tool->backend_pid = -1;
		return;
	} else if (tool->backend_pid) {
		/* Parent */

		close (fd_read  [1]); /* Close writing end of read pipe */
		close (fd_write [0]); /* Close reading end of write pipe */

		tool->backend_read_fd  = fd_read  [0];
		tool->backend_write_fd = fd_write [1];

		fcntl (tool->backend_read_fd, F_SETFL, 0);  /* Let's block */

		xst_tool_process_startup (tool);
	} else {
		/* Child */

		/* For the child, things are upside-down */
		close (fd_read  [0]); /* Close reading end of read pipe */
		close (fd_write [1]); /* Close writing end of write pipe */

		dup2 (fd_read  [1], STDOUT_FILENO);
		dup2 (fd_write [0],  STDIN_FILENO);

		if (tool->current_platform)
			execl (tool->script_path, tool->script_path,
			       "--progress", "--report", "--platform",
			       xst_platform_get_key (tool->current_platform), NULL);
		else
#ifndef XST_DEBUG_STRACE_BACKEND
			execl (tool->script_path, tool->script_path, "--progress",
			       "--report", NULL);
#else		
		execl ("/usr/bin/strace", "/usr/bin/strace", "-f", tool->script_path, "--progress",
		       "--report", NULL);
#endif					
		
		g_error ("Unable to run backend: %s", tool->script_path);
	}
}

static gint
xst_tool_idle_run_directives (gpointer data)
{
	XstTool           *tool = data;
	XstDirectiveEntry *entry;
	
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
xst_tool_idle_run_directives_add (XstTool *tool)
{
	if (!tool->directive_queue_idle_id && !tool->directive_running)
		tool->directive_queue_idle_id = 
			gtk_idle_add_priority (GTK_PRIORITY_LOW, xst_tool_idle_run_directives, tool);
}

static void
xst_tool_idle_run_directives_remove (XstTool *tool)
{
	if (tool->directive_queue_idle_id) {
		gtk_idle_remove (tool->directive_queue_idle_id);
		tool->directive_queue_idle_id = 0;
	}
}

static void
xst_tool_kill_backend_cb (XstDirectiveEntry *entry)
{
	XstTool *tool = entry->tool;
	
	/* The backend was never called. No problem! */
	/* For further reference check http://www.xs4all.nl/~pjbrink/apekool/050/045-en.html */
	if (tool->backend_pid > 0) {
		if (root_access == ROOT_ACCESS_SIMULATED)
			root_access = ROOT_ACCESS_SIMULATED_DISABLED;
		
		xst_tool_run_set_directive (tool, NULL, NULL, "end", NULL);
		
		if (root_access == ROOT_ACCESS_SIMULATED_DISABLED)
			root_access = ROOT_ACCESS_SIMULATED;
	
		waitpid (tool->backend_pid, NULL, 0);
	}
	
	gtk_signal_emit_by_name (GTK_OBJECT (tool), "destroy");
}

static void
xst_tool_kill_backend (XstTool *tool, gpointer data)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));

	xst_tool_queue_directive (tool, xst_tool_kill_backend_cb, NULL, NULL, NULL, "end");
}

static gboolean
xst_tool_end_of_request (GString *string)
{
	gchar *str;
	gint len, eorlen, i;

	str = string->str;
	len = strlen (str);
	eorlen = strlen (XST_TOOL_EOR);
	if (len < eorlen)
		return FALSE;

	for (i = 0; i < eorlen; i++)
		if (XST_TOOL_EOR[eorlen - i - 1] != str[len - i - 1])
			return FALSE;

	g_string_truncate (string, len - eorlen + 1);

	return TRUE;
}

static xmlDoc *
xst_tool_read_xml_from_backend (XstTool *tool)
{
	char buffer [4096];
	int t;
	xmlDoc *xml;

	if (!tool->xml_document)
		return NULL;
	
	fcntl (tool->backend_read_fd, F_SETFL, O_NONBLOCK);

	while (! xst_tool_end_of_request (tool->xml_document)) {
		while (gtk_events_pending ())
			gtk_main_iteration ();

		t = read (tool->backend_read_fd, buffer, sizeof (buffer) - 1);

		if (t == 0)
			break;
		if (t == -1)
			t = 0;

		buffer [t] = 0;
		g_string_append (tool->xml_document, buffer);
	}
	
	fcntl (tool->backend_read_fd, F_SETFL, 0);

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
xst_tool_default_set_directive_callback (XstDirectiveEntry *entry)
{
	xst_tool_run_set_directive (entry->tool, entry->in_xml, entry->report_sign,
				    entry->directive, NULL);
}

/* All parameters except directive can be NULL.
   NULL report_sign makes no report window to be shown when directive is run.
   NULL callback runs the default directive callback, which runs a set directive.
   data is for closure. in_xml is an input xml that may be required by the directive. */
void
xst_tool_queue_directive (XstTool *tool, XstDirectiveFunc callback, gpointer data,
			  xmlDoc *in_xml, gchar *report_sign, gchar *directive)
{
	XstDirectiveEntry *entry;

	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));

	entry = g_new0 (XstDirectiveEntry, 1);
	g_return_if_fail (entry != NULL);
	
	entry->tool        = tool;
	entry->callback    = callback? callback: xst_tool_default_set_directive_callback;
	entry->data        = data;
	entry->in_xml      = in_xml;
	entry->report_sign = report_sign;
	entry->directive   = directive;

	tool->directive_queue = g_slist_append (tool->directive_queue, entry);
	xst_tool_idle_run_directives_add (tool);
}

/* As specified, escapes :: to \:: and \ to \\ and joins the strings. */
static GString *
xst_tool_join_directive (const gchar *directive, va_list ap)
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
xst_tool_send_directive (XstTool *tool, const gchar *directive, va_list ap)
{
	GString *directive_line;
	FILE *f;

	g_return_if_fail (tool->backend_pid >= 0);

       	directive_line = xst_tool_join_directive (directive, ap);
	g_string_append_c (directive_line, '\n');
	
	f = fdopen (dup (tool->backend_write_fd), "w");
	fprintf (f, directive_line->str);
	fclose (f);

	g_string_free (directive_line, TRUE);
}

static xmlDoc *
xst_tool_run_get_directive_va (XstTool *tool, const gchar *report_sign, const gchar *directive, va_list ap)
{
	xmlDoc *xml;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->directive_running == FALSE, NULL);

	if (tool->backend_pid < 0)
		xst_tool_init_backend (tool);
	
	tool->directive_running = TRUE;
	xst_tool_idle_run_directives_remove (tool);

	xst_tool_send_directive (tool, directive, ap);

	/* FIXME: Instead of doing the following, we should pass around a value describing
	 * tool I/O mode (as opposed to a string to report_progress ()). */
	tool->report_hook_type = XST_REPORT_HOOK_LOAD;

	xmlSubstituteEntitiesDefault (TRUE);

	/* LibXML support for parsing from memory is good, but parsing from
	 * opened filehandles is not supported unless you write your own feed
	 * mechanism. Let's just load it all into memory, then. Also, refusing
	 * enormous documents can be considered a plus. </dystopic> */
	
	if (tool->xml_document == NULL)
		tool->xml_document = g_string_new ("");

	if (location_id == NULL)
		report_progress (tool, report_sign? _(report_sign): NULL);

	xml = xst_tool_read_xml_from_backend (tool);
	
	tool->directive_running = FALSE;
	xst_tool_idle_run_directives_add (tool);
	
	return xml;
}

xmlDoc *
xst_tool_run_get_directive (XstTool *tool, const gchar *report_sign, const gchar *directive, ...)
{
	va_list ap;
	
	va_start (ap, directive);
	return xst_tool_run_get_directive_va (tool, report_sign, directive, ap);
}

static xmlDoc *
xst_tool_run_set_directive_va (XstTool *tool, xmlDoc *xml,
			       const gchar *report_sign, const gchar *directive, va_list ap)
{
	FILE *f;
	xmlDoc *xml_out;
	
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->directive_running == FALSE, NULL);

	if (tool->backend_pid < 0)
		xst_tool_init_backend (tool);

	tool->directive_running = TRUE;
	xst_tool_idle_run_directives_remove (tool);

	/* don't actually run if we are just pretending */
	if (root_access == ROOT_ACCESS_SIMULATED) {
		g_warning (_("Skipping %s directive..."), directive);
		tool->directive_running = FALSE;
		return NULL;
	}

	xst_tool_send_directive (tool, directive, ap);

	if (xml) {
		f = fdopen (dup (tool->backend_write_fd), "w");
		xmlDocDump (f, xml);
		fprintf (f, XST_TOOL_EOR);
		fclose (f);
	}

	/* FIXME: Instead of doing the following, we should pass around a value describing
	 * tool I/O mode (as opposed to a string to report_progress ()). */
	tool->report_hook_type = XST_REPORT_HOOK_SAVE;

	if (location_id == NULL)
		report_progress (tool, report_sign? _(report_sign): NULL);

	/* This is tipicaly to just read the end of request string,
	   but a set directive may return some XML too. */
	xml_out = xst_tool_read_xml_from_backend (tool);

	tool->directive_running = FALSE;
	xst_tool_idle_run_directives_add (tool);

	return xml_out;
}

xmlDoc *
xst_tool_run_set_directive (XstTool *tool, xmlDoc *xml,
			    const gchar *report_sign, const gchar *directive, ...)
{
	va_list ap;
	
	va_start (ap, directive);
	return xst_tool_run_set_directive_va (tool, xml, report_sign, directive, ap);
}

gboolean
xst_tool_load (XstTool *tool)
{
	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (XST_IS_TOOL (tool), FALSE);	
	g_return_val_if_fail (tool->script_path, FALSE);
  
	if (tool->config) {
		xmlFreeDoc (tool->config);
		tool->config = NULL;
	}

	if (tool->run_again)
		return TRUE;

	if (location_id == NULL) {
		tool->config = xst_tool_run_get_directive
			(tool, _("Scanning your system configuration."), "get", NULL);
	} else {
#ifndef XST_HAVE_ARCHIVER
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


	if (tool->config)
		gtk_signal_emit (GTK_OBJECT (tool), xsttool_signals[FILL_GUI]);

	return tool->config != NULL;
}

#if 0
static void
xst_tool_save_directive_callback (XstDirectiveEntry *entry)
{
	xst_tool_run_set_directive_va (entry->tool, entry->in_xml, entry->report_sign,
				       entry->directive, entry->ap);
	xst_dialog_thaw_visible (entry->tool->main_dialog);
}
#endif

gboolean
xst_tool_save (XstTool *tool)
{
#ifdef XST_HAVE_ARCHIVER
	CORBA_Environment ev;
	ConfigArchiver_Archive archive;
	ConfigArchiver_Location location;
	gchar *backend_id;
#endif	

	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (XST_IS_TOOL (tool), FALSE);
	g_return_val_if_fail (root_access != ROOT_ACCESS_NONE, FALSE);

	xst_dialog_freeze_visible (tool->main_dialog);

	gtk_signal_emit (GTK_OBJECT (tool), xsttool_signals[FILL_XML]);

#ifdef XST_DEBUG
	/* don't actually save if we are just pretending */
	if (root_access == ROOT_ACCESS_SIMULATED) {
		g_warning (_("Skipping actual save..."));
		return TRUE;
	}
#endif

#ifdef XST_HAVE_ARCHIVER
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
	
	if (location_id == NULL)
		xst_tool_run_set_directive (tool, tool->config, _("Updating your system configuration."),
					    "set", NULL);
	xst_dialog_thaw_visible (tool->main_dialog);

	return TRUE;  /* FIXME: Determine if it really worked. */
}

void
xst_tool_save_cb (GtkWidget *w, XstTool *tool)
{
	xst_tool_save (tool);
}

void
xst_tool_load_try (XstTool *tool)
{
	if (!xst_tool_load (tool)) {
		GtkWidget *d;

		d = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_CLOSE,
					    _("There was an error running the backend script,\n"
					      "and the configuration could not be loaded."));

		gtk_dialog_run (GTK_DIALOG (d));
		exit (0);
	}
}

void
xst_tool_main (XstTool *tool, gboolean no_main_loop)
{
	xst_dialog_freeze_visible (tool->main_dialog);
	gtk_widget_show (GTK_WIDGET (tool->main_dialog));

	xst_tool_load_try (tool);
	xst_dialog_thaw_visible (tool->main_dialog);

	if (!no_main_loop)
		gtk_main ();
}

static void
xst_tool_destroy (GtkObject *object)
{
	XstTool *tool;

	tool = XST_TOOL (object);
	
	parent_class->destroy (object);
	gtk_main_quit ();
}

static void
xst_tool_class_init (XstToolClass *klass)
{	
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;

	parent_class = g_type_class_peek_parent (klass);

#if 1
	xsttool_signals[FILL_GUI] = 
		g_signal_new ("fill_gui",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XstToolClass, fill_gui),
			      NULL, NULL,
			      xst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	xsttool_signals[FILL_XML] = 
		g_signal_new ("fill_xml",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XstToolClass, fill_xml),
			      NULL, NULL,
			      xst_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	xsttool_signals[CLOSE] = 
		g_signal_new ("close",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XstToolClass, close),
			      NULL, NULL,
			      xst_marshal_VOID__VOID,
			      GTK_TYPE_NONE, 0);
#else	
	xsttool_signals[FILL_GUI] = 
		gtk_signal_new ("fill_gui",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (XstToolClass, fill_gui),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	xsttool_signals[FILL_XML] = 
		gtk_signal_new ("fill_xml",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (XstToolClass, fill_xml),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	xsttool_signals[CLOSE] = 
		gtk_signal_new ("close",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (XstToolClass, close),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
#endif	

#if 0	
	gtk_object_class_add_signals (object_class, xsttool_signals, LAST_SIGNAL);
#endif	

	object_class->destroy = xst_tool_destroy;
}

#if 0

static void
visibility_toggled (GtkWidget *w, gpointer data)
{
	XstTool *tool;
	gboolean show;
	GtkAdjustment *vadj;

	tool = XST_TOOL (data);

	show = GTK_TOGGLE_BUTTON (w)->active;
	vadj = gtk_scrolled_window_get_vadjustment (
		GTK_SCROLLED_WINDOW (tool->report_scrolled));

	set_arrow (tool, show ? GTK_ARROW_UP : GTK_ARROW_DOWN);

	if (show) {
		gtk_widget_set_usize (tool->report_scrolled, -1, 140);
		gtk_widget_show (tool->report_scrolled);
		gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);
		gtk_adjustment_value_changed (vadj);
	} else { 
		gtk_widget_hide (tool->report_scrolled);
		if (tool->timeout_done)
			gtk_main_quit();
	}

	tool->report_list_visible = show;
}

#endif

static void
xst_tool_type_init (XstTool *tool)
{
	GladeXML *xml;

	tool->glade_common_path  = g_strdup_printf ("%s/common.glade", INTERFACES_DIR);

	tool->etspecs_common_path = g_strdup (ETSPECS_DIR);

	tool->report_gui = xml  = xst_tool_load_glade_common (tool, "report_window");

	tool->report_window     = glade_xml_get_widget (xml, "report_window");
	gtk_signal_connect (GTK_OBJECT (tool->report_window), "delete_event",
			    GTK_SIGNAL_FUNC (report_window_close_cb), tool);
#if 0
	tool->report_scrolled   = glade_xml_get_widget (xml, "report_list_scrolled_window");
	tool->report_progress   = glade_xml_get_widget (xml, "report_progress");
#endif
	tool->report_label      = glade_xml_get_widget (xml, "report_label");
#if 0
	tool->report_list       = glade_xml_get_widget (xml, "report_list");
	tool->report_entry      = glade_xml_get_widget (xml, "report_text");
	tool->report_visibility = glade_xml_get_widget (xml, "report_visibility");
	tool->report_notebook   = glade_xml_get_widget (xml, "report_notebook");

	gtk_notebook_remove_page (GTK_NOTEBOOK (tool->report_notebook), 0);
	gtk_notebook_append_page (GTK_NOTEBOOK (tool->report_notebook),
				  gnome_stock_pixmap_widget_new (tool->report_notebook, 
								 GNOME_STOCK_PIXMAP_UP),
				  NULL);

	gtk_notebook_append_page (GTK_NOTEBOOK (tool->report_notebook),
				  gnome_stock_pixmap_widget_new (tool->report_notebook, 
								 GNOME_STOCK_PIXMAP_DOWN),
				  NULL);

	set_arrow (tool, GTK_ARROW_DOWN);
	gtk_widget_show_all (tool->report_notebook);

	glade_xml_signal_connect_data (xml, "visibility_toggled", visibility_toggled, tool);
#endif

	xml = xst_tool_load_glade_common (tool, "platform_dialog");
	tool->platform_dialog    = glade_xml_get_widget (xml, "platform_dialog");
	tool->platform_list      = glade_xml_get_widget (xml, "platform_list");
	tool->platform_ok_button = glade_xml_get_widget (xml, "platform_ok_button");
}

GtkType
xst_tool_get_type (void)
{
	static GtkType xsttool_type = 0;

	if (!xsttool_type) {
		GtkTypeInfo xsttool_info = {
			"XstTool",
			sizeof (XstTool),
			sizeof (XstToolClass),
			(GtkClassInitFunc) xst_tool_class_init,
			(GtkObjectInitFunc) xst_tool_type_init,
			NULL, NULL, NULL
		};

		xsttool_type = gtk_type_unique (GTK_TYPE_OBJECT, &xsttool_info);
	}

	return xsttool_type;
}

/**
 * xst_tool_construct:
 * @tool: 
 * @name: 
 * @title: 
 * 
 * 
 * 
 * Return Value: FALSE on error
 **/
void
xst_tool_construct (XstTool *tool, const char *name, const char *title)
{
	GdkPixbuf *pb;
	char *s, *t, *u;

	g_return_if_fail (name != NULL);

	tool->name        = g_strdup (name);
	tool->glade_path  = g_strdup_printf ("%s/%s.glade",     INTERFACES_DIR, name);
	tool->script_path = g_strdup_printf ("%s/%s-conf",      SCRIPTS_DIR,    name);
	
	s = g_strdup_printf ("%s_admin", name);
	t = g_strdup_printf (_("%s - GNOME System Tools"), title);
	u = g_strdup_printf (PIXMAPS_DIR "/%s.png", name);

	tool->main_dialog = xst_dialog_new (tool, s, t);

	pb = gdk_pixbuf_new_from_file (u, NULL);
	if (pb) {
		gtk_window_set_icon (GTK_WINDOW (tool->main_dialog), pb);
		gdk_pixbuf_unref (pb);
	}

	g_free (s);
	g_free (t);
	g_free (u);

	gtk_signal_connect (GTK_OBJECT (tool->main_dialog),
			    "apply",
			    GTK_SIGNAL_FUNC (xst_tool_save_cb),
			    tool);

	tool->report_hook_list = NULL;
	memset (&tool->report_hook_defaults, 0, sizeof (XstReportHook *) * XST_MAJOR_MAX);
	xst_tool_add_report_hooks (tool, common_report_hooks);

	tool->backend_pid = tool->backend_read_fd = tool->backend_write_fd = -1;
	xst_tool_set_close_func (tool, xst_tool_kill_backend, NULL);

	tool->directive_running = FALSE;
	tool->directive_queue   = NULL;
	tool->directive_queue_idle_id = 0;
}

XstTool *
xst_tool_new (void)
{
	XstTool *tool;

	tool = XST_TOOL (gtk_type_new (XST_TYPE_TOOL));

	return tool;
}

void
xst_tool_set_xml_funcs (XstTool *tool, XstXmlFunc load_cb, XstXmlFunc save_cb, gpointer data)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));

	if (load_cb)
		gtk_signal_connect (GTK_OBJECT (tool), "fill_gui", GTK_SIGNAL_FUNC (load_cb), data);

	if (save_cb)
		gtk_signal_connect (GTK_OBJECT (tool), "fill_xml", GTK_SIGNAL_FUNC (save_cb), data);
}

void
xst_tool_set_close_func (XstTool *tool, XstCloseFunc close_cb, gpointer data)
{
	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));

	if (close_cb)
		gtk_signal_connect (GTK_OBJECT (tool), "close", GTK_SIGNAL_FUNC (close_cb), data);
}

void
xst_tool_add_report_hooks (XstTool *tool, XstReportHookEntry *report_hook_table)
{
	XstReportHook *hook;
	int i;

	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (report_hook_table != NULL);

	for (i = 0; report_hook_table [i].key; i++)
        {
		hook = xst_report_hook_new_from_entry (&report_hook_table [i]);

		if (!hook)
			continue;

		tool->report_hook_list = g_slist_append (tool->report_hook_list, hook);
	}
}

void
xst_tool_set_default_hook (XstTool *tool, XstReportHookEntry *entry, XstReportMajor major)
{
	g_return_if_fail (major < XST_MAJOR_MAX);
	
	if (tool->report_hook_defaults[major])
		g_free (tool->report_hook_defaults[major]);

	tool->report_hook_defaults[major] = xst_report_hook_new_from_entry (entry);
}

static gboolean
xst_tool_call_report_hook (XstTool *tool, XstReportHook *hook, XstReportLine *rline)
{
	gboolean res;
	
	hook->invoked = TRUE;
	tool->input_block = TRUE;
	res = hook->func (tool, rline, hook->data);
	tool->input_block = FALSE;
	return res;
}

static void
xst_tool_invoke_default_hook (XstTool *tool, XstReportLine *rline)
{
	XstReportHook *hook;
	
	if (tool->report_hook_defaults[rline->major]) {
		hook = tool->report_hook_defaults[rline->major];
		if ((hook->allow_repeat || !hook->invoked) &&
		    (hook->type == XST_REPORT_HOOK_LOADSAVE ||
		     hook->type == tool->report_hook_type)) {
			xst_tool_call_report_hook (tool, hook, rline);
		}
	}
}

void
xst_tool_invoke_report_hooks (XstTool *tool, XstReportHookType type, XstReportLine *rline)
{
	GSList *list;
	XstReportHook *hook;
	const gchar *key;
	gboolean invoked;

	key = xst_report_line_get_key (rline);
	invoked = FALSE;

	for (list = tool->report_hook_list; list; list = g_slist_next (list))
	{
		hook = (XstReportHook *) list->data;

		if (!strcmp (hook->key, key) && (hook->allow_repeat || !hook->invoked) &&
		    (hook->type == XST_REPORT_HOOK_LOADSAVE || hook->type == type)) {
			invoked = TRUE;
			if (xst_tool_call_report_hook (tool, hook, rline))
				return;
		}
	}

	if (!invoked)
		xst_tool_invoke_default_hook (tool, rline);
}

void
xst_tool_reset_report_hooks (XstTool *tool)
{
	GSList *list;

	for (list = tool->report_hook_list; list; list = g_slist_next (list))
		((XstReportHook *) list->data)->invoked = FALSE;
}

/**
 * xst_fool_the_linker:
 * @void: 
 * 
 * We need to keep the symbol for the create image widget function
 * so that libglade can find it to create the icons.
 **/
void xst_fool_the_linker (void);
void
xst_fool_the_linker (void)
{
	xst_ui_image_widget_create (NULL, NULL, NULL, 0, 0);
}

static void
authenticate (int argc, char *argv[])
{
	GtkWidget *error_dialog;
	gchar *password;
	gint result;

	for (;;)
	{
		result = xst_su_get_password (&password);

		if (result < 0)
			exit (0);
		else if (result == 0)
			return;

		/* If successful, the following never returns */

		xst_su_run_with_password (argc, argv, password);

		if (strlen (password))
			memset (password, 0, strlen (password));

		error_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
						       GTK_MESSAGE_ERROR,
						       GTK_BUTTONS_OK,
						       _("The password you entered is invalid."));

		gtk_dialog_run (GTK_DIALOG (error_dialog));
		gtk_widget_destroy (error_dialog);
	}
}

static void
try_show_usage_warning (void)
{
	gchar *key;
	gboolean value;
	gchar *warning = g_strdup_printf(_("Welcome to the %s release of the "
		  "GNOME System Tools.\n\n"
		  "This is still a work in progress, and so it may have serious bugs.\n"
		  "Due to the nature of these tools, bugs may render your computer\n"
		  "PRACTICALLY USELESS, costing time, effort and sanity points.\n\n"
		  "You have been warned. Thank you for trying out this release of\n"
		  "the GNOME System Tools!\n\n"
		  "--\nThe GNOME System Tools team"), VERSION);
	key = g_strjoin ("/", XST_CONF_ROOT, "global", "previously-run-" VERSION, NULL);

	value = gnome_config_get_bool (key);

	if (!value)
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
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox))) {
				gnome_config_set_bool (key, TRUE);
				gnome_config_sync ();
			}
		}
		gtk_widget_hide (dialog);
		gtk_widget_destroy (dialog);
	}

	g_free (warning);
	g_free (key);
}

void
xst_init (const gchar *app_name, int argc, char *argv [], const poptOption options)
{
	GnomeProgram *program;
	struct poptOption xst_options[] =
	{
		{NULL, '\0', 0, NULL, 0}
	};

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#warning FIXME	
#if 0	
        gnomelib_register_popt_table (xst_options, "general XST options");

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
			g_print ("-->%s<--\n", (gchar *) args_list->data);
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
	

	if (geteuid () == 0) {
		root_access = ROOT_ACCESS_REAL;
#ifdef XST_DEBUG
	} else if (getenv ("SET_ME_UP_HARDER")) {
		g_warning (_("Pretending we are root..."));
		root_access = ROOT_ACCESS_SIMULATED;
#endif
	} else {
		authenticate (argc, argv);
		root_access = ROOT_ACCESS_NONE;
	}

	try_show_usage_warning ();
}
