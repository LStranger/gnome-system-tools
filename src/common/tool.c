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
 * Authors: Hans Petter Jansson <hpj@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <gnome.h>
#include <tree.h>
#include <parser.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "tool.h"
#include "xml.h"

/*#include "reading.xpm"*/

/* libglade callbacks */
void tool_report_list_visible_toggle (GtkWidget *, gpointer);
void tool_user_apply (GtkWidget *, gpointer);
gint tool_user_delete (GtkWidget *, GdkEvent *, gpointer);
void tool_user_help (GtkWidget *w, gpointer);
void tool_user_complexity (GtkWidget *w, gpointer);

/* --- Internals --- */


ToolContext *tool_context = NULL;


static gchar *
make_script_name (char *task)
{
	return g_strconcat (task, "-conf", NULL);
}


static gchar *
make_script_path (char *task)
{
	gchar *name, *path;

	name = make_script_name (task);
	path = g_concat_dir_and_file (SCRIPTS_DIR, name);
	g_free (name);
	return (path);
}


static gchar *
make_glade_path (char *task)
{
	return g_strdup_printf ("%s/%s.glade", INTERFACES_DIR, task);
}


GdkPixbuf *
tool_load_image (char *image_name)
{
	GdkPixbuf *pixbuf;
	gchar *path;

	path = g_concat_dir_and_file (PIXMAPS_DIR, image_name);
	pixbuf = gdk_pixbuf_new_from_file (path);
	if (!pixbuf) g_warning ("Couldn't load image \"%s\".", path);
	g_free (path);
	return pixbuf;
}


GtkWidget *
tool_widget_get (gchar *name)
{
	GtkWidget *ret;
	
	ret = glade_xml_get_widget (tool_context->interface, name);
	if (ret == NULL)
		g_warning ("tool_widget_get: widget %s not found.", name);
	
	return (ret);
}


GtkWidget *
tool_widget_get_common (gchar *name)
{
	return (glade_xml_get_widget (tool_context->common_interface, name));
}


static gboolean
tool_interface_load ()
{
	GtkWidget *vbox, *icon, *button;
	gchar *path;
	
	/* Load interface from Glade file */

	path = make_glade_path (tool_context->task);
	tool_context->interface = glade_xml_new (path, NULL);
	if (!tool_context->interface)
	        g_error ("Could not load tool interface from %s", path);

	g_free (path);

	glade_xml_signal_autoconnect (tool_context->interface);

	/* Load common interface elements from Glade file */

	path = make_glade_path ("common");
	tool_context->common_interface = glade_xml_new (path, NULL);
	if (!tool_context->common_interface)
		g_error ("Could not load common interface elements from %s", path);

	g_free (path);
	
	glade_xml_signal_autoconnect (tool_context->common_interface);

	/* Locate and save a pointer to the top level window and child widget */

	tool_context->top_window = tool_widget_get_common ("tool_window");
	vbox = tool_widget_get_common ("tool_vbox");

	path = g_strconcat (tool_context->task, "_admin", NULL);
	tool_context->child = tool_widget_get (path);
	g_free (path);

#warning FIXME: this really needs to be a dialog
	if (!tool_context->child)
	        g_error ("Undefined toplevel window in Glade file.");

	gtk_widget_ref (tool_context->child);
	gtk_widget_unparent (tool_context->child);

	gtk_box_pack_start (GTK_BOX (vbox), tool_context->child,
			    TRUE, TRUE, 0);

	button = tool_widget_get_common ("help");
	icon = gnome_stock_pixmap_widget (button, GNOME_STOCK_PIXMAP_HELP);
	gtk_widget_show (icon);
	gtk_container_add (GTK_CONTAINER (button), icon);

	return TRUE;
}


static ToolContext *
tool_context_new (gchar *task)
{
	ToolContext *tc;
  
	tc = g_new0 (ToolContext, 1);
	tc->task = g_strdup (task);

	tool_context = tc;
	tool_interface_load (tc);
	tool_set_modified (FALSE);
	return tc;
}


void
tool_context_destroy (ToolContext *tc)
{
	g_assert (tc->task);

	g_free (tc->task);
	/* TODO: Free interface */
	/* TODO: Free config */
	g_free (tc);
}


