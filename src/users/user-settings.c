/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user-settings.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <utmp.h>
#include <ctype.h>
#include <gnome.h>

#include "gst.h"
#include "gst-hig-dialog.h"
#include "users-table.h"
#include "table.h"
#include "callbacks.h"
#include "user-group-xml.h"
#include "user-settings.h"
#include "user_group.h"
#include "passwd.h"

#define MAX_TOKENS 8

extern GstTool *tool;
extern GList *groups_list;
xmlNodePtr user_profile;

static gboolean
check_user_delete (xmlNodePtr node)
{
	gchar *login;
	GtkWindow *parent;
	GtkWidget *dialog;
	gint reply;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	login = gst_xml_get_child_content (node, "login");

	if (!login) {
		g_warning ("check_user_delete: Can't get username");
		return FALSE;
	}

	if (strcmp (login, "root") == 0) {
		show_error_message ("user_settings_dialog",
				    _("The \"root\" user should not be deleted"),
				    _("This would leave the system unusable"));
		g_free (login);
		return FALSE;
	}

	dialog = gst_hig_dialog_new (parent,
				     GTK_DIALOG_MODAL,
				     GST_HIG_MESSAGE_WARNING,
				     NULL,
				     "This will disable the access of this user to the system, "
				     "but his home directory will not be deleted",
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				     NULL);
	gst_hig_dialog_set_primary_text (GST_HIG_DIALOG (dialog),
					 _("Are you sure you want to delete user \"%s\"?"), login);

	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (login);

	if (reply == GTK_RESPONSE_ACCEPT)
		return TRUE;
        else
		return FALSE;
}

gboolean
delete_user (xmlNodePtr node)
{
	if (check_user_delete (node))
	{
		del_user_groups (node);
		gst_xml_element_destroy (node);
		return TRUE;
	}

	return FALSE;
}

void
user_set_profile (xmlNodePtr profile)
{
	gint counter = 0;
	GList *element;
	gchar *value;
	gchar *buf;
	ug_data *ud;
	GtkWidget *widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");

	g_return_if_fail (profile != NULL);
	
	ud = g_object_get_data (G_OBJECT (widget), "data");

	buf = gst_xml_get_child_content (profile, "home_prefix");
	gtk_entry_set_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_home")), buf);
	g_free (buf);

	buf = gst_xml_get_child_content (profile, "shell");
	gtk_entry_set_text (GTK_ENTRY (GTK_BIN (gst_dialog_get_widget (tool->main_dialog, "user_settings_shell"))->child), buf);
	g_free (buf);

	/* FIXME: this is hidden, will this section be ever necessary? */
	gtk_widget_hide (gst_dialog_get_widget (tool->main_dialog, "user_optional_settings"));

/*
	buf = gst_xml_get_child_content (profile, "pwd_maxdays");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_max")),
				   g_strtod (buf, NULL));
	g_free (buf);

	buf = gst_xml_get_child_content (profile, "pwd_mindays");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_min")),
				   g_strtod (buf, NULL));
	g_free (buf);

	buf = gst_xml_get_child_content (profile, "pwd_warndays");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_days")),
				   g_strtod (buf, NULL));
	g_free (buf);
*/

	value = gst_xml_get_child_content (profile, "group");
	element = g_list_first (groups_list);

	while ((element != NULL) && (strcmp (element->data, value) != 0)) {
		element = element->next;
		counter++;
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (gst_dialog_get_widget (tool->main_dialog, "user_settings_group")),
				  counter);

	/* Gets a new uid */
	buf = (gchar *) find_new_id (ud->node, profile);
	if (buf) {
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_settings_uid")), 
		                           g_strtod (buf, NULL));
		g_free (buf);
	}

	user_profile = profile;
}

void
user_new_prepare (ug_data *ud)
{
	GtkWidget *button;
	gchar *buf;
	GtkWidget *widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");

	gtk_window_set_title (GTK_WINDOW (widget), _("User Account Editor"));
	g_object_set_data (G_OBJECT (widget), "data", ud);

	gtk_widget_show (gst_dialog_get_widget (tool->main_dialog, "user_settings_profiles"));
	
	populate_user_privileges_table (NULL);
	
	/* Fill menus */
	combo_add_groups   (gst_dialog_get_widget (tool->main_dialog, "user_settings_group"), TRUE);
	combo_add_shells   (gst_dialog_get_widget (tool->main_dialog, "user_settings_shell"));
	combo_add_profiles (gst_dialog_get_widget (tool->main_dialog, "user_settings_profile_menu"));

#ifdef HAVE_LIBCRACK
	/* If we have libcrack, password quality check is enabled */
	button = gst_dialog_get_widget (tool->main_dialog, "user_passwd_quality");
	gtk_widget_show (button);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
#endif

	gtk_widget_show (widget);
}

