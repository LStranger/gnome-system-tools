/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

/* Functions for transferring information between XML tree and UI */


#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>
#include "global.h"

#include "transfer.h"
#include "user_group.h"
#include "callbacks.h"
#include "e-table.h"
#include "profile.h"

extern XstTool *tool;

/* Structure with some hard-coded defaults, just in case any of the tags is not present. */
/* These were taken form RH 6.2's default values. Any better suggestions? */
/* NULL means not present for string members. */

const static login_defs default_logindefs = {
	NULL, /* qmail_dir */
	"/var/spool/mail", /* mailbox_dir */
	NULL, /* mailbox_file */
	99999, /* passwd_max_day_use */
	0, /* passwd_min_day_use */
	5, /* passwd_min_length */
	7, /* passwd_warning_advance_days */
	500, /* new_user_min_id */
	60000, /* new_user_max_id */
	500, /* new_group_min_id */
	60000, /* new_group_max_id */
	NULL, /* del_user_additional_command */
	TRUE /* create_home */
};


gchar *user_tags[] = {
	"key", "login", "password", "uid", "gid", "comment", "home", "shell", "last_mod",
	"passwd_min_life", "passwd_max_life", "passwd_exp_warn", "passwd_exp_disable", "passwd_disable",
	"reserved", "is_shadow", NULL
};

gchar *group_tags[] = {
	"key", "name", "password", "gid", NULL
};


/* Helper functions */

static guint
my_atoi (gchar *str) 
{
	if (!str || !*str)
		return 0;
	return atoi (str);
}

/* ---- */

static void
transfer_logindefs_from_xml (xmlNodePtr root)
{
	xmlNodePtr node, n0;
	gchar *logindefs_tags[] = {
		"qmail_dir", "mailbox_dir", "mailbox_file", "passwd_max_day_use",
		"passwd_min_day_use", "passwd_min_length", "passwd_warning_advance_days", "new_user_min_id",
		"new_user_max_id", "new_group_min_id", "new_group_max_id", "del_user_additional_command",
		"create_home", NULL
	};
	gchar *tag, *tmp;
	gint i;

	/* Assign defaults */
	
	logindefs = default_logindefs;
	
	/* Find login.defs */
	
	node = xst_xml_element_find_first (root, "logindefs");
	if (!node)
	{
		g_warning ("transfer_logindefs_from_xml: couldn't find logindefs node.");
		return;
	}

	/* make login.defs struct */

	for (i = 0, tag = logindefs_tags[0]; tag; i++, tag = logindefs_tags[i]) 
	{
		n0 = xst_xml_element_find_first (node, tag);

		if (n0) 
		{
			switch (i)
			{
			 case  0: logindefs.qmail_dir    = xst_xml_element_get_content (n0); break;
			 case  1: logindefs.mailbox_dir  = xst_xml_element_get_content (n0); break;
			 case  2: logindefs.mailbox_file = xst_xml_element_get_content (n0); break;
			 case  3: logindefs.passwd_max_day_use          = my_atoi (xst_xml_element_get_content (n0)); break;
			 case  4: logindefs.passwd_min_day_use          = my_atoi (xst_xml_element_get_content (n0)); break;
			 case  5: logindefs.passwd_min_length           = my_atoi (xst_xml_element_get_content (n0)); break;
			 case  6: logindefs.passwd_warning_advance_days = my_atoi (xst_xml_element_get_content (n0)); break;
			 case  7: logindefs.new_user_min_id             = my_atoi (xst_xml_element_get_content (n0)); break;
			 case  8: logindefs.new_user_max_id             = my_atoi (xst_xml_element_get_content (n0)); break;
			 case  9: logindefs.new_group_min_id            = my_atoi (xst_xml_element_get_content (n0)); break;
			 case 10: logindefs.new_group_max_id            = my_atoi (xst_xml_element_get_content (n0)); break;
			 case 11: logindefs.del_user_additional_command = xst_xml_element_get_content (n0); break;
			 case 12: 
				tmp = xst_xml_element_get_content (n0);
				if (! strcmp (tmp, "yes"))
					logindefs.create_home = TRUE;
				else
					logindefs.create_home = FALSE;
				break;
			 case 13: g_warning ("transfer_logindefs_from_xml: we shouldn't be here."); break;
			}
		}
	}
}

