/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro <garnacho@tuxerver.net>
 */

#define _GNU_SOURCE

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>

#include <gnome.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

#ifdef __FreeBSD__
# include <errno.h>
# include <libutil.h>
#else
#include <pty.h>
#endif

#include "gst-auth.h"
#include "gst-tool.h"
#include "gst-hig-dialog.h"

#define GST_AUTH_RESPONSE_NP 1

static int root;			/* if we are root, no password is
					   required */

static void
gst_auth_display_error_message (GstTool *tool, gchar *primary_text, gchar *secondary_text)
{
	GtkWidget *error_dialog;

	error_dialog = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					   GTK_DIALOG_MODAL,
					   GST_HIG_MESSAGE_ERROR,
					   primary_text,
					   secondary_text,
					   GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					   NULL);
	gtk_dialog_run (GTK_DIALOG (error_dialog));
	gtk_widget_destroy (error_dialog);
}

static gboolean
gst_auth_wait_child (GstTool *tool)
{
	gint status, pid;
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	gchar *auth_command;

	pid = waitpid (tool->backend_pid, &status, WNOHANG);

	if (pid > 0) {
		if ((WIFEXITED (status)) && (WEXITSTATUS (status)) && (WEXITSTATUS(status) < 255)) {
			if (tool->remote_config) {
				/* the proccess was running ssh */
				primary_text   = g_strdup (_("The tool could not connect to the computer"));
				secondary_text = g_strdup (_("Check that you have access to this network "
							     "and that the computer is actually working and running SSHD"));
			} else {
				/* the proccess was running su */
				primary_text   = g_strdup (_("The entered password is invalid"));
				secondary_text = g_strdup (_("Check that you typed it correctly "
							     "and that you haven't activated the \"caps lock\" key"));
			}
		} else if ((WIFEXITED (status)) && (WEXITSTATUS (status)) && (WEXITSTATUS (status) == 255)) {
			if (tool->remote_config)
				auth_command = "ssh";
			else
				auth_command = "su";

			primary_text   = g_strdup_printf (_("Could not run \"%s\""), auth_command);
			secondary_text = g_strdup (_("Check that you have permissions to run this command"));
		} else {
			primary_text   = g_strdup (_("An unexpected error has ocurred"));
			secondary_text = NULL;
		}

		gst_auth_display_error_message (tool, primary_text, secondary_text);

		g_free (primary_text);
		g_free (secondary_text);
		exit (0);
	}

	return TRUE;	
}

/* runs a term with su in it */
static void
gst_auth_run_term (GstTool *tool, gchar *args[])
{
	struct termios t;
	int p[2];

	pipe (p);
	tool->backend_pid = forkpty (&tool->write_fd, NULL, NULL, NULL);

	if (tool->backend_pid < 0) {
		g_warning ("could not fork to backend");
	}
	else if (tool->backend_pid == 0) {
		/* It's the child process */
		/* We must set the locale to "C" to keep su/ssh strings untranslated */
		unsetenv("LC_ALL");
		unsetenv("LC_MESSAGES");
		unsetenv("LANG");
		unsetenv("LANGUAGE");

		dup2 (p[1], 1);
		dup2 (p[1], 2);
		close (p[0]);

		execv (args[0], args);
		exit (255);
	} else {
#ifndef __FreeBSD__
		/* Linux's su works ok with echo disabling */
/*		tcgetattr (tool->write_fd, &t);
		t.c_lflag ^= ECHO;
		tcsetattr (tool->write_fd, TCSANOW, &t);
*/
#endif
		close (p[1]);

		tool->read_fd      = p[0];
		tool->timeout_id   = g_timeout_add (1000, (GSourceFunc) gst_auth_wait_child, tool);
		tool->read_stream  = fdopen (tool->read_fd, "r");
		tool->write_stream = fdopen (tool->write_fd, "w");
		setvbuf (tool->read_stream, NULL, _IONBF, 0);
		fcntl (tool->read_fd, F_SETFL, 0);
	}
}