static void
destroy_widget_cb (GtkWidget *w, gpointer data)
{
	gtk_container_remove (GTK_CONTAINER (tool_widget_get_common ("report_visibility")),
			      w);
}


static void
progress_folding_button_set_hidden ()
{
	GtkWidget *w;

	w = gnome_stock_pixmap_widget (tool_get_top_window (),
				       GNOME_STOCK_PIXMAP_DOWN);
	gtk_widget_show (w);
	
	gtk_container_foreach (GTK_CONTAINER (tool_widget_get_common ("report_visibility")),
			       destroy_widget_cb, NULL);

	gtk_container_add (GTK_CONTAINER (tool_widget_get_common ("report_visibility")),
			   w);
}


static void
progress_folding_button_set_visible ()
{
	GtkWidget *w;

	w = gnome_stock_pixmap_widget (tool_get_top_window (),
				       GNOME_STOCK_PIXMAP_UP);
	gtk_widget_show (w);
	
	gtk_container_foreach (GTK_CONTAINER (tool_widget_get_common ("report_visibility")),
			       destroy_widget_cb, NULL);

	gtk_container_add (GTK_CONTAINER (tool_widget_get_common ("report_visibility")),
			   w);
}


static gboolean timeout_done = FALSE;
static gboolean report_list_visible = FALSE;


void
tool_report_list_visible_toggle (GtkWidget *w, gpointer null)
{
	GtkAdjustment *vadj;
	
	vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (tool_widget_get_common ("report_list_scrolled_window")));
	report_list_visible = ~report_list_visible;
	
	if (report_list_visible)
	{
		progress_folding_button_set_visible ();
		
		gtk_widget_set_usize (tool_widget_get_common ("report_list_scrolled_window"),
				      -1, 140);
		gtk_widget_show (tool_widget_get_common ("report_list_scrolled_window"));
		gtk_adjustment_set_value (vadj,
					  vadj->upper - vadj->page_size);
		gtk_adjustment_value_changed (vadj);
	}
	else
	{
		progress_folding_button_set_hidden ();
		
		gtk_widget_hide (tool_widget_get_common ("report_list_scrolled_window"));
		if (timeout_done)
			gtk_main_quit();
	}
}


static void
timeout_cb (void)
{
	timeout_done = TRUE;

	if (timeout_done && !report_list_visible)
		gtk_main_quit();
}


static void
report_progress_tick (gpointer data, gint fd, GdkInputCondition cond)
{
	char c;
	GtkWidget *bar, *report, *report_list, *report_list_scrolled_window;
	GtkAdjustment *vadj;
/*	gfloat p;*/
	static char *line = NULL;
	static int line_len = 0;
	char *report_text[] =
	{
		NULL,
		NULL
	};

	bar = tool_widget_get_common ("report_progress");
	report = tool_widget_get_common ("report_text");
	report_list = tool_widget_get_common ("report_list");
	report_list_scrolled_window = tool_widget_get_common ("report_list_scrolled_window");
	vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (report_list_scrolled_window));

	if (!line)
	{
		line = malloc(1);
		line [0] = '\0';
	}

	/* NOTE: The read() being done here is inefficient, but we're not
	 * going to handle any large amount of data */
	
	if (read (fd, &c, 1) > 0)
	{
		if (c == '\n')
		{
			/* End of line. Take action. */
			
			if (line_len < 3)
			{
				/* End of headers. We're done. */
				
				gtk_main_quit ();
			}
			else
			{
				if (line [0] == '0')
				{
					/* Progress update */

					gtk_progress_set_percentage (GTK_PROGRESS(bar),
								     (gfloat) ((line [1] - '0') * 0.1) +
								              (line [2] - '0') * 0.01);
				}
				else
				{
					gboolean scroll;

					/* Report line */
					
					gtk_entry_set_text (GTK_ENTRY(report),
							    line + 4);
					
					if (vadj->value >= vadj->upper - vadj->page_size)
						scroll = TRUE;
					else
						scroll = FALSE;
						
					report_text[1] = line + 4;
					gtk_clist_append (GTK_CLIST(report_list),
							  report_text);
					
					if (scroll)
					{
						gtk_adjustment_set_value (vadj,
									  vadj->upper - vadj->page_size);
					}
				}
			}

			line [0] = '\0';
			line_len = 0;
		}
		else
		{
			/* Add character to end of current line */
			
			line = realloc (line, line_len + 2);
			line [line_len] = c;
			line_len++;
			line [line_len] = '\0';
		}
	}
}


