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
static gint xsttool_signals[LAST_SIGNAL] = { 0 };

static gboolean platform_unknown_cb (XstTool *tool, guint id, const gchar *message);
static gboolean platform_unsupported_cb (XstTool *tool, guint id, const gchar *message);
static gboolean platform_add_supported_cb (XstTool *tool, guint id, const gchar *message);
static gboolean platform_set_current_cb (XstTool *tool, guint id, const gchar *message);

static XstReportHookEntry common_report_hooks[] = {
	{ 599, platform_unknown_cb,        XST_REPORT_HOOK_LOADSAVE, FALSE },  /* Unable to guess platform */
	{ 598, platform_unsupported_cb,    XST_REPORT_HOOK_LOADSAVE, FALSE },  /* Unsupported platform */
	{ 296, platform_add_supported_cb,  XST_REPORT_HOOK_LOAD,     TRUE  },  /* Supported: [platform] */
	{ 295, platform_set_current_cb,    XST_REPORT_HOOK_LOAD,     FALSE },  /* Configuring for [platform] */
	{ 0,   NULL,                       -1,                       FALSE }
};

/* --- Report hook callbacks --- */

static gboolean
platform_unknown_cb (XstTool *tool, guint id, const gchar *message)
{
	GtkWidget *d;

	d = gnome_ok_dialog (_("The backend was unable to guess your computer's\n"
			       "platform. In the future, you will be able to specify\n"
			       "one at this point, but today we'll just quit."));

	gnome_dialog_run_and_close (GNOME_DIALOG (d));
	exit (1);
}

static gboolean
platform_unsupported_cb (XstTool *tool, guint id, const gchar *message)
{
	GtkWidget *d;

	d = gnome_ok_dialog (_("The platform you're running is currently not supported\n"
			       "by this tool. In the future, you will be able to specify\n"
			       "another platform at this point, but today we'll just quit."));

	gnome_dialog_run_and_close (GNOME_DIALOG (d));
	exit (1);
}

static gboolean
platform_add_supported_cb (XstTool *tool, guint id, const gchar *message)
{
	/* TODO: Build list of supported platforms as these come in. They
	 * will be needed for the choose-a-platform list if the tool
	 * breaks (platform_unknown_cb () or platform_unsupported_cb ()). */

	return TRUE;
}

static gboolean
platform_set_current_cb (XstTool *tool, guint id, const gchar *message)
{
	/* TODO: Store name of currently configured-for platform
	 * in tool. This can be specified on --set, to ensure that
	 * --platform is the same as it was on --get. */

	return TRUE;
}

/* --- XstTool --- */

void
xst_tool_add_supported_platform (XstTool *tool, const gchar *platform)
{
}

void
xst_tool_clear_supported_platforms (XstTool *tool)
{
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

static void
report_dispatch_lines (XstTool *tool)
{
	GSList *list;
	XstReportLine *rline;
	GtkAdjustment *vadj;
	char *report_text[] = {
		NULL,
		NULL
	};

	vadj = gtk_scrolled_window_get_vadjustment (
		GTK_SCROLLED_WINDOW (tool->report_scrolled));

	for (list = tool->report_line_list; list; list = g_slist_next (list))
	{
		rline = (XstReportLine *) list->data;

		if (xst_report_line_get_handled (rline))
			continue;

		if (xst_report_line_get_id (rline) < 100) {
			/* Progress update */

			gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress),
						     xst_report_line_get_id (rline) / 100.0);
		} else {
			gboolean scroll;
			
			/* Report line */
			
			gtk_entry_set_text (GTK_ENTRY (tool->report_entry),
					    xst_report_line_get_message (rline));
			
			if (vadj->value >= vadj->upper - vadj->page_size)
				scroll = TRUE;
			else
				scroll = FALSE;

			/* FIXME: Is g_strdup () right? gtk_clist_append () expects
			 * non-const... */

			report_text [1] = g_strdup (xst_report_line_get_message (rline));
			gtk_clist_append (GTK_CLIST (tool->report_list), report_text);

			if (scroll)
				gtk_adjustment_set_value (vadj, vadj->upper - vadj->page_size);

			tool->report_dispatch_pending = TRUE;
			xst_tool_invoke_report_hooks (tool, tool->report_hook_type, rline);
			tool->report_dispatch_pending = FALSE;
		}

		xst_report_line_set_handled (rline, TRUE);
	}
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

			if (tool->line_len < 3)
			{
				/* End of report */

				tool->report_finished = TRUE;
			}
			else
			{
				/* Report line; add to list */

				rline = xst_report_line_new_from_string (tool->line);

				if (rline)
					tool->report_line_list = g_slist_append (tool->report_line_list,
										 rline);
			}

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

	if (!tool->report_dispatch_pending)
		report_dispatch_lines (tool);

	if (tool->report_finished && !tool->report_dispatch_pending)
		gtk_main_quit ();
}

