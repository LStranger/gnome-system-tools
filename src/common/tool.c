/* Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Hans Petter Jansson <hpj@helixcode.com>.
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
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "tool.h"
#include "xml.h"

#include "reading.xpm"


/* --- Internals --- */


ToolContext *tool_context = NULL;


static gchar *
make_script_name (char *task)
{
	gchar *name;
  
	name = g_strjoin ("-", task, "conf", NULL);
	return(name);
}


static gchar *
make_script_path (char *task)
{
	gchar *name, *path;

	name = make_script_name (task);
	path = g_strjoin ("/", SCRIPTS_DIR, name, NULL);
	g_free (name);
	return (path);
}


static gchar *
make_glade_path (char *task)
{
	gchar *path0, *path1;

	path0 = g_strjoin ("/", INTERFACES_DIR, task, NULL);
	path1 = g_strjoin (".", path0, "glade", NULL);
	g_free (path0);
	return (path1);
}


GdkPixbuf *
tool_load_image (char *image_name)
{
	GdkPixbuf *pixbuf;
	gchar *path;

	path = g_strjoin ("/", PIXMAPS_DIR, image_name, NULL);
	pixbuf = gdk_pixbuf_new_from_file (path);
	if (!pixbuf) g_warning ("Couldn't load image \"%s\".", path);
	g_free (path);
	return pixbuf;
}


GtkWidget *tool_widget_get(gchar *name)
{
	return(glade_xml_get_widget (tool_context->interface, name));
}


GtkWidget *tool_widget_get_common(gchar *name)
{
	return(glade_xml_get_widget (tool_context->common_interface, name));
}


static gboolean tool_interface_load()
{
	gchar *path;
	
	/* Load interface from Glade file */

	path = make_glade_path(tool_context->task);
	tool_context->interface = glade_xml_new(path, NULL);

	/* Connect signals */

	if (tool_context->interface)
	        glade_xml_signal_autoconnect(tool_context->interface);
	else
	        g_error("Could not load tool interface from %s", path);
	g_free(path);

	/* Load common interface elements from Glade file */

	path = make_glade_path("common");
	tool_context->common_interface = glade_xml_new(path, NULL);
	if (!tool_context->common_interface)
	  g_error("Could not load common interface elements from %s", path);
	g_free(path);

	/* Locate and save a pointer to the top level window */

	path = g_strjoin("_", tool_context->task, "admin", NULL);
	tool_context->top_window = tool_widget_get(path);
	g_free(path);

	if (!tool_context->top_window)
	        g_error ("Undefined toplevel window in Glade file.");
	else
	        return TRUE;
	 
	return FALSE;
}


static ToolContext *tool_context_new(gchar *task)
{
	ToolContext *tc;
  
	tc = g_new (ToolContext, 1);
	memset(tc, 0, sizeof (*tc));
	tc->task = g_strdup (task);

	tool_context = tc;
	tool_interface_load (tc);
	tool_set_modified (FALSE);
	return(tc);
}


void tool_context_destroy(ToolContext *tc)
{
	g_assert(tc->task);

	g_free(tc->task);
	/* TODO: Free interface */
	/* TODO: Free config */
	g_free(tc);
}


static void read_progress_tick(gpointer data, gint fd, GdkInputCondition cond)
{
	char c;
	GtkWidget *bar, *report;
	gfloat p;
	static char *line = NULL;
	static int line_len = 0;

	bar = tool_widget_get_common("read_progress");
	report = tool_widget_get_common("read_report");
	if (!line)
	{
		line = malloc(1);
		line[0] = '\0';
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
				if (line[0] == '0')
				{
					/* Progress update */

					gtk_progress_set_percentage (GTK_PROGRESS(bar),
								     (gfloat) ((line[1] - '0') * 0.1) +
								              (line[2] - '0') * 0.01);
				}
				else
				{
					/* Report line */
					
					gtk_entry_set_text(GTK_ENTRY(report),
							   line + 4);
				}
			}

			line[0] = '\0';
			line_len = 0;
		}
		else
		{
			/* Add character to end of current line */
			
			line = realloc(line, line_len + 2);
			line[line_len] = c;
			line_len++;
			line[line_len] = '\0';
		}
	}
