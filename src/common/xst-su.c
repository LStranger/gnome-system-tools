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
#include <pty.h>

#include <gnome.h>
#include <glade/glade.h>

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

static int root;			/* if we are root, no password is
					   required */

static gint
exec_su (int argc, char *argv[], gchar *user, gchar *pwd)
{
	gchar *exec_p, *user_p;  /* command to execute, user name */
	pid_t pid;
	int t_fd, i;
	GString *str;
	gint fd;

#if 0
	exec_p = g_strdup (exec_path);
#endif

	g_assert (argv && argv[0]);
	str = g_string_new (argv[0]);
	for (i = 1; i < argc; i++) {
		g_string_append_c (str, ' ');
		g_string_append (str, argv[i]);
	}

	exec_p = str->str;
	g_string_free (str, 0);

#if 0
	if (asprintf (&exec_p, "%s&", exec_path) < 0) {
		perror ("Unable to allocate memory chunk");
		return 0;
	}
#endif

	user_p = (user ? user : "root");

	if ((pwd == NULL) || (*pwd == '\0'))
		return 0;

	pid = forkpty (&fd, NULL, NULL, NULL);

	if (pid < 0) {
		perror ("unable to fork a new process\n");
		ABORT (root);
	}

	if (pid > 0) {
		/* This is the parent process */
		char *buf = g_malloc0 (20);
		int status;
		struct passwd *pw;
		char *env, *home;
		
		/* We have rights to run X (obviously).  We need to ensure the
		 * destination user has the right stuff in the environment
		 * to be able to continue to run X.
		 * su will change $HOME to the new users home, so we will
		 * need an XAUTHORITY / ICEAUTHORITY pointing to the
		 * authorization files.
		 */

		if ((home = getenv ("HOME")) == NULL) {
			if ((env = getenv ("USER")) == NULL)
				pw = getpwuid(getuid());
			else
				pw = getpwnam(env);
			if (pw)
				home = pw->pw_dir;
			else {
				perror ("Unable to find home directory");
				_exit (-1);
			}
		}

		if ((env = getenv ("XAUTHORITY")) == NULL) {
			if (asprintf (&env, "XAUTHORITY=%s/.Xauthority", home) > 0)
				putenv (env);
			else {
				perror ("Unable to allocate memory chunk");
				_exit (-1);
			}
		}

		if ((env = getenv ("ICEAUTHORITY")) == NULL) {
			if (asprintf (&env, "ICEAUTHORITY=%s/.ICEauthority", home) > 0)
				putenv (env);
			else {
				perror ("Unable to allocate memory chunk");
				_exit (-1);
			}
		}
		
		/* just read the password prompt, we don't really need it, but
		 * it's just to ensure that the password is sent after the prompt */
		read (fd, buf, 20);

		/* Send the password */
		write (fd, pwd, strlen (pwd));

		/* read all the trash from the file descriptor and clear buffer */
		while (read (fd, buf, 20) > 0);
		bzero (buf, 20);

		/* wait child process */
		waitpid (pid, &status, 0);
		
		if (WIFEXITED (status) && WEXITSTATUS (status) && (WEXITSTATUS(status) < 255)) {
			return 0;
		}
		else {
			memset (pwd, 0, strlen (pwd));
			_exit (0);
		}
	}
	else {
		/* This is the child process */
		execlp ("su", "su", user_p, "-c", exec_p, NULL);

		_exit (1);
	}

	return 0;
}

void
xst_su_run_with_password (int argc, char *argv[], gchar *password)
{
	exec_su (argc, argv, "root", password);
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
