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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>

#include <gnome.h>

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

static GtkWidget *win_wd;		/* main window */
static char *exec_p, *user_p;		/* command to execute, user name */

static int root;			/* if we are root, no password is
                                           required */

static GtkSignalFunc win_wd_destroy (GtkWidget *, gpointer);

static void error_box
(const char *str)
{
	GtkWidget *e_wd, *vb_wd, *hb_wd, *lb_wd, *bt_wd;

	e_wd = gtk_window_new (GTK_WINDOW_DIALOG);
	gtk_container_set_border_width (GTK_CONTAINER(e_wd), 5);
	gtk_window_set_modal (GTK_WINDOW(e_wd), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW(e_wd), GTK_WINDOW(win_wd));
	gtk_window_set_title (GTK_WINDOW(e_wd), _("Error"));
	gtk_widget_show (e_wd);

	gtk_signal_connect (GTK_OBJECT (e_wd), "delete_event",
			    GTK_SIGNAL_FUNC (win_wd_destroy), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT (e_wd), "destroy",
			   GTK_SIGNAL_FUNC (win_wd_destroy), (gpointer) NULL);

	vb_wd = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (vb_wd);
	gtk_container_add (GTK_CONTAINER (e_wd), vb_wd);

	lb_wd = gtk_label_new (str);
	gtk_label_set_line_wrap (GTK_LABEL (lb_wd), TRUE);
	gtk_widget_show (lb_wd);
	gtk_box_pack_start (GTK_BOX (vb_wd), lb_wd, TRUE, TRUE, 0);

	hb_wd = gtk_hbox_new (TRUE, 20);
	gtk_widget_show (hb_wd);
	gtk_container_add (GTK_CONTAINER (vb_wd), hb_wd);

	bt_wd = gtk_button_new_with_label (_("OK"));
	gtk_window_set_focus (GTK_WINDOW(e_wd), bt_wd);
	gtk_widget_show (bt_wd);
	gtk_box_pack_start (GTK_BOX(hb_wd), bt_wd, TRUE, TRUE, 100);
	gtk_signal_connect (GTK_OBJECT(bt_wd), "clicked",
			    GTK_SIGNAL_FUNC(win_wd_destroy), (gpointer) NULL);
}

