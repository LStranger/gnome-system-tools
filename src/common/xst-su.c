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
#include <glade/glade.h>

#include "xst-su.h"

#warning FIXME: exec_su and related code needs a bit more cleanup

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

/* OPEN_TTY() is supposed to return a file descriptor to a pseudo-terminal.
 */
#define OPEN_TTY() getpt()

static int root;			/* if we are root, no password is
                                           required */

static gint
exec_su (gchar *exec_path, gchar *user, gchar *pwd)
{
	gchar *exec_p, *user_p;  /* command to execute, user name */
	pid_t pid;
	int t_fd;

	exec_p = g_strdup (exec_path);

#if 0
	if (asprintf (&exec_p, "%s&", exec_path) < 0) {
		perror ("Unable to allocate memory chunk");
		return 0;
	}
#endif

	user_p = (user ? user : "root");

	if ((pwd == NULL) || (*pwd == '\0'))
		return 0;

	/*
	 * Make su think we're sending the password from a terminal:
	 */

	if ((t_fd = OPEN_TTY()) < 0) {
		fprintf (stderr, "Unable to open a terminal\n");
		ABORT (root);
	}

	if ((pid = fork()) < 0) {
		perror ("Unable to fork a new process");
		ABORT (root);
	}

	if (pid > 0) {			/* parent process */
		int status;

		/* su(1) won't want a password if we're already root.
		 */
		if (root == 0)
			write (t_fd, pwd, strlen(pwd));

		waitpid (pid, &status, 0);

		if ((WIFEXITED(status)) && (WEXITSTATUS(status))) {
/*			error_box (_("Incorrect password.")); */
			return 0;
		}
		else {
			memset (pwd, 0, strlen (pwd));
			_exit (0);
		}
	}
	else {				/* child process */
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

		dup2 (t_fd, 0);
		freopen ("/dev/null", "w", stderr);
		freopen ("/dev/null", "w", stdout);

		execlp ("su", "su", "-c", exec_p, user_p, NULL);

		_exit (0);
	}

	return 0;
}

void
xst_su_run_with_password (gchar *exec_path, gchar *password)
{
	exec_su (exec_path, "root", password);
}

static GladeXML *
load_glade_common (const gchar *widget)
{
	gchar *glade_common_path;
	GladeXML *xml;

	glade_common_path = g_strdup_printf ("%s/common.glade", INTERFACES_DIR);
	xml = glade_xml_new (glade_common_path, widget);
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
	g_assert (password_dialog);
	g_assert (password_entry);

	gnome_dialog_editable_enters (GNOME_DIALOG (password_dialog), GTK_EDITABLE (password_entry));

	result = gnome_dialog_run_and_close (GNOME_DIALOG (password_dialog));
	if (result == 2 || result < 0)
		return -1;  /* Cancel */
	else if (result == 1)
		return 0;   /* Run unprivileged */

	*password = gtk_entry_get_text (GTK_ENTRY (password_entry));
	if (!*password)
		*password = g_strdup ("");

	/* Make a pathetic stab at clearing the GtkEntry field memory */

	blank = g_strdup (*password);
	if (strlen (blank))
		memset (blank, ' ', strlen (blank));

	gtk_entry_set_text (GTK_ENTRY (password_entry), blank);
	gtk_entry_set_text (GTK_ENTRY (password_entry), "");
	g_free (blank);

	return 1;  /* Run privileged with password */
}
