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

#include <gnome.h>
#include <parser.h>
#include <memory.h>

#include <stdio.h>
#include <fcntl.h>

#define XST_DEBUG 1

enum {
	FILL_GUI,
	FILL_XML,
	LAST_SIGNAL
};

static enum {
	ROOT_ACCESS_NONE,
	ROOT_ACCESS_SIMULATED,
	ROOT_ACCESS_REAL
} root_access = ROOT_ACCESS_NONE;

static GtkObjectClass *parent_class;
static gint xsttool_signals [LAST_SIGNAL] = { 0 };

static gboolean platform_unsupported_cb   (XstTool *tool, XstReportLine *rline);
static gboolean platform_add_supported_cb (XstTool *tool, XstReportLine *rline);
static gboolean platform_set_current_cb   (XstTool *tool, XstReportLine *rline);
static gboolean report_finished_cb        (XstTool *tool, XstReportLine *rline);

static void report_dispatch (XstTool *tool);

static gint xst_tool_compare_platforms (XstPlatform *a, XstPlatform *b);

static XstReportHookEntry common_report_hooks[] = {
	{ "end",              report_finished_cb,         XST_REPORT_HOOK_LOADSAVE, TRUE  },
	{ "platform_list",    platform_add_supported_cb,  XST_REPORT_HOOK_LOAD,     TRUE  }, 
	{ "platform_unsup",   platform_unsupported_cb,    XST_REPORT_HOOK_LOADSAVE, FALSE },
	{ "platform_undet",   platform_unsupported_cb,    XST_REPORT_HOOK_LOADSAVE, FALSE },
	{ "platform_success", platform_set_current_cb,    XST_REPORT_HOOK_LOAD,     FALSE }, 
	{ 0,                  NULL,                       -1,                       FALSE }
};

/* --- Report hook callbacks --- */

static void
platform_list_select_row_cb (GtkCList *clist, gint row, gint column, GdkEvent *event,
			     gpointer data)
{
	XstTool *tool;

	tool = (XstTool *) data;
	tool->platform_selected_row = row;
	gtk_widget_set_sensitive (tool->platform_ok_button, TRUE);
}

static void
platform_list_unselect_row_cb (GtkCList *clist, gint row, gint column, GdkEvent *event,
			       gpointer data)
{
	XstTool *tool;

	tool = (XstTool *) data;
	tool->platform_selected_row = -1;
	gtk_widget_set_sensitive (tool->platform_ok_button, FALSE);
}

static void
platform_dialog_close_cb (GtkWidget *dialog, XstTool *tool)
{
	exit (0);
}

static void
platform_unsupported_clicked_cb (GtkWidget *widget, gint ret, XstTool *tool)
{
	if (ret == -1 || ret == 1)
		exit (0);

	/* Locate the selected platform and set it up as the current one. Prepare to
	 * invoke the backend again, this time with the current platform specified. */

	if (tool->current_platform)
		xst_platform_free (tool->current_platform);

	tool->current_platform =
		xst_platform_dup ((XstPlatform *)
				 g_slist_nth_data (tool->supported_platforms_list,
						   tool->platform_selected_row));

	g_assert (tool->current_platform);

	tool->report_dispatch_pending = FALSE;
	tool->report_finished = TRUE;
	tool->run_again = TRUE;

	report_dispatch (tool);

	gtk_signal_disconnect_by_func (GTK_OBJECT (tool->platform_dialog),
				       platform_dialog_close_cb,
				       tool);
	
}

static gboolean
platform_unsupported_cb (XstTool *tool, XstReportLine *rline)
{
	GSList *list;
	XstPlatform *platform;
	char *platform_text[1];

	tool->report_dispatch_pending = TRUE;

	/* Fill in the platform GtkCList */

	gtk_clist_clear (GTK_CLIST (tool->platform_list));

	for (list = tool->supported_platforms_list; list;
	     list = g_slist_next (list))
	{
		platform = (XstPlatform *) list->data;

		platform_text [0] = (char *) xst_platform_get_name (platform);
		gtk_clist_append (GTK_CLIST (tool->platform_list), platform_text);
	}

	/* Prepare dialog and show it */

	tool->platform_selected_row = -1;

	gtk_signal_connect (GTK_OBJECT (tool->platform_list), "select-row",
			    platform_list_select_row_cb, tool);
	gtk_signal_connect (GTK_OBJECT (tool->platform_list), "unselect-row",
			    platform_list_unselect_row_cb, tool);
	gtk_signal_connect (GTK_OBJECT (tool->platform_dialog), "clicked",
			    platform_unsupported_clicked_cb, tool);
	gtk_signal_connect (GTK_OBJECT (tool->platform_dialog), "close",
			    platform_dialog_close_cb, tool);

	gtk_widget_set_sensitive (tool->platform_ok_button, FALSE);
	gtk_widget_show (tool->platform_dialog);

	return TRUE;
}