static GtkSignalFunc
exec_su (GtkWidget *w_wd, gpointer pwd_entry)
{
	register char *pwd = NULL;
	pid_t pid;
	int t_fd;

	if (root == 0) {
		pwd = gtk_entry_get_text (GTK_ENTRY(pwd_entry));
		if ((pwd == NULL) || (*pwd == '\0'))
			return 0;
	}

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
			error_box (_("Incorrect password."));
			return 0;
		}
		else {
			if (root == 0)
				gtk_main_quit();
			_exit(0);
		}
	}
	else {				/* child process */
		struct passwd *pw;
		char *env, *home;

		/*
		 * We're the child, so we don't want or need gtk, so quit that.
		 * Also, su will use stderr to print error messages that we
		 * want, so redirect those to our pipe.
		 */

		if (root == 0)
			gtk_main_quit();

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

static GtkSignalFunc
win_wd_destroy (GtkWidget *wd, gpointer ptr)
{
	gtk_main_quit ();
	exit (0);

	return 0;
}

static GtkSignalFunc
cancel_cb (GtkWidget *w, gpointer data)
{
	gtk_main_quit ();
	return TRUE;
}

void
xst_su_run (gchar *exec_path, gchar *user, gchar *prompt)
{
	GtkWidget *vb_wd, *vbt_wd, *hb_wd, *lb_wd, *bt_wd,
		  *ca_wd, *sep_wd, *hb2_wd, *ed_wd, *sep2_wd,
		  *lb2_wd;
	char *p_str;

	g_assert (exec_path);
	g_assert (prompt);

	if (asprintf (&exec_p, "%s&", exec_path) < 0) {
		perror ("Unable to allocate memory chunk");
		return;
	}

	user_p = (user ? user : "root");

#if 0
	/* If we're already the user we want to be, there's no reason to
	 * go through and try and get the password.
	 */
	if ((root = (getuid() == 0)) == 0) {
		char *user;

		user = getenv("USER");
		if ((user != NULL) && (strcmp(user, user_p) == 0)) {
			/* We're already user_p, so we can just execute the
			 * program they're trying to run.
			 */
			execlp("sh", "sh", "-c", argv[1], NULL);
		}
	}

	if (root != 0) {
		exec_su(NULL, NULL);
		_exit(-1);			/* NOTREACHED */
	}
#endif

	win_wd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(win_wd), 5);
	gtk_window_set_title(GTK_WINDOW(win_wd), _("Access Verification"));
	gtk_window_set_policy (GTK_WINDOW (win_wd), FALSE, FALSE, TRUE);
	gtk_widget_show(win_wd);

	gtk_signal_connect(GTK_OBJECT(win_wd), "delete_event",
			GTK_SIGNAL_FUNC(win_wd_destroy), (gpointer) NULL);
	gtk_signal_connect(GTK_OBJECT(win_wd), "destroy",
			GTK_SIGNAL_FUNC(win_wd_destroy), (gpointer) NULL);

	vb_wd = gtk_vbox_new(FALSE, 4);
	gtk_container_add(GTK_CONTAINER(win_wd), vb_wd);
	gtk_widget_show(vb_wd);

	if (asprintf (&p_str, prompt) < 0) {
		perror("Unable to allocate memory chunk");
		return;
	}

	lb_wd = gtk_label_new(p_str);
	free(p_str);
	gtk_widget_show(lb_wd);
	gtk_box_pack_start(GTK_BOX(vb_wd), lb_wd, TRUE, TRUE, 5);

	hb_wd = gtk_hbox_new(TRUE, 10);
	gtk_widget_show(hb_wd);
	gtk_container_add(GTK_CONTAINER(vb_wd), hb_wd);

	ed_wd = gtk_entry_new_with_max_length(255);
	gtk_entry_set_visibility(GTK_ENTRY(ed_wd), FALSE);
	gtk_widget_show(ed_wd);
	gtk_box_pack_start(GTK_BOX(hb_wd), ed_wd, FALSE, TRUE, 30);
	gtk_window_set_focus(GTK_WINDOW(win_wd), ed_wd);
	gtk_signal_connect(GTK_OBJECT(ed_wd), "activate",
			   GTK_SIGNAL_FUNC(exec_su), (gpointer) ed_wd);

	vbt_wd = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbt_wd);
	gtk_box_pack_end(GTK_BOX(vb_wd), vbt_wd, TRUE, TRUE, 0);

	hb2_wd = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(hb2_wd);
	gtk_box_pack_end(GTK_BOX(vb_wd), hb2_wd, TRUE, TRUE, 0);

	sep_wd = gtk_hseparator_new();
	gtk_widget_show(sep_wd);
	gtk_box_pack_start(GTK_BOX(vb_wd), sep_wd, TRUE, TRUE, 10);

	if (asprintf(&p_str, "%s: %s", _("Run"), exec_path) < 0) {
		perror("Unable to allocate memory chunk");
		return;
	}

	lb2_wd = gtk_label_new(p_str);
	free(p_str);
	gtk_widget_show(lb2_wd);
	gtk_box_pack_start(GTK_BOX(vb_wd), lb2_wd, TRUE, TRUE, 0);

	sep2_wd = gtk_hseparator_new();
	gtk_widget_show(sep2_wd);
	gtk_box_pack_start(GTK_BOX(vb_wd), sep2_wd, TRUE, TRUE, 10);

	bt_wd = gtk_button_new_with_label(_("Run unprivileged"));
	gtk_widget_show(bt_wd);
	gtk_box_pack_start(GTK_BOX(hb2_wd), bt_wd, FALSE, TRUE, 30);
	gtk_signal_connect(GTK_OBJECT(bt_wd), "clicked",
			   GTK_SIGNAL_FUNC(cancel_cb), (gpointer) NULL);

	ca_wd = gtk_button_new_with_label(_("Authenticate"));
	gtk_widget_show(ca_wd);
	gtk_box_pack_start(GTK_BOX(hb2_wd), ca_wd, FALSE, TRUE, 30);
	gtk_signal_connect(GTK_OBJECT(ca_wd), "clicked",
			   GTK_SIGNAL_FUNC(exec_su), (gpointer) ed_wd);

	gtk_main();

	gtk_signal_disconnect_by_func (GTK_OBJECT (win_wd), win_wd_destroy, NULL);
	gtk_widget_destroy (win_wd);
}