static void
report_progress (XstTool *tool, int fd, const gchar *label)
{
	guint input_id;
	gint cb_id;

	tool->timeout_done = FALSE;
	tool->report_list_visible = FALSE;

	gtk_label_set_text (GTK_LABEL (tool->report_label), label);

	set_arrow (tool, GTK_ARROW_DOWN);

	gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress), 0.0);
	
	input_id = gtk_input_add_full (fd, GDK_INPUT_READ, report_progress_tick,
				       NULL, tool, NULL);

	gtk_widget_show (tool->report_window);
	
	gtk_main ();

	gtk_input_remove (input_id);

	/* Set progress to 100% and wait a bit before closing display */
  
	gtk_progress_set_percentage (GTK_PROGRESS (tool->report_progress), 1.0);

	cb_id = gtk_timeout_add (1500, (GtkFunction) timeout_cb, tool);
	gtk_main ();
	gtk_timeout_remove (cb_id);
	
	gtk_widget_hide (tool->report_window);
	gtk_clist_clear (GTK_CLIST (tool->report_list));
}

gboolean
xst_tool_save (XstTool *tool)
{
	FILE *f;
	int fd_xml [2], fd_report [2];
	int t;

	g_return_val_if_fail (tool != NULL, FALSE);
	g_return_val_if_fail (XST_IS_TOOL (tool), FALSE);
	g_return_val_if_fail (tool->script_path, FALSE);

	g_return_val_if_fail (root_access != ROOT_ACCESS_NONE, FALSE);

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

		xmlDocDump (f, tool->config);
		fclose (f);
		report_progress (tool, fd_report [0], _("Updating your system configuration."));
		close (fd_report [0]);
	} else {
		/* Child */

		close (fd_xml [1]);	/* Close writing end of XML pipe */
		close (fd_report [0]);  /* Close reading end of report pipe */
		dup2 (fd_xml [0], STDIN_FILENO);
		dup2 (fd_report [1], STDOUT_FILENO);

		execl (tool->script_path, tool->script_path, "--set", "--progress", "--report", NULL);
		g_error ("Unable to run backend: %s", tool->script_path);
	}
  
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

		p = g_malloc (102400);
		fcntl(fd [0], F_SETFL, 0);  /* Let's block */
		len = 0; 
		for (len = 0; (t = read (fd [0], p + len, 102399 - len)); len += t)
			;

		if (len < 1 || len == 102399) {
			g_free (p);
			g_error ("Backend malfunction.");
		}

		p = g_realloc (p, len + 1);
		*(p + len) = 0;

		tool->config = xmlParseDoc (p);
		g_free (p);
		close (fd [0]);
	} else {
		/* Child */

		close (fd [0]);	/* Close reading end */
		dup2 (fd [1], STDOUT_FILENO);

		execl (tool->script_path, tool->script_path, "--get", "--progress", "--report", NULL);
		g_error ("Unable to run backend: %s", tool->script_path);
	}
	
	if (tool->config)
		gtk_signal_emit (GTK_OBJECT (tool), xsttool_signals[FILL_GUI]);

	return tool->config != NULL;
}