static gboolean
platform_add_supported_cb (XstTool *tool, XstReportLine *rline)
{
	XstPlatform *platform;

	platform = xst_platform_new_from_report_line (rline);
	g_return_val_if_fail (platform != NULL, TRUE);

	xst_tool_add_supported_platform (tool, platform);
	return TRUE;
}

static gboolean
platform_set_current_cb (XstTool *tool, XstReportLine *rline)
{
	XstPlatform *platform;

	platform = xst_platform_new_from_report_line (rline);
	g_return_val_if_fail (platform != NULL, TRUE);

	if (tool->current_platform)
		xst_platform_free (tool->current_platform);

	tool->current_platform = platform;
	return TRUE;
}

/* --- Other Report Hooks --- */

static gboolean
report_finished_cb (XstTool *tool, XstReportLine *rline)
{
	tool->report_finished = TRUE;
	return TRUE;
}

/* --- Utility function --- */

static gint
xst_tool_compare_platforms (XstPlatform *a, XstPlatform *b)
{
	return strcmp (a->name, b->name);
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
		    (GCompareFunc) xst_tool_compare_platforms) == NULL);

	tool->supported_platforms_list =
		g_slist_insert_sorted (tool->supported_platforms_list, platform,
		    (GCompareFunc) xst_tool_compare_platforms);
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
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->glade_common_path != NULL, NULL);

	return glade_xml_new (tool->glade_common_path, widget);
}

GladeXML *
xst_tool_load_glade (XstTool *tool, const gchar *widget)
{
	g_return_val_if_fail (tool != NULL, NULL);
	g_return_val_if_fail (XST_IS_TOOL (tool), NULL);
	g_return_val_if_fail (tool->glade_path != NULL, NULL);

	return glade_xml_new (tool->glade_path, widget);
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

	if (tool->report_finished && !tool->report_dispatch_pending)
		gtk_main_quit ();
}

static void
report_progress_tick (gpointer data, gint fd, GdkInputCondition cond)
{
	XstTool *tool;
	XstReportLine *rline;
	char c;

	tool = XST_TOOL (data);

#warning FIXME: make this not suck
	if (!tool->line) {
		tool->line = g_malloc(1024);
		tool->line_len = 0;
		tool->line [0] = '\0';
	}

	/* NOTE: The read() being done here is inefficient, but we're not
	 * going to handle any large amount of data */

	if (read (fd, &c, 1) > 0) {
		if (c == '\n') {
			/* End of line */

		        /* Report line; add to list */
			rline = xst_report_line_new_from_string (tool->line);
			
			if (rline)
				tool->report_line_list = g_slist_append (tool->report_line_list,
									 rline);

			tool->line [0] = '\0';
			tool->line_len = 0;
		} else {
			/* Add character to end of current line */

			tool->line = g_realloc (tool->line, MAX (tool->line_len + 2, 1024));
			tool->line [tool->line_len] = c;
			tool->line_len++;
			tool->line [tool->line_len] = '\0';
		}
	}
	else
	{
		/* Zero-length read; pipe closed unexpectedly */
		/* FIXME: handle this at the UI level correctly. */

		tool->report_finished = TRUE;
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
report_window_close_cb (GtkWidget *window, gpointer data)
{
	gtk_widget_hide (window);
	return TRUE;
}

static void
report_progress (XstTool *tool, int fd, const gchar *label)
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

	gtk_label_set_text (GTK_LABEL (tool->report_label), label);
#if 0
	set_arrow (tool, GTK_ARROW_DOWN);
	gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress), 0.0);
#endif
	tool->input_id = gtk_input_add_full (fd, GDK_INPUT_READ, report_progress_tick,
					     NULL, tool, NULL);

	gtk_signal_connect_after (GTK_OBJECT (tool->report_window), "delete-event",
				  GTK_SIGNAL_FUNC (report_window_close_cb), NULL);
	gtk_widget_show (tool->report_window);
	
	gtk_main ();

	if (tool->input_id)
		gtk_input_remove (tool->input_id);

	if (!tool->run_again)
	{
		/* Set progress to 100% and wait a bit before closing display */
#if 0
		gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress), 1.0);

		cb_id = gtk_timeout_add (1500, (GtkFunction) timeout_cb, tool);
		gtk_main ();
		gtk_timeout_remove (cb_id);