void
user_settings_dialog_close (void)
{
	GtkWidget *widget;
	GtkTreeModel *model;
	ug_data *ud;
	gint i;
	GList *list;
	char *text_boxes[] = { 
		"user_settings_name",
		"user_settings_comment",
		"user_settings_office",
		"user_settings_wphone",
		"user_settings_hphone",
		"user_settings_wphone",
		"user_settings_home",
		"user_passwd_entry1",
		"user_passwd_entry2",
		NULL};
		
	/* Clear text boxes */
	for (i = 0; text_boxes[i]; i++) {
		widget = gst_dialog_get_widget (tool->main_dialog, text_boxes[i]);
		gtk_entry_set_text (GTK_ENTRY (widget), "");
	}

	/* Set the manual password */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	
	/* Set the first notebook page as default for the next time the dialog is opened */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
	
	/* Clear user data attached to the widget */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	ud = g_object_get_data (G_OBJECT (widget), "data");
	g_free (ud);
	g_object_steal_data (G_OBJECT (widget), "data");
	gtk_widget_hide (widget);

	/* Clear groups option menu */
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (gst_dialog_get_widget (tool->main_dialog, "user_settings_group")));
	gtk_list_store_clear (GTK_LIST_STORE (model));
}

static gboolean
is_user_root (xmlNodePtr node)
{
	gchar *login;
	
	if (strcmp (node->name, "userdb") == 0) {
		/* node is set to userdb node, this means that the user is new, thus cannot be root */
		return FALSE;
	} else {
		/* node isn't set to userdb node, this means that the user is being modified */
		login = gst_xml_get_child_content (node, "login");
	
		if (strcmp (login, "root") == 0)
			return TRUE;
	}
	return FALSE;
}

static gboolean
is_login_valid (xmlNodePtr node, const gchar *login)
{
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	struct utmp ut;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (login != NULL, FALSE);

	/* If !empty. */
	if (strlen (login) < 1) {
		primary_text = g_strdup (_("The user name is empty"));
		secondary_text = g_strdup (_("A user name must be specified"));
	}

	/* If too long. */
#ifdef __FreeBSD__
	else if (strlen (login) > UT_NAMESIZE) { /*  = sizeof (ut.ut_name) */
		primary_text   = g_strdup (_("The user name is too long"));
		secondary_text = g_strdup_printf (_("The user name should have less than %i characters for being valid"), UT_NAMESIZE);
	}
#else
	else if (strlen (login) > sizeof (ut.ut_user)) {
		primary_text   = g_strdup (_("The user name is too long"));
		secondary_text = g_strdup_printf (_("The user name should have less than %i characters for being valid"), sizeof (ut.ut_user));
	}
#endif

	/* If user being modified is root */
	else if ((is_user_root (node)) && (strcmp (login, "root") != 0)) {
		primary_text   = g_strdup (_("User name for the \"root\" user should not be modified"));
	        secondary_text = g_strdup (_("This would leave the system unusable"));
	}

	/* if valid. */
	else if (!is_valid_name (login)) {
		primary_text   = g_strdup (_("User name has invalid characters"));
		secondary_text = g_strdup (_("Please set a valid user name consisting of "
					     "a lower case letter followed by lower case letters and numbers"));
	}

	/* if !exist. */
	else if (node_exists (node, "login", login)) {
		primary_text   = g_strdup_printf (_("User name \"%s\" already exists"), login);
		secondary_text = g_strdup (_("Please select a different user name"));
	}

	/* If anything is wrong. */
	if (primary_text) {
		show_error_message ("user_settings_dialog", primary_text, secondary_text);
		g_free (primary_text);
		g_free (secondary_text);

		return FALSE;
	} else {
		return TRUE;
	}
}