#if 0  
	if (tool_context->read_state == TOOL_READ_PROGRESS_MAX)
	{
		if (c - '0' < 10 && c - '0' >= 0)
		{
			tool_context->progress_max *= 10;
			tool_context->progress_max += (c - '0');
		}
		else
		        tool_context->read_state = TOOL_READ_PROGRESS_DONE;
	}
	else if (c != '.')
	{
		/* Progressbar's death */

		gtk_main_quit ();
	}
	else
	{
		/* Update progressbar */
		
		tool_context->progress_done += 1;
		
		if (!tool_context->progress_max) p = 0.0;
		else p = (gfloat) tool_context->progress_done / tool_context->progress_max;
		if (p > 1.0) p = 1.0;
		
		gtk_progress_set_percentage (GTK_PROGRESS(bar), p);
	}
#endif
}


static void read_progress(int fd)
{
	guint input_id;
	gint cb_id;

	tool_context->progress_max = 0;
	tool_context->read_state = TOOL_READ_PROGRESS_MAX;
	input_id = gtk_input_add_full (fd, GDK_INPUT_READ, read_progress_tick,
				       NULL, NULL, NULL);
	gtk_main ();

	gtk_input_remove (input_id);

	/* Set progress to 100% and wait a bit before closing display */
  
	gtk_progress_set_percentage (GTK_PROGRESS (tool_widget_get_common ("read_progress")),
				     1.0);
	cb_id = gtk_timeout_add (500, (GtkFunction) gtk_main_quit, NULL);
	gtk_main ();
	gtk_timeout_remove (cb_id);
}


/* --- Exported interface --- */


gboolean tool_config_load()
{
	ToolContext *tc;
	int fd[2];
	int t, len;
	char *p;
	/* char *argv[] = { 0, "--get", "-v", 0 }; */
	char *argv[] = { 0, "--get", "--progress", "--report", 0 };
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

		close (fd[1]);	/* Close writing end */

		/* LibXML support for parsing from memory is good, but parsing from
		 * opened filehandles is not supported unless you write your own feed
		 * mechanism. Let's just load it all into memory, then. Also, refusing
		 * enormous documents can be considered a plus. </dystopic> */

		read_progress (fd[0]);

		p = malloc (102400);
		fcntl(fd[0], F_SETFL, 0);  /* Let's block */
		for (len = 0; (t = read (fd[0], p + len, 102399 - len)); )
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
		close (fd[0]);
	}
	else
	{
		/* Child */

		close (fd[0]);	/* Close reading end */
		dup2 (fd[1], STDOUT_FILENO);

		argv[0] = make_script_name (tc->task);
		path = make_script_path (tc->task);
		execve (path, argv, __environ);
		g_error ("Unable to run backend: %s", path);
	}

	if (tc->config) return TRUE;

	return FALSE;
}


gboolean tool_config_save()
{
	ToolContext *tc;
	FILE *f;
	int fd[2];
	int t;
	/* char *argv[] = { 0, "--filter", "-v", 0 }; */
	char *argv[] = { 0, "--set", 0 };
	gchar *path;

	tc = tool_context;
	g_assert (tc->task);
	g_assert (tc->config);

	pipe (fd);

	t = fork ();
	if (t < 0)
	{
		g_error ("Unable to fork.");
	}
	else if (t)
	{
		/* Parent */

		close (fd[0]);	/* Close reading end */
		f = fdopen (fd[1], "w");

		xmlDocDump (f, tc->config);
		fclose (f);
	}
	else
	{
		/* Child */

		close (fd[1]);	/* Close writing end */
		dup2 (fd[0], STDIN_FILENO);

		argv[0] = make_script_name (tc->task);
		path = make_script_path (tc->task);
		execve (path, argv, __environ);
		g_error ("Unable to run backend: %s", path);
	}
  
	return TRUE;  /* FIXME: Determine if it really worked. */
}


xmlDocPtr tool_config_get_xml()
{
	return (tool_context->config);
}