void
xst_tool_main (XstTool *tool)
{
	GtkWidget *d;

	if (!xst_tool_load (tool)) {
		d = gnome_ok_dialog (_("There was an error running the backend script,\n"
				       "and the configuration could not be loaded."));
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		exit (1);
	}
	xst_dialog_thaw (tool->main_dialog);
	gtk_widget_show (GTK_WIDGET (tool->main_dialog));
	gtk_main ();
}

static void
xst_tool_destroy (XstTool *tool)
{
	parent_class->destroy (GTK_OBJECT (tool));
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

static void
xst_tool_type_init (XstTool *tool)
{
	GladeXML *xml;

	tool->glade_common_path  = g_strdup_printf ("%s/common.glade", INTERFACES_DIR);

	xml = xst_tool_load_glade_common (tool, "report_window");

	tool->report_window     = glade_xml_get_widget (xml, "report_window");
	tool->report_scrolled   = glade_xml_get_widget (xml, "report_list_scrolled_window");
	tool->report_progress   = glade_xml_get_widget (xml, "report_progress");
	tool->report_label      = glade_xml_get_widget (xml, "report_label");
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

	xst_dialog_freeze (tool->main_dialog);
	gtk_signal_connect (GTK_OBJECT (tool->main_dialog),
			    "apply",
			    GTK_SIGNAL_FUNC (xst_tool_save_cb),
			    tool);

	tool->report_hook_list = NULL;
	xst_tool_add_report_hooks (tool, common_report_hooks);
}

XstTool *
xst_tool_new (const char *name, const char *title)
{
	XstTool *tool;
	g_return_val_if_fail (name != NULL, NULL);
       
	tool = XST_TOOL (gtk_type_new (XST_TYPE_TOOL));
	xst_tool_construct (tool, name, title);

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
xst_tool_add_report_hooks (XstTool *tool, XstReportHookEntry *report_hook_table)
{
	XstReportHook *hook;
	int i;

	g_return_if_fail (tool != NULL);
	g_return_if_fail (XST_IS_TOOL (tool));
	g_return_if_fail (report_hook_table != NULL);

	for (i = 0; report_hook_table [i].id; i++)
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
	guint id;

	id = xst_report_line_get_id (rline);

	for (list = tool->report_hook_list; list; list = g_slist_next (list))
	{
		hook = (XstReportHook *) list->data;

		if (hook->id == id && (hook->allow_repeat || !hook->invoked) &&
		    (hook->type == XST_REPORT_HOOK_LOADSAVE || hook->type == type)) {
			hook->invoked = TRUE;
			hook->func (tool, id, xst_report_line_get_message (rline));
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

XstTool *
xst_tool_init (const char *name, const char *title, int argc, char *argv [])
{
	GtkWidget *d;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (title != NULL, NULL);

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
#endif
	
	gnome_init (name, VERSION, argc, argv);
	glade_gnome_init ();

	if (geteuid () == 0) {
		root_access = ROOT_ACCESS_REAL;
#ifdef XST_DEBUG
	} else if (getenv ("SET_ME_UP_HARDER")) {
		g_warning (_("Pretending we are root..."));
		root_access = ROOT_ACCESS_SIMULATED;
#endif
	} else {
		d = gnome_ok_cancel_dialog (_("You need full administration privileges (i.e. root)\n"
					      "to run this configuration tool. You can acquire\n"
					      "such privileges by running it from the GNOME Control\n"
					      "Center or issuing an \"su\" command in a shell.\n\n"
					      "You will be unable to make any changes. Continue anyway?"), 
					    NULL, NULL);

		if (gnome_dialog_run_and_close (GNOME_DIALOG (d)))
			exit (1);
		
		root_access = ROOT_ACCESS_NONE;
	}

	return xst_tool_new (name, title);
}