static gboolean
is_comment_valid (gchar *name, gchar *location, gchar *wphone, gchar *hphone)
{
	gchar *comment = g_strjoin (" ", name, location, wphone, hphone, NULL);
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	gint i;
	
	for (i = 0; i < strlen (comment); i++) {
		if (iscntrl (comment[i]) || comment[i] == ',' || comment[i] == '=' || comment[i] == ':') {
			primary_text   = g_strdup_printf (N_("Invalid character \"%c\" in comment"), comment[i]);
			secondary_text = g_strdup (N_("Check that this character is not used"));
			break;
		}
	}
	g_free (comment);

	if (primary_text) {
		show_error_message ("user_settings_dialog", _(primary_text), _(secondary_text));
		g_free (primary_text);
		g_free (secondary_text);
		
		return FALSE;
	} else {
		return TRUE;
	}
}

static gchar *
parse_user_home (gchar *old_home, gchar *name)
{
	gchar *home;
	gchar **buf;
	gint i;

	buf = g_strsplit (old_home, "/", MAX_TOKENS);

	for (i = 0; buf[i]; i++) {
		if (!strcmp (buf[i], "$user")) {
			g_free (buf[i]);
			buf[i] = g_strdup (name);
		}
	}
	
	home = g_strjoinv ("/", buf);
	
	g_strfreev (buf);
	
	return home;
}

static gboolean
is_home_valid (xmlNodePtr node, gchar *home)
{
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	struct stat s;
	
	if (!home || (strlen (home) < 1)) {
		primary_text   = g_strdup (N_("Home directory should not be empty"));
		secondary_text = g_strdup (N_("Make sure you provide a home directory"));
	} else if (*home != '/') {
		primary_text   = g_strdup (N_("Incomplete path in home directory"));
		secondary_text = g_strdup (N_("Please enter full path for home directory\n"
					      "<span size=\"smaller\">i.e.: /home/john</span>"));
	} else if ((is_user_root (node)) && (strcmp (home, "/root") != 0)) {
		primary_text   = g_strdup (N_("Home directory of the \"root\" user should not be modified"));
		secondary_text = g_strdup (N_("This would leave the system unusable"));
	}

/*	else if (stat (home, &s))
	{
		switch (errno) {
		case ENOTDIR: buf = g_strdup (_("Part of the path to the home directory is a file"));
		case ELOOP:   buf = g_strdup (_("There is a loop in the path to the home directory"));
		case ENOMEM:  buf = g_strdup (_("Not enough memory to check the home directory"));
		case ENAMETOOLONG: buf = g_strdup (_("The path to the home directory is too long"));
		}
	}
*/
	if (primary_text) {
		show_error_message ("user_settings_dialog", _(primary_text), _(secondary_text));
		g_free (primary_text);
		g_free (secondary_text);
		
		return FALSE;
	} else {
		return TRUE;
	}
}

static gboolean
is_user_uid_valid (xmlNodePtr node, const gchar *uid)
{
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	GtkWidget *dialog;

	if (!is_valid_id (uid)) {
		primary_text   = g_strdup (N_("Invalid user ID"));
		secondary_text = g_strdup (N_("User ID must be a positive number"));
	} else if ((is_user_root (node)) && (strcmp (uid, "0") != 0)) {
		primary_text   = g_strdup (N_("User ID of the \"root\" user should not be modified"));
		secondary_text = g_strdup (N_("This would leave the system unusable"));
	}
	
	if (primary_text) {
		show_error_message ("user_settings_dialog", _(primary_text), _(secondary_text));
		g_free (primary_text);
		g_free (secondary_text);
		
		return FALSE;
	} else {
		if (node_exists (node, "uid", uid)) {
			dialog = gst_hig_dialog_new (GTK_WINDOW (gst_dialog_get_widget (tool->main_dialog,
											"user_settings_dialog")),
						     GTK_DIALOG_MODAL,
						     GST_HIG_MESSAGE_INFO,
						     NULL,
						     _("Several users may share a single user ID, "
						       "but it's not common and may lead to security problems"),
						     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						     NULL);
			gst_hig_dialog_set_primary_text (GST_HIG_DIALOG (dialog),
							 _("User ID %s already exists"), uid);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}

		return TRUE;
	}
}


static gboolean
is_shell_valid (const gchar *val)
{
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	
	if (strlen (val) > 0 && *val != '/') {
		primary_text   = g_strdup (N_("Incomplete path in shell"));
		secondary_text = g_strdup (N_("Please enter full path for shell\n"
					      "<span size=\"smaller\">i.e.: /bin/sh</span>"));
	}

	if (primary_text) {
		show_error_message ("user_settings_dialog", _(primary_text), _(secondary_text));
		g_free (primary_text);
		g_free (secondary_text);
		
		return FALSE;
	} else {
		return TRUE;
	}
}