void tool_config_set_xml(xmlDocPtr xml)
{
	tool_context->config = xml;
}


gboolean tool_get_frozen()
{
	return (tool_context->frozen);
}


void tool_set_frozen(gboolean state)
{
	tool_context->frozen = state;
}


gboolean tool_get_modified()
{
	return (tool_context->modified);
}


void tool_set_modified(gboolean state)
{
	GtkWidget *w0;

	if (tool_get_frozen() || !tool_get_access()) return;

	if (state)
	{
		w0 = tool_widget_get("apply");
		if (w0) gtk_widget_set_sensitive(w0, TRUE);
	}
	else
	{
		w0 = tool_widget_get("apply");
		if (w0) gtk_widget_set_sensitive(w0, FALSE);
	}

	tool_context->modified = state;
}


gboolean tool_get_access()
{
	return (tool_context->access);
}


void tool_set_access(gboolean state)
{
	GtkWidget *w0;

	if (!state)
	{
		w0 = tool_widget_get ("apply");
		if (w0) gtk_widget_set_sensitive (w0, FALSE);
	}

	tool_context->access = state;
}


ToolComplexity tool_get_complexity()
{
	return (tool_context->complexity);
}


void tool_set_complexity(ToolComplexity complexity)
{
	tool_context->complexity = complexity;

	/* TODO: Invoke callbacks that update the interface to match
	 * the new complexity level. */
}


GtkWidget *tool_get_top_window()
{
	return (tool_context->top_window);
}


void tool_modified_cb()
{
	tool_set_modified (TRUE);
}


static void
handle_events_immediately()
{
	while (gtk_events_pending ()) gtk_main_iteration ();
	usleep(100000);
	while (gtk_events_pending ()) gtk_main_iteration ();
}


void tool_splash_show()
{
	GtkWidget *w0;
  
	/* FIXME: Keep track of created GnomePixmap, to avoid duplicates if
	 * splash is shown multiple times. */

	w0 = gnome_pixmap_new_from_xpm_d (reading_xpm);

	gtk_box_pack_end (GTK_BOX (tool_widget_get_common("reading_box")),
			  w0, TRUE, TRUE, 0);

	w0 = tool_widget_get_common ("reading");
	gtk_widget_show_all (w0);

	handle_events_immediately ();
}


void tool_splash_hide()
{
	GtkWidget *w0;

	w0 = tool_widget_get_common("reading");
	gtk_widget_hide (w0);
}


static int reply;


static void reply_cb(gint val, gpointer data)
{
	reply = val;
	gtk_main_quit ();
}


void tool_user_close(GtkWidget *widget, gpointer data)
{
	/* TODO: Check for changes and optionally ask for confirmation */
	gtk_main_quit ();
}


ToolContext *tool_init(gchar *task, int argc, char *argv[])
{
	ToolContext *tc;
	GtkWidget *w0;
	gchar *s;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
#endif

	s = g_strjoin ("_", task, "admin", NULL);
	gnome_init (s, VERSION, argc, argv);

	glade_gnome_init ();
	tc = tool_context_new (task);

	if (geteuid () != 0)
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
	else
	        tool_set_access (TRUE);

	tool_splash_show ();
	tool_config_load ();
	tool_splash_hide ();

	/* Connect the close and delete signals to generic handlers */

	gtk_signal_connect (GTK_OBJECT (tool_widget_get ("close")),
			    "clicked", tool_user_close, NULL);

	gtk_signal_connect (GTK_OBJECT (tool_get_top_window ()),
			    "delete_event", tool_user_close, NULL);

	/* Make sure apply starts out as insensitive */

	w0 = tool_widget_get ("apply");
	if (w0) gtk_widget_set_sensitive (w0, FALSE);
	
	/* Desensitize the complexity button.
	 * 
	 * NOTE: This is temporary, until we have more complexity levels
	 * for at least one tool */
	
	w0 = tool_widget_get ("complexity");
	if (w0) gtk_widget_set_sensitive (w0, FALSE);

	g_free (s);
	return (tc);
}