static void
report_progress (int fd, gchar *label)
{
	guint input_id;
	gint cb_id;

	timeout_done = FALSE;
	report_list_visible = FALSE;

	gtk_label_set_text (GTK_LABEL (tool_widget_get_common ("report_label")),
			    label);
	progress_folding_button_set_hidden ();
	gtk_progress_set_percentage (GTK_PROGRESS (tool_widget_get_common ("report_progress")),
				     0.0);
	
	input_id = gtk_input_add_full (fd, GDK_INPUT_READ, report_progress_tick,
				       NULL, NULL, NULL);

	gtk_widget_show (tool_widget_get_common ("report_window"));
	
	gtk_main ();

	gtk_input_remove (input_id);

	/* Set progress to 100% and wait a bit before closing display */
  
	gtk_progress_set_percentage (GTK_PROGRESS (tool_widget_get_common ("report_progress")),
				     1.0);
	cb_id = gtk_timeout_add (3000, (GtkFunction) timeout_cb, NULL);
	gtk_main ();
	gtk_timeout_remove (cb_id);
	
	gtk_widget_hide (tool_widget_get_common ("report_window"));
	gtk_clist_clear (GTK_CLIST (tool_widget_get_common ("report_list")));
}


/* --- Exported interface --- */


gboolean
tool_config_load (void)
{
	ToolContext *tc;
	int fd [2];
	int t, len;
	char *p;
	/* char *argv [] = { 0, "--get", "-v", 0 }; */
	char *argv [] = { 0, "--get", "--progress", "--report", 0 };
	gchar *path;

	tc = tool_context;
	g_assert (tc->task);
  
	xmlSubstituteEntitiesDefault (TRUE);

	pipe (fd);

	t = fork ();
	if (t < 0)
	{
		g_error ("Unable to fork.");
	}
	else if (t)
	{
		/* Parent */

		close (fd [1]);	/* Close writing end */

		/* LibXML support for parsing from memory is good, but parsing from
		 * opened filehandles is not supported unless you write your own feed
		 * mechanism. Let's just load it all into memory, then. Also, refusing
		 * enormous documents can be considered a plus. </dystopic> */

		report_progress (fd [0], _("Scanning your system configuration."));

		p = malloc (102400);
		fcntl(fd [0], F_SETFL, 0);  /* Let's block */
		for (len = 0; (t = read (fd [0], p + len, 102399 - len)); )
		  len += t;

		if (len < 1 || len == 102399)
		{
			free (p);
			g_error ("Backend malfunction.");
		}

		p = realloc (p, len + 1);
		*(p + len) = 0;

		tc->config = xmlParseDoc (p);
		free (p);
		close (fd [0]);
	}
	else
	{
		/* Child */

		close (fd [0]);	/* Close reading end */
		dup2 (fd [1], STDOUT_FILENO);

		argv [0] = make_script_name (tc->task);
		path = make_script_path (tc->task);
		execve (path, argv, __environ);
		g_error ("Unable to run backend: %s", path);
	}

	if (tc->config) return TRUE;

	return FALSE;
}