static gboolean
is_password_valid (const gchar *passwd1, const gchar *passwd2) 
{
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	
	if (!passwd1 || !passwd2 || strlen (passwd1) < 1 || strlen (passwd2) < 1) {
		primary_text   = g_strdup (N_("Password should not be empty"));
		secondary_text = g_strdup (N_("A password must be provided"));
	} else if (strcmp (passwd1, passwd2) != 0) {
		primary_text   = g_strdup (N_("Password confirmation isn't correct"));
		secondary_text = g_strdup (N_("Check that you have provided the same password in both text fields"));
	}

	if (primary_text) {
		show_error_message ("user_settings_dialog", _(primary_text), _(secondary_text));
		g_free (primary_text);
		g_free (secondary_text);
		
		return FALSE;
	} else {
		return TRUE;
	}
}

static gchar*
group_check (xmlNodePtr node, const gchar *groupname)
{
	gchar *buf, *gid;
	xmlNodePtr groupdb = get_corresp_field (node);
	xmlNodePtr group;
	
	gid = group_xml_get_gid (groupdb, (gchar *) groupname);
	if (!gid) {
		gid = find_new_id (groupdb, user_profile);
		group = group_add_blank_xml (groupdb);
		group_update_xml (group, (gchar *) groupname, gid, NULL);
	}
	
	return gid;
}

