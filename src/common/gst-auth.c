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
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
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
				primary_text   = g_strdup (_("Could not connect to the computer"));
				secondary_text = g_strdup (_("Check that you have access to this network "
							     "and that the computer is actually working and running SSHD"));
			} else {
				/* the proccess was running su */
				primary_text   = g_strdup (_("The password you entered is invalid"));
				secondary_text = g_strdup (_("Check that you typed it correctly "
							     "and that you haven't activated caps lock"));
			}
		} else if ((WIFEXITED (status)) && (WEXITSTATUS (status)) && (WEXITSTATUS (status) == 255)) {
			if (tool->remote_config)
				auth_command = "ssh";
			else
				auth_command = "su";

			primary_text   = g_strdup_printf (_("Could not run \"%s\""), auth_command);
			secondary_text = g_strdup (_("Check that you have permissions to run it."));
		} else {
			primary_text   = g_strdup (_("An unexpected error has ocurred."));
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
void
gst_auth_run_term (GstTool *tool, gchar *args[])
{
	struct termios t;

	tool->backend_pid = forkpty (&tool->backend_master_fd, NULL, NULL, NULL);

	if (tool->backend_pid < 0) {
		g_warning ("could not fork to backend");
	}
	else if (tool->backend_pid == 0) {
		/* It's the child process */
		execv (args[0], args);
		exit (255);
	} else {
#ifndef __FreeBSD__
		/* Linux's su works ok with echo disabling */
		tcgetattr (tool->backend_master_fd, &t);
		t.c_lflag ^= ECHO;
		tcsetattr (tool->backend_master_fd, TCSANOW, &t);
#endif
		fcntl (tool->backend_master_fd, F_SETFL, O_NONBLOCK);

		tool->timeout_id = g_timeout_add (1000, (GSourceFunc) gst_auth_wait_child, tool);
		tool->backend_stream = fdopen (tool->backend_master_fd, "a+");
	}
}

/* writes the password to the term with su in it */
void
gst_auth_write_password (GstTool *tool, gchar *pwd)
{
	gchar *answer = "yes\n";
	gboolean cont = FALSE;
	gchar *str;
	
	/* read all the su or ssh output and flush the descriptors */
	while (!cont) {
		str = gst_tool_read_from_backend (tool, "assword:", "/no)?", NULL);

		/* FIXME: hope that someday we can get rid of this ssh output string parsing */
		if (g_strrstr (g_ascii_strup (str, -1), "AUTHENTICITY") != NULL) {
			/* it's the "add to known hosts list" ssh's message, just answer "yes" */
			gst_tool_write_to_backend (tool, answer);
		} else if (g_strrstr (g_ascii_strup (str, -1), "PASSWORD") != NULL) {
			cont = TRUE;
		}

		g_free (str);
	}

	gst_tool_write_to_backend (tool, pwd);
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
		return GST_AUTH_RUN_AS_ROOT;
		break;
	case GST_AUTH_RESPONSE_NP:
		/* Run unprivileged */
		return GST_AUTH_RUN_AS_USER;
		break;
	default:
		/* Cancel */
		return GST_AUTH_CANCEL;
		break;
	}

	g_assert ("Not reached");
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

	gchar buffer[40];
	gint size;
	struct termios t;

	gst_auth_run_term (tool, args);

	if ((geteuid () == 0) && (tool->remote_config == FALSE)) {
		/* only avoid password requesting when the user is already root and works locally */
		tool->root_access = ROOT_ACCESS_REAL;
	} else {
		result = gst_auth_get_password (&password);

		if (result == GST_AUTH_RUN_AS_ROOT) {
			gst_auth_write_password (tool, password);
			/* gst_auth_read_output (tool); */

			if (strlen (password) > 0)
				memset (password, 0, strlen (password));
			tool->root_access = ROOT_ACCESS_REAL;
		} else if (result == GST_AUTH_RUN_AS_USER) {
			tool->root_access = ROOT_ACCESS_NONE;
		} else {
			exit (0);
		}
	}

#ifdef __FreeBSD__
	/* FreeBSD seems to have weird issues with su and echo disabling */
	tcgetattr (tool->backend_master_fd, &t);
	t.c_lflag ^= ECHO;
	tcsetattr (tool->backend_master_fd, TCSANOW, &t);
#endif

}

void
gst_auth_do_ssh_authentication (GstTool *tool, gchar *host)
{
	gchar *ssh_args[6];
	GString *command;

	command = g_string_new ("`pkg-config --variable=backenddir system-tools-backends`");
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
}

void
gst_auth_do_su_authentication (GstTool *tool)
{
	gchar *su_args[5];
	GString *command;

	command = g_string_new (tool->script_path);
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
}