#endif
		gtk_widget_hide (tool->report_window);
	}

#if 0
	gtk_clist_clear (GTK_CLIST (tool->report_list));
#endif
}

gboolean
xst_tool_save (XstTool *tool)
{
	FILE *f;
#warning This might be a security problem, fix
	FILE *debug_file;
	int fd_xml [2], fd_report [2];
	int t;

	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (XST_IS_TOOL (tool), FALSE);
	g_return_val_if_fail (tool->script_path, FALSE);

	g_return_val_if_fail (root_access != ROOT_ACCESS_NONE, FALSE);

	xst_dialog_freeze_visible (tool->main_dialog);

	gtk_signal_emit (GTK_OBJECT (tool), xsttool_signals[FILL_XML]);

	/* FIXME: Instead of doing the following, we should pass around a value describing
	 * tool I/O mode (as opposed to a string to report_progress ()). */
	tool->report_hook_type = XST_REPORT_HOOK_SAVE;

#ifdef XST_DEBUG
	/* don't actually save if we are just pretending */
	if (root_access == ROOT_ACCESS_SIMULATED) {
		g_warning (_("Skipping actual save..."));
		return TRUE;
	}
#endif

	pipe (fd_xml);
	pipe (fd_report);

	t = fork ();
	if (t < 0) {
		g_error ("Unable to fork.");
	} else if (t) {
		/* Parent */

		close (fd_xml [0]);	/* Close reading end of XML pipe */
		close (fd_report [1]);  /* Close writing end of report pipe */

		f = fdopen (fd_xml [1], "w");
		debug_file = fopen ("/tmp/xst-fe-write", "w");

		xmlDocDump (f, tool->config);
		xmlDocDump (debug_file, tool->config);
		
		fclose (f);
		fclose (debug_file);
		
		report_progress (tool, fd_report [0], _("Updating your system configuration."));
		close (fd_report [0]);

	} else {
		/* Child */

		close (fd_xml [1]);	/* Close writing end of XML pipe */
		close (fd_report [0]);  /* Close reading end of report pipe */
		dup2 (fd_xml [0], STDIN_FILENO);
		dup2 (fd_report [1], STDOUT_FILENO);

		if (tool->current_platform)
			execl (tool->script_path, tool->script_path,
			       "--set", "--progress", "--report", "--platform",
			       xst_platform_get_name (tool->current_platform), NULL);
		else
			execl (tool->script_path, tool->script_path, "--set", "--progress",
			       "--report", NULL);

		g_error ("Unable to run backend: %s", tool->script_path);
	}

	xst_dialog_thaw_visible (tool->main_dialog);
	return TRUE;  /* FIXME: Determine if it really worked. */
}

void
xst_tool_save_cb (GtkWidget *w, XstTool *tool)
{
	xst_tool_save (tool);
}

gboolean
xst_tool_load (XstTool *tool)
{
	int fd [2];
	int t, len;
	char *p;

	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (XST_IS_TOOL (tool), FALSE);	
	g_return_val_if_fail (tool->script_path, FALSE);
  
	/* FIXME: Instead of doing the following, we should pass around a value describing
	 * tool I/O mode (as opposed to a string to report_progress ()). */
	tool->report_hook_type = XST_REPORT_HOOK_LOAD;

	xmlSubstituteEntitiesDefault (TRUE);

	if (tool->config) {
		xmlFreeDoc (tool->config);
		tool->config = NULL;
	}

	pipe (fd);
			  
	t = fork ();

	if (t < 0) {
		g_error ("Unable to fork.");
	} else if (t) {
		/* Parent */

		close (fd [1]);	/* Close writing end */

		/* LibXML support for parsing from memory is good, but parsing from
		 * opened filehandles is not supported unless you write your own feed
		 * mechanism. Let's just load it all into memory, then. Also, refusing
		 * enormous documents can be considered a plus. </dystopic> */

		report_progress (tool, fd [0], _("Scanning your system configuration."));

		if (tool->run_again)
			return TRUE;

		p = g_malloc (102400);
		fcntl(fd [0], F_SETFL, 0);  /* Let's block */
		len = 0; 
		for (len = 0; (t = read (fd [0], p + len, 102399 - len)); len += t)
			;

		if (len < 1 || len == 102399) {
			g_free (p);
		} else {
			p = g_realloc (p, len + 1);
			*(p + len) = 0;

			tool->config = xmlParseDoc (p);
			g_free (p);
			close (fd [0]);
		}
	} else {
		/* Child */

		close (fd [0]);	/* Close reading end */
		dup2 (fd [1], STDOUT_FILENO);

		if (tool->current_platform)
			execl (tool->script_path, tool->script_path,
			       "--get", "--progress", "--report", "--platform",
			       xst_platform_get_name (tool->current_platform), NULL);
		else
			execl (tool->script_path, tool->script_path, "--get", "--progress",
			       "--report", NULL);

		g_error ("Unable to run backend: %s", tool->script_path);
	}
	
	if (tool->config)
		gtk_signal_emit (GTK_OBJECT (tool), xsttool_signals[FILL_GUI]);

	return tool->config != NULL;
}