gboolean
user_update (ug_data *ud)
{
	UserAccountData *data;
	gboolean change_password = FALSE;
	GList *list;
	
	data = g_new0 (UserAccountData, 1);
	
	/* check user login */
	data->login = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_name")));
	if (!is_login_valid (ud->node, data->login)) {
		return FALSE;
	}
	
	/* check user comments */
	data->name = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_comment")));
	data->location = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_office")));
	data->work_phone = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_wphone")));
	data->home_phone = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_hphone")));
	if (!is_comment_valid (data->name, data->location, data->work_phone, data->home_phone)) {
		return FALSE;
	}

	/* check user home */
	if (ud->is_new) {
		/* The user is new, home must be parsed to replace the '$user' tag */
		data->home = parse_user_home ((gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_home"))), data->login);
	} else {
		data->home = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_home")));
	}
	if (!is_home_valid (ud->node, data->home)) {
		return FALSE;
	}
	
	/* check user uid */
	data->uid = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_settings_uid"))));
	if (!is_user_uid_valid (ud->node, data->uid)) {
		return FALSE;
	}
	
	/* check user shell */
	data->shell = (gchar *) gtk_entry_get_text (GTK_ENTRY (GTK_BIN (gst_dialog_get_widget (tool->main_dialog, "user_settings_shell"))->child));
	if (!is_shell_valid (data->shell)) {
		return FALSE;
	}
	
	/* check user password */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual")))) {
		/* Manual password enabled */
		if (!ud->is_new) {
			/* If it is a modified user, we have to check that the entries are unchanged */
			gboolean d1, d2;
			d1 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1")), "changed"));
			d2 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2")), "changed"));
			if (d1 || d2) {
				data->password1 = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1")));
				data->password2 = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2")));
				if (!is_password_valid (data->password1, data->password2)) {
					return FALSE;
				}
				change_password = TRUE;
			}
		} else {
			/* If it is a new user, we have to check it anyway */
			data->password1 = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1")));
			data->password2 = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2")));
			if (!is_password_valid (data->password1, data->password2)) {
				return FALSE;
			}
		}
	} else {
		/* Automatic password generation enabled */
		data->password1 = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_passwd_random_entry")));
	}

	/* get user main group GID*/
	data->group = g_list_nth_data (groups_list, gtk_combo_box_get_active (GTK_COMBO_BOX (gst_dialog_get_widget (tool->main_dialog, "user_settings_group"))));
	if (strcmp (data->group, "$user") == 0)
		data->group = data->login;
	
	data->gid = group_check (ud->node, data->group);
	if (!data->gid)
		return FALSE;

	g_list_free (data->extra_groups);
	data->extra_groups = user_privileges_get_list ();

	data->pwd_maxdays = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_max"))));
	data->pwd_mindays = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_min"))));
	data->pwd_warndays = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_days"))));
	
	if (ud->is_new) {
		/* Add new user, update table */
		xmlNodePtr node;
		
		node = user_add_blank_xml (ud->node);
		user_update_xml (node, data, TRUE);
	} else {
		/* Entered data ok, not new: just update */
		
		del_user_groups (ud->node);
		user_update_xml (ud->node, data, change_password);
	}
	
	tables_update_content ();
	return TRUE;
}

static void
user_settings_dialog_prepare_password (void){
	GtkWidget *pwd1, *pwd2;
	
	pwd1 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1");
	pwd2 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2");
	
	gtk_entry_set_text (GTK_ENTRY (pwd1), "********");
	gtk_entry_set_text (GTK_ENTRY (pwd2), "********");
	
	g_object_set_data (G_OBJECT (pwd1), "changed", GINT_TO_POINTER (FALSE));
	g_object_set_data (G_OBJECT (pwd2), "changed", GINT_TO_POINTER (FALSE));
}

static void
user_settings_dialog_prepare_comments (gchar *comments)
{
	GtkWidget *w0;
	gint i;
	gchar **buf = NULL;
	gchar *widgets[] = {
		"user_settings_comment",
		"user_settings_office",
		"user_settings_wphone",
		"user_settings_hphone"
	};
	
	if (!comments)
		return;
	buf = g_strsplit (comments, ",", 4);
	
	for (i = 0; buf[i] != NULL; i++) {
		w0 = gst_dialog_get_widget (tool->main_dialog, widgets[i]);
		gst_ui_entry_set_text (w0, buf[i]);
		g_free (buf[i]);
	}
}

static void
user_settings_dialog_prepare (ug_data *ud)
{
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	GtkWidget *w0;
	gchar *txt, *name;
	gint counter = 0;
	GList *element;
	
	g_return_if_fail (ud != NULL);

	gtk_widget_hide (gst_dialog_get_widget (tool->main_dialog, "user_settings_profiles"));
	
	/* Attach the data to the dialog */
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	g_object_set_data (G_OBJECT (w0), "data", ud);

	/* Set user login */
	name = gst_xml_get_child_content (ud->node, "login");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_settings_name");
	gst_ui_entry_set_text (w0, name);

	/* Set comments */
	user_settings_dialog_prepare_comments (gst_xml_get_child_content (ud->node, "comment"));
	
	/* Set UID */
	txt = gst_xml_get_child_content (ud->node, "uid");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	/* Set home directory */
	txt = gst_xml_get_child_content (ud->node, "home");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	gst_ui_entry_set_text (w0, txt);
	g_free (txt);
	
	/* Set shells combo */
	combo_add_shells (combo);
	txt = gst_xml_get_child_content (ud->node, "shell");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	gtk_entry_set_text (GTK_ENTRY (GTK_BIN (w0)->child), txt);
	g_free (txt);
	
	/* set main group option menu */
	combo_add_groups (option_menu, FALSE);
	txt = get_group_name (gst_xml_get_child_content (ud->node, "gid"));

	if (txt != NULL) {
		element = g_list_first (groups_list);

		while ((element != NULL) && (strcmp (element->data, txt) != 0))	{
			element = element->next;
			counter++;
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (gst_dialog_get_widget (tool->main_dialog, "user_settings_group")), counter);
		g_free (txt);
	}

	populate_user_privileges_table (name);
	
	/* Set password */
	user_settings_dialog_prepare_password ();

	/* FIXME: This is hidden, will this section be ever necessary? */
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_optional_settings");
	gtk_widget_hide (w0);

/*
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_max");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	txt = gst_xml_get_child_content (ud->node, "passwd_min_life");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_min");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	txt = gst_xml_get_child_content (ud->node, "passwd_exp_warn");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_days");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
*/
	
	/* Set window title */
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), g_strdup_printf (_("Settings for User %s"), name));
	g_free (name);
	
	gtk_widget_show (w0);
}

void
user_settings_prepare (ug_data *ud)
{
	gchar *buf;
	
	g_return_if_fail (ud != NULL);

	buf = gst_xml_get_child_content (ud->node, "name");

	if (buf)
	{
		/* Has to be some kind of group */
		g_free (buf);
		g_warning ("settings_prepare: Shouldn't be here");
		return;
	}

	buf = gst_xml_get_child_content (ud->node, "login");

	if (buf)
	{
		/* Has to be some kind of user */
		g_free (buf);

		user_settings_dialog_prepare (ud);
		return;
	}

	g_warning ("settings_prepare: shouldn't be here");
}