static void
transfer_logindefs_to_gui (void)
{
	GtkWidget *w0;
	GtkSpinButton *spin;
	gint i;

	gchar *widgets[] = {"defs_min_uid", "defs_max_uid", "defs_min_gid", "defs_max_gid", 
			    "defs_passwd_max_days", "defs_passwd_min_days", 
			    "defs_passwd_warn", "defs_passwd_min_len", NULL};

	gint values[] = {logindefs.new_user_min_id, logindefs.new_user_max_id,
			 logindefs.new_group_min_id, logindefs.new_group_max_id,
			 logindefs.passwd_max_day_use, logindefs.passwd_min_day_use,
			 logindefs.passwd_warning_advance_days, logindefs.passwd_min_length, 0};

	/* System settings. */

	w0 = xst_dialog_get_widget (tool->main_dialog, "defs_create_home");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w0), logindefs.create_home);

	w0 = xst_dialog_get_widget (tool->main_dialog, "defs_mail_dir");
	gtk_entry_set_text (GTK_ENTRY (w0), logindefs.mailbox_dir);

	/* User and group id's and passwords. */

	for (i = 0; widgets[i]; i++)
	{
		spin = GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, widgets[i]));
		gtk_spin_button_set_value (spin, (gfloat) values[i]);
	}
}

static void
transfer_logindefs_to_xml (xmlNodePtr root)
{
	GtkWidget *w0;
	GtkSpinButton *spin;
	gchar *val;
	xmlNodePtr node;
	gint i;

	gchar *widgets[] = {"defs_min_uid", "defs_max_uid", "defs_min_gid", "defs_max_gid", 
			    "defs_passwd_max_days", "defs_passwd_min_days", 
			    "defs_passwd_warn", "defs_passwd_min_len", NULL};

	gchar *nodes[] = {"new_user_min_id", "new_user_max_id", "new_group_min_id",
	       		 "new_group_max_id", "passwd_max_day_use", "passwd_min_day_use",
			 "passwd_warning_advance_days", "passwd_min_length", NULL};

	g_message ("saving to XML");

	root = xst_xml_element_find_first (root, "logindefs");

	/* System settings. */

	w0 = xst_dialog_get_widget (tool->main_dialog, "defs_create_home");
	node = xst_xml_element_find_first (root, "create_home");
	if (!node)
		node = xst_xml_element_add (root, "create_home");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w0)))
		xst_xml_element_set_content (node, "yes");
	else
		xst_xml_element_set_content (node, "no");

	w0 = xst_dialog_get_widget (tool->main_dialog, "defs_mail_dir");
	val = gtk_editable_get_chars (GTK_EDITABLE (w0), 0, -1);
	node = xst_xml_element_find_first (root, "mailbox_dir");
	if (!node)
		node = xst_xml_element_add (root, "mailbox_dir");

	xst_xml_element_set_content (node, val);
	g_free (val);

	/* User and group id's and passwords. */

	for (i = 0; widgets[i]; i++)
	{
		spin = GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, widgets[i]));
		val = g_strdup_printf ("%d", gtk_spin_button_get_value_as_int (spin));

		node = xst_xml_element_find_first (root, nodes[i]);
		if (!node)
			node = xst_xml_element_add (root, nodes[i]);

		xst_xml_element_set_content (node, val);
		g_free (val);
	}
}

void
transfer_xml_to_gui (XstTool *tool, gpointer data)
{
	xmlNodePtr root;
	Profile *pf;

	root = xst_xml_doc_get_root (tool->config);

	transfer_logindefs_from_xml (root);
	transfer_logindefs_to_gui ();
	
	/* Popuplate tables */
	populate_all_tables ();

	/* Profiles */
	profile_table_init ();
	pf = profile_get_default ();
	profile_table_add_profile (pf, TRUE);
}

void
transfer_gui_to_xml (XstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = xst_xml_doc_get_root (tool->config);
	
	transfer_logindefs_to_xml (root);
}