gboolean
tool_config_save ()
{
	ToolContext *tc;
	FILE *f;
	int fd_xml [2], fd_report [2];
	int t;
	/* char *argv [] = { 0, "--filter", "-v", 0 }; */
	char *argv [] = { 0, "--set", "--progress", "--report", 0 };
	gchar *path;

	tc = tool_context;
	g_assert (tc->task);
	g_assert (tc->config);

	if (tc->to_xml)
		tc->to_xml (xml_doc_get_root (tc->config));

	/* don't actually save if we are just pretending */
	if (getenv ("SET_ME_UP_HARDER")) {
		g_warning (_("Skipping actual save..."));
		return TRUE;
	}

	pipe (fd_xml);
	pipe (fd_report);

	t = fork ();
	if (t < 0)
	{
		g_error ("Unable to fork.");
	}
	else if (t)
	{
		/* Parent */

		close (fd_xml [0]);	/* Close reading end of XML pipe */
		close (fd_report [1]);  /* Close writing end of report pipe */
		f = fdopen (fd_xml [1], "w");

		xmlDocDump (f, tc->config);
		fclose (f);
		report_progress (fd_report [0], _("Updating your system configuration."));
		close (fd_report [0]);
	}
	else
	{
		/* Child */

		close (fd_xml [1]);	/* Close writing end of XML pipe */
		close (fd_report [0]);  /* Close reading end of report pipe */
		dup2 (fd_xml [0], STDIN_FILENO);
		dup2 (fd_report [1], STDOUT_FILENO);

		argv [0] = make_script_name (tc->task);
		path = make_script_path (tc->task);
		execve (path, argv, __environ);
		g_error ("Unable to run backend: %s", path);
	}
  
	return TRUE;  /* FIXME: Determine if it really worked. */
}


xmlDocPtr
tool_config_get_xml ()
{
	return (tool_context->config);
}


void
tool_config_set_xml (xmlDocPtr xml)
{
	tool_context->config = xml;
}


gboolean
tool_get_frozen ()
{
	return (tool_context->frozen);
}


void
tool_set_frozen (gboolean state)
{
	tool_context->frozen = state;
}


gboolean tool_get_modified()
{
	return (tool_context->modified);
}


void
tool_set_modified (gboolean state)
{
	if (tool_get_frozen() || !tool_get_access())
		return;

	gtk_widget_set_sensitive (tool_widget_get_common ("apply"), state);

	tool_context->modified = state;
}


gboolean
tool_get_access ()
{
	return (tool_context->access);
}


void
tool_set_access (gboolean state)
{
	GtkWidget *w0;

	if (!state)
	{
		w0 = tool_widget_get_common ("apply");
		if (w0) gtk_widget_set_sensitive (w0, FALSE);
	}

	tool_context->access = state;
}


ToolComplexity
tool_get_complexity ()
{
	return (tool_context->complexity);
}

void
tool_set_complexity (ToolComplexity complexity)
{
	GtkWidget *button, *label;
	gchar *txt = NULL;
	
	tool_context->complexity = complexity;

	/* TODO: Invoke callbacks that update the interface to match
	 * the new complexity level. */
	
	button = tool_widget_get_common ("complexity");
	label = GTK_BIN (button)->child;
	
	switch (complexity)
	{
	 case TOOL_COMPLEXITY_BASIC:
		/* Translation: respect the spaces before and after, and the minus than signs. */
		txt = _(" More Options >> ");
		break;
	 case TOOL_COMPLEXITY_INTERMEDIATE:
		txt = _(" Full Options >> ");
		break;
	 case TOOL_COMPLEXITY_ADVANCED:
		txt = _(" << Fewer Options ");
		break;
	 default:
		g_warning ("Unexpected complexity level.");
	}
	
	gtk_label_set_text (GTK_LABEL (label), txt);

	if (tool_context->complexity_func)
		tool_context->complexity_func (complexity);
}

void
tool_set_complexity_func (UpdateComplexityFunc func)
{
	g_return_if_fail (tool_context != NULL);
	g_return_if_fail (func != NULL);

	tool_context->complexity_func = func;
	gtk_widget_set_sensitive (tool_widget_get_common ("complexity"), func != NULL);
}

void
tool_set_xml_funcs (ToolXMLFunc to_gui, ToolXMLFunc to_xml)
{
	g_return_if_fail (tool_context != NULL);
	g_return_if_fail (to_gui != NULL);
	g_return_if_fail (to_xml != NULL);

	tool_context->to_gui = to_gui;
	tool_context->to_xml = to_xml;
}

static void 
tool_set_initial_complexity (void)
{
	/* FIXME: we need to remember (gnome-config) the last complexity set. */
	tool_set_complexity (TOOL_COMPLEXITY_BASIC);
}

GtkWidget *
tool_get_top_window ()
{
	return (tool_context->top_window);
}


void
tool_modified_cb ()
{
	tool_set_modified (TRUE);
}

#if 0

static void
handle_events_immediately ()
{
	while (gtk_events_pending ()) gtk_main_iteration ();
	usleep(100000);
	while (gtk_events_pending ()) gtk_main_iteration ();
}