void
xst_tool_load_try (XstTool *tool)
{
	GtkWidget *d;

	do
	{
		tool->run_again = FALSE;

		if (!xst_tool_load (tool)) {
			d = gnome_ok_dialog (_("There was an error running the backend script,\n"
					       "and the configuration could not be loaded."));
			gnome_dialog_run_and_close (GNOME_DIALOG (d));
			exit (1);
		}
	}
	while (tool->run_again);
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

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

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

	gtk_object_class_add_signals (object_class, xsttool_signals, LAST_SIGNAL);

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

	xml = xst_tool_load_glade_common (tool, "report_window");

	tool->report_window     = glade_xml_get_widget (xml, "report_window");
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

void
xst_tool_construct (XstTool *tool, const char *name, const char *title)
{
	char *s, *t;

	g_return_if_fail (name != NULL);

	tool->name        = g_strdup (name);
	tool->glade_path  = g_strdup_printf ("%s/%s.glade",     INTERFACES_DIR, name);
	tool->script_path = g_strdup_printf ("%s/%s-conf",      SCRIPTS_DIR,    name);
	
	s = g_strdup_printf ("%s_admin", name);
	t = g_strdup_printf (_("%s - Ximian Setup Tools"), title);
	
	tool->main_dialog = xst_dialog_new (tool, s, t);

	g_free (s);
	g_free (t);

	gtk_signal_connect (GTK_OBJECT (tool->main_dialog),
			    "apply",
			    GTK_SIGNAL_FUNC (xst_tool_save_cb),
			    tool);

	tool->report_hook_list = NULL;
	xst_tool_add_report_hooks (tool, common_report_hooks);
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
		gtk_signal_connect (GTK_OBJECT (tool), "destroy", GTK_SIGNAL_FUNC (close_cb), data);
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
xst_tool_invoke_report_hooks (XstTool *tool, XstReportHookType type, XstReportLine *rline)
{
	GSList *list;
	XstReportHook *hook;
	const gchar *key;

	key = xst_report_line_get_key (rline);

	for (list = tool->report_hook_list; list; list = g_slist_next (list))
	{
		hook = (XstReportHook *) list->data;

		if (!strcmp (hook->key, key) && (hook->allow_repeat || !hook->invoked) &&
		    (hook->type == XST_REPORT_HOOK_LOADSAVE || hook->type == type)) {
			hook->invoked = TRUE;
			hook->func (tool, rline);
		}
	}
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
	xst_ui_create_image_widget (NULL, NULL, NULL, 0, 0);
}

static void
authenticate (gchar *exec_path)
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

		xst_su_run_with_password (exec_path, password);

		if (strlen (password))
			memset (password, 0, strlen (password));

		error_dialog = gnome_error_dialog (_("The password you entered is invalid."));
		gnome_dialog_run_and_close (GNOME_DIALOG (error_dialog));
	}
}

void
xst_init (const gchar *app_name, int argc, char *argv [], const poptOption options)
{
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
#endif

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

	glade_gnome_init ();

	if (geteuid () == 0) {
		root_access = ROOT_ACCESS_REAL;
#ifdef XST_DEBUG
	} else if (getenv ("SET_ME_UP_HARDER")) {
		g_warning (_("Pretending we are root..."));
		root_access = ROOT_ACCESS_SIMULATED;
#endif
	} else {
		authenticate (argv [0]);
		root_access = ROOT_ACCESS_NONE;
	}
}
