/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Chris L. Bond
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
 * Authors: Chris L. Bond       <cbond@stormix.com>  gtksu 1.7
 *          Hans Petter Jansson <hpj@ximian.com>     Minor XST adaptions
 */

#define _GNU_SOURCE

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <gnome.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <vte/vte.h>
#include <vte/reaper.h>

#ifdef __FreeBSD__
# include <errno.h>
# include <libutil.h>
#endif

#include "xst-su.h"

/* ABORT() kills GTK if we're not root, else it just exits.
 */
#define ABORT(root)			\
	        if (root == 0)		\
			GTK_ABORT();	\
		else			\
			_exit(-1)

/* GTK_ABORT() is supposed to kill GTK and exit.
 */
#define GTK_ABORT() do {			\
			gtk_main_quit();	\
			_exit(0);		\
		    } while (0)

#define XST_SU_RESPONSE_NP 1

GtkWidget *term;
static int root;			/* if we are root, no password is
					   required */

/* This is the signal callback that answers to the forked su command */
static void
on_terminal_child_exited (GtkWidget *reaper, gint pid, gint status, gpointer data)
{
	GtkWidget *error_dialog;
	
	if (WIFEXITED (status) && WEXITSTATUS (status) && (WEXITSTATUS(status) < 255)) {
		error_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
						       GTK_MESSAGE_ERROR,
						       GTK_BUTTONS_OK,
						       _("The password you entered is invalid."));

		gtk_dialog_run (GTK_DIALOG (error_dialog));
		gtk_widget_destroy (error_dialog);
	}

	_exit (0);
}

/* finds out the path in which the tool is */
static gchar*
get_tool_path (gchar *argv0)
{
	gchar *program_name = g_path_get_basename (argv0);

	if (strcmp (argv0, program_name) == 0) {
		/* we are calling the tool without any path, because root user may not have
		 * the tool's path in his $PATH, we find out the absolute path
		 */
		g_free (program_name);

		return g_find_program_in_path (argv0);
	} else {
		/* if argv[0] is not equal to the tool basename, then it's already
		 * an absolute of relative path, and it doesn't need the full path to run
		 */
		g_free (program_name);

		return argv0;
	}
}

/* runs a term with su in it */
void
xst_su_run_term (gint argc, gchar *argv[], gchar *user)
{
	GString *str;
	VteReaper *reaper = vte_reaper_get ();
	gchar *args[5], *string;
	int i;
	int pid, status;

	term = vte_terminal_new ();
	g_signal_connect (G_OBJECT (reaper), "child-exited",
			  G_CALLBACK (on_terminal_child_exited), NULL);

	g_assert (argv && argv[0]);

	str = g_string_new (get_tool_path (argv[0]));
	for (i = 1; i < argc; i++) {
		g_string_append_c (str, ' ');
		g_string_append (str, argv[i]);
	}

	args[0] = g_strdup (SU_PATH);
	args[1] = (user ? user : "root");
	args[2] = g_strdup ("-c");
	args[3] = str->str;
	args[4] = NULL;

	g_string_free (str, 0);

	vte_terminal_fork_command (VTE_TERMINAL (term),
					 args[0],
					 args,
					 NULL,
					 NULL,
					 FALSE, FALSE, FALSE);
}

/* writes the password to the term with su in it */
void
xst_su_write_password (gchar *pwd)
{
	vte_terminal_feed_child (VTE_TERMINAL (term), pwd, -1);

	memset (pwd, 0, strlen (pwd));

	/* when the user authenticates, the current proccess forks to launch again the tool
	 * with root uid, so this proccess is only needed to wait for its son, so it idles with gtk_main ()
	 */
	gtk_main ();
}

/* tries to clear the term (if user runs tool unprivileged) */
void
xst_su_clear_term (void)
{
	vte_terminal_reset (VTE_TERMINAL (term), TRUE, TRUE);
	gtk_widget_destroy (term);
}

static GladeXML *
load_glade_common (const gchar *widget)
{
	gchar *glade_common_path;
	GladeXML *xml;

	glade_common_path = g_strdup_printf ("%s/common.glade", INTERFACES_DIR);
	xml = glade_xml_new (glade_common_path, widget, NULL);
	g_free (glade_common_path);

	return xml;
}

gint
xst_su_get_password (gchar **password)
{
	GladeXML *xml;
	gint result;
	gchar *blank;
	GtkWidget *password_dialog, *password_entry;

	xml = load_glade_common ("password_dialog");
	password_dialog = glade_xml_get_widget (xml, "password_dialog");
	password_entry  = glade_xml_get_widget (xml, "password_entry");
	g_assert (password_entry);
	g_assert (password_dialog);

	result = gtk_dialog_run (GTK_DIALOG (password_dialog));

	/* get the password with a \n at the end */
	*password = g_strdup_printf ("%s\n", gtk_entry_get_text (GTK_ENTRY (password_entry)));

	/* Make a pathetic stab at clearing the GtkEntry field memory */
	blank = g_strdup (*password);
	if (strlen (blank))
		memset (blank, ' ', strlen (blank));

	gtk_entry_set_text (GTK_ENTRY (password_entry), blank);
	gtk_entry_set_text (GTK_ENTRY (password_entry), "");
	g_free (blank);

	gtk_widget_destroy (password_dialog);
	/* FIXME: I added this because if you click "OK", the dialog
	 * never goes away.  Is it really needed?
	 */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	switch (result) {
	case GTK_RESPONSE_OK:
		/* Run privileged with password */
		if (!*password)
			*password = g_strdup ("");
		return 1;
		break;
	case XST_SU_RESPONSE_NP:
		/* Run unprivileged */
		return 0;
		break;
	default:
		/* Cancel */
		return -1;
		break;
	}

	g_assert ("Not reached");
}