void
tool_splash_show ()
{
	GtkWidget *w0;
  
	/* FIXME: Keep track of created GnomePixmap, to avoid duplicates if
	 * splash is shown multiple times. */

/*	w0 = gnome_pixmap_new_from_xpm_d (reading_xpm);

	gtk_box_pack_end (GTK_BOX (tool_widget_get_common ("reading_box")),
			  w0, TRUE, TRUE, 0);*/

	w0 = tool_widget_get_common ("reading");
	gtk_widget_show (w0);

	handle_events_immediately ();
}


void
tool_splash_hide ()
{
	GtkWidget *w0;

	w0 = tool_widget_get_common ("reading");
	gtk_widget_hide (w0);
}

#endif

static int reply;


static void
reply_cb (gint val, gpointer data)
{
	reply = val;
	gtk_main_quit ();
}

void
tool_user_help (GtkWidget *w, gpointer null)
{
	GnomeHelpMenuEntry help_entry = { NULL, "index.html" };
	help_entry.name = g_strconcat (tool_context->task, "-admin", NULL);

	gnome_help_display (NULL, &help_entry);

	g_free (help_entry.name);
}

void
tool_user_complexity (GtkWidget *w, gpointer null)
{
	switch (tool_get_complexity ()) {
	case TOOL_COMPLEXITY_BASIC:
		tool_set_complexity (TOOL_COMPLEXITY_ADVANCED);
		break;
	case TOOL_COMPLEXITY_ADVANCED:
		tool_set_complexity (TOOL_COMPLEXITY_BASIC);
		break;
	default:
		break;
	}
}

void
tool_user_apply (GtkWidget *w, gpointer null)
{
	tool_config_save ();
	tool_set_modified (FALSE);
}

void
tool_user_close (GtkWidget *widget, gpointer data)
{
	if (tool_get_modified ())
	{
		/* Changes have been made. */
		GtkWidget *w;
		
		w = gnome_question_dialog_parented (
			_("There are changes which haven't been applyed.\nApply now?"),
			reply_cb, NULL, GTK_WINDOW (tool_context->top_window));

		gnome_dialog_run (GNOME_DIALOG (w));
		
		if (!reply)
			tool_config_save();
	}

	gtk_main_quit ();
}

gint
tool_user_delete (GtkWidget *w, GdkEvent *e, gpointer null)
{
	tool_user_close (w, null);
	return FALSE;
}

ToolContext *
tool_init (gchar *task, int argc, char *argv [])
{
	ToolContext *tc;
	gchar *s, *s1;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	s = g_strconcat (task, "_admin", NULL);
	gnome_init (s, VERSION, argc, argv);

	glade_gnome_init ();
	tc = tool_context_new (task);

	if (geteuid () == 0)
		tool_set_access (TRUE);
	else if (getenv ("SET_ME_UP_HARDER"))
	{
		g_warning (_("Pretending we are root..."));
		tool_set_access (TRUE);
	}
	else
	{
		gnome_ok_cancel_dialog (_("You need full administration privileges (i.e. root)\n"
					  "to run this configuration tool. You can acquire\n"
					  "such privileges by running it from the GNOME control\n"
					  "center or issuing an \"su\" command in a shell.\n\n"
					  "You will be unable to make any changes. Continue anyway?"), reply_cb, NULL);

		gtk_main ();

		if (reply) exit(1);

		tool_set_access (FALSE);
	}

	/* tool_splash_show (); */
	tool_config_load ();
	/* tool_splash_hide (); */

	/* Make sure apply and complexity start out as insensitive */

	gtk_widget_set_sensitive (tool_widget_get_common ("apply"), FALSE);
	gtk_widget_set_sensitive (tool_widget_get_common ("complexity"), FALSE);

	s1 = g_strconcat (task, "-admin", NULL);
	s = gnome_help_file_find_file (s1, "index.html");

	gtk_widget_set_sensitive (tool_widget_get_common ("help"),
				  s && g_file_exists (s));

	g_free (s);
	g_free (s1);

	/*
	 * NOTE: This is temporary, until we have more complexity levels
	 * for at least one tool
	*/

	tool_set_initial_complexity ();

	return (tc);
}