/* it guesses whether we have to type password or not */
static gint
gst_auth_get_auth_required (GstTool *tool)
{
	const gchar  *answer = "yes\n";
	gboolean      cont   = FALSE;
	gchar        *s, *str;
	gint          ret;

	while (!cont) {
		s   = gst_tool_read_from_backend (tool, "assword:", "/no)?", "\n", NULL);
		str = g_ascii_strup (s, -1);

		/* FIXME: hope that someday we can get rid of this ssh output string parsing */
		if (g_strrstr (str, "AUTHENTICITY") != NULL) {
			/* it's the "add to known hosts list" ssh's message, just answer "yes" */
			gst_tool_write_to_backend (tool, (gchar *) answer);
		} else if (g_strrstr (str, "PASSWORD") != NULL) {
			/* it's asking for the password */
			cont = TRUE;
			ret  = GST_AUTH_PASSWORD;
		} else if (g_strrstr (str, "\n") != NULL) {
			/* this is the last case to test, it's the CR
			   used to synchronize communication */
			cont = TRUE;
			ret  = GST_AUTH_PASSWORDLESS;
		}

		g_free (s);
		g_free (str);
	}

	return ret;
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

static void
gst_auth_get_password (gchar **password)
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

	gtk_widget_show (password_dialog);
	gtk_widget_grab_focus (password_entry);
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

	while (gtk_events_pending ())
		gtk_main_iteration ();

	switch (result) {
	case GTK_RESPONSE_OK:
		/* Run privileged with password */
		if (!*password)
			*password = g_strdup ("");
		break;
	default:
		/* Cancel */
		exit (0);
		break;
	}
}

/* this is done because we need to synchronize with the backend */
static void
gst_auth_read_output (GstTool *tool)
{
	gchar *b;

	/* read the synchrony CR after sending the password */
	b = gst_tool_read_from_backend (tool, "\n", NULL);
	g_free (b);
}

/* it does authentication, first of all it runs su (just to ensure that it's completely run
 * when password is sent), then asks for password and sends it to su (if you want to run it
 * with root privileges)
 */
static void
gst_auth_do_authentication (GstTool *tool, gchar *args[])
{
	gchar *password;
	gint result;
	struct termios t;

	gst_auth_run_term (tool, args);
	result = gst_auth_get_auth_required (tool);

	if (result == GST_AUTH_PASSWORD) {
		gst_auth_get_password (&password);
		gst_tool_write_to_backend (tool, password);

		if (strlen (password) > 0)
			memset (password, 0, strlen (password));
	}

	tool->root_access = ROOT_ACCESS_REAL;

#ifdef __FreeBSD__
	/* FreeBSD seems to have weird issues with su and echo disabling */
/*	tcgetattr (tool->write_fd, &t);
	t.c_lflag ^= ECHO;
	tcsetattr (tool->write_fd, TCSANOW, &t);
*/
#endif
}

static void
gst_auth_save_locale (GString* command) {
	char *lc_all, *lc_messages, *lang, *language;

	/* Save the current state of current locale to restore it for the backend later */
	lc_messages = getenv ("LC_MESSAGES");
	language    = getenv ("LANGUAGE");
	lc_all      = getenv ("LC_ALL");
	lang        = getenv ("LANG");

	g_string_append (command, "env ");

	if (lc_all)
		g_string_append_printf (command, "LC_ALL=\"%s\" ", lc_all);
	if (lc_messages)
		g_string_append_printf (command, "LC_MESSAGES=\"%s\" ", lc_messages);
	if (lang)
		g_string_append_printf (command, "LANG=\"%s\" ", lang);
	if (language)
		g_string_append_printf (command, "LANGUAGE=\"%s\" ", language);
}

void
gst_auth_do_ssh_authentication (GstTool *tool, gchar *host)
{
	gchar *ssh_args[6];
	GString *command;

	command = g_string_new (NULL);
	gst_auth_save_locale   (command);
	g_string_append (command, "`pkg-config --variable=backenddir system-tools-backends`");
	g_string_append (command, "/");
	g_string_append (command, tool->script_name);
	g_string_append (command, " --report");

	if (tool->current_platform) {
		g_string_append (command, " --platform ");
		g_string_append (command, (gchar *)gst_platform_get_key (tool->current_platform));
	}

	/* these are the ssh args */
	ssh_args[0] = SSH_PATH;
	ssh_args[1] = "-l";
	ssh_args[2] = "root";
	ssh_args[3] = host;
	ssh_args[4] = command->str;
	ssh_args[5] = NULL;

	gst_auth_do_authentication (tool, ssh_args);
	g_string_free (command, TRUE);
}

void
gst_auth_do_su_authentication (GstTool *tool)
{
	gchar *su_args[5];
	GString *command;

	command = g_string_new (NULL);
	gst_auth_save_locale   (command);
	command = g_string_append (command, tool->script_path);
	command = g_string_append (command, " --report");

	if (tool->current_platform) {
		g_string_append (command, " --platform ");
		g_string_append (command, (gchar *)gst_platform_get_key (tool->current_platform));
	}

	/* these are the su args */
	su_args[0] = SU_PATH;
	su_args[1] = "root";
	su_args[2] = "-c";
	su_args[3] = command->str;
	su_args[4] = NULL;

	gst_auth_do_authentication (tool, su_args);
	g_string_free (command, TRUE);
}
