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

static void
create_groups_lists (xmlNodePtr node, GList **user_settings_all_list, GList **user_settings_members_list)
{
	*user_settings_members_list = user_get_groups (node);
	*user_settings_all_list = my_g_list_remove_duplicates (get_list_from_node ("name", NODE_GROUP), *user_settings_members_list);
}

static void
create_groups_gtk_trees (void)
{
	GtkWidget *user_settings_all = gst_dialog_get_widget (tool->main_dialog, "user_settings_all");
	GtkWidget *user_settings_members = gst_dialog_get_widget (tool->main_dialog, "user_settings_members");
	GtkWidget *add_button, *remove_button;
	GtkTreeSelection *selection;
	GtkSizeGroup *sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	
	/* connect signals and attach data if they haven't been created already */
	if (g_object_get_data (G_OBJECT (user_settings_all), "button") == NULL) {
		create_gtk_tree_list (user_settings_all);
		g_object_set_data (G_OBJECT (user_settings_all), "button", "user_settings_add");
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_settings_all));
		g_signal_connect (G_OBJECT (selection),
		                  "changed",
		                  G_CALLBACK (on_list_select_row),
		                  NULL);

		create_gtk_tree_list (user_settings_members);
		g_object_set_data (G_OBJECT (user_settings_members), "button", "user_settings_remove");
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (user_settings_members));
		g_signal_connect (G_OBJECT (selection),
		                  "changed",
		                  G_CALLBACK (on_list_select_row),
		                  NULL);

		/* We also need to attach some data to the 'add' and 'remove' buttons */
		add_button = gst_dialog_get_widget (tool->main_dialog, "user_settings_add");
		g_object_set_data (G_OBJECT (add_button), "in", user_settings_all);
		g_object_set_data (G_OBJECT (add_button), "out", user_settings_members);
		
		remove_button = gst_dialog_get_widget (tool->main_dialog, "user_settings_remove");
		g_object_set_data (G_OBJECT (remove_button), "in", user_settings_members);
		g_object_set_data (G_OBJECT (remove_button), "out", user_settings_all);

		/* Add the scrolled windows that contain the trees in the same GtkSizeGroup */
		gtk_size_group_add_widget (sizegroup, gst_dialog_get_widget (tool->main_dialog, "group_settings_all_container"));
		gtk_size_group_add_widget (sizegroup, gst_dialog_get_widget (tool->main_dialog, "group_settings_members_container"));
	}
}

static gboolean
check_user_delete (xmlNodePtr node)
{
	gchar *login, *buf;
	GtkWindow *parent;
	GtkWidget *dialog;
	gint reply;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	login = gst_xml_get_child_content (node, "login");

	if (!login)
	{
		g_warning ("check_user_delete: Can't get username");
		return FALSE;
	}

	if (strcmp (login, "root") == 0)
	{
		buf = g_strdup (_("The root user must not be deleted."));
		show_error_message ("user_settings_dialog", buf);
		g_free (login);
		g_free (buf);
		return FALSE;
	}

	buf = g_strdup_printf (_("Are you sure you want to delete user %s?"), login);
	dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, buf);
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (buf);
	g_free (login);
	
	if (reply == GTK_RESPONSE_NO)
		return FALSE;
        else
		return TRUE;
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
	
	ud = g_object_get_data (G_OBJECT (widget), "data");
	
	gtk_entry_set_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_home")),
			    gst_xml_get_child_content (profile, "home_prefix"));
	gtk_entry_set_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_shell_entry")),
			    gst_xml_get_child_content (profile, "shell"));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_max")),
				   g_strtod (gst_xml_get_child_content (profile, "pwd_maxdays"), NULL));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_min")),
				   g_strtod (gst_xml_get_child_content (profile, "pwd_mindays"), NULL));
	gtk_spin_button_set_value ( GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_days")),
				   g_strtod (gst_xml_get_child_content (profile, "pwd_warndays"), NULL));

	value = gst_xml_get_child_content (profile, "group");
	element = g_list_first (groups_list);

	while ((element != NULL) && (strcmp (element->data, value) != 0))
	{
		element = element->next;
		counter++;
	}
	gtk_option_menu_set_history (GTK_OPTION_MENU (gst_dialog_get_widget (tool->main_dialog, "user_settings_group")), counter);

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
	GList *user_settings_all_list = NULL;
	GList *user_settings_members_list = NULL;
	GtkWidget *widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	GtkWidget *user_settings_all = gst_dialog_get_widget (tool->main_dialog, "user_settings_all");
	GtkWidget *user_settings_members = gst_dialog_get_widget (tool->main_dialog, "user_settings_members");

	gtk_window_set_title (GTK_WINDOW (widget), _("User Account Editor"));
	g_object_set_data (G_OBJECT (widget), "data", ud);

	gtk_widget_show (gst_dialog_get_widget (tool->main_dialog, "user_settings_profiles"));
	
	/* Creates and fills GtkTrees */
	create_groups_gtk_trees ();
	create_groups_lists (ud->node, &user_settings_all_list, &user_settings_members_list);
	populate_gtk_tree_list (GTK_TREE_VIEW (user_settings_all), user_settings_all_list);
	
	/* Attach the GLists to the GtkTreeViews */
	g_object_set_data (G_OBJECT (user_settings_all), "list", user_settings_all_list);
	g_object_set_data (G_OBJECT (user_settings_members), "list", user_settings_members_list);
	
	/* Fill menus */
	option_menu_add_profiles (gst_dialog_get_widget (tool->main_dialog, "user_settings_profile_menu"));
	option_menu_add_groups (gst_dialog_get_widget (tool->main_dialog, "user_settings_group"), TRUE);
	combo_add_shells (gst_dialog_get_widget (tool->main_dialog, "user_settings_shell"));
	
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
	GtkWidget *user_settings_all = gst_dialog_get_widget (tool->main_dialog, "user_settings_all");
	GtkWidget *user_settings_members = gst_dialog_get_widget (tool->main_dialog, "user_settings_members");
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

	/* Clear both lists */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_settings_all));
	gtk_tree_store_clear (GTK_TREE_STORE (model));

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_settings_members));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	/* Set unsensitive the add and remove buttons */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_add");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_remove");
	gtk_widget_set_sensitive (widget, FALSE);
	
	/* Set the manual password */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	
	/* Set the first notebook page as default for the next time the dialog is opened */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
	
	/* Clear the GLists attached to the GtkTreeViews */
	list = g_object_get_data (G_OBJECT (user_settings_all), "list");
	g_list_free (list);
	g_object_steal_data (G_OBJECT (user_settings_all), "list");
	
	list = g_object_get_data (G_OBJECT (user_settings_members), "list");
	g_list_free (list);
	g_object_steal_data (G_OBJECT (user_settings_members), "list");

	/* Clear user data attached to the widget */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	ud = g_object_get_data (G_OBJECT (widget), "data");
	g_free (ud);
	g_object_steal_data (G_OBJECT (widget), "data");
	gtk_widget_hide (widget);

	/* Clear groups option menu */
	gtk_option_menu_remove_menu (GTK_OPTION_MENU (gst_dialog_get_widget (tool->main_dialog, "user_settings_group")));
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
	gchar *buf = NULL;
	struct utmp ut;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (login != NULL, FALSE);

	/* If !empty. */
	if (strlen (login) < 1)
		buf = g_strdup (_("The username is empty."));

	/* If too long. */
#ifdef __FreeBSD__
	else if (strlen (login) > UT_NAMESIZE) /*  = sizeof (ut.ut_name) */
#else
	else if (strlen (login) > sizeof (ut.ut_user))
#endif
		buf = g_strdup (_("The username is too long."));
	
	/* If user being modified is root */
	else if ((is_user_root (node)) && (strcmp (login, "root") != 0))
		buf = g_strdup (_("root username shouldn't be modified."));

	/* if valid. */
	else if (!is_valid_name (login))
		buf = g_strdup (_("Please set a valid username consisting of a lower case letter followed by lower case letters and numbers."));

	/* if !exist. */
	else if (node_exists (node, "login", login))
		buf = g_strdup_printf (_("Username \"%s\" already exists.\n\nPlease select a different Username"), login);

	/* If anything is wrong. */
	if (buf) {
		show_error_message ("user_settings_dialog", buf);
		g_free (buf);

		return FALSE;
	} else {
		return TRUE;
	}
}

static gboolean
is_comment_valid (gchar *name, gchar *location, gchar *wphone, gchar *hphone)
{
	gchar *comment = g_strjoin (" ", name, location, wphone, hphone, NULL);
	gchar *buf = NULL;
	gint i;
	
	for (i = 0; i < strlen (comment); i++) {
		if (iscntrl (comment[i]) || comment[i] == ',' || comment[i] == '=' || comment[i] == ':') {
			buf = g_strdup_printf (N_("Invalid character '%c' in comment."), comment[i]);
			break;
		}
	}
	g_free (comment);

	if (buf) {
		show_error_message ("user_settings_dialog", buf);
		g_free (buf);
		
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
	gchar *buf = NULL;
	struct stat s;
	
	if (!home || (strlen (home) < 1))
		buf = g_strdup (N_("Home directory must not be empty."));

	else if (*home != '/')
		buf = g_strdup (N_("Please enter full path for home directory."));
	
	else if ((is_user_root (node)) && (strcmp (home, "/root") != 0))
		buf = g_strdup (_("root home shouldn't be modified."));

/*	else if (stat (home, &s))
	{
		switch (errno) {
		case ENOTDIR: buf = g_strdup (_("Part of the path to the home directory is a file."));
		case ELOOP:   buf = g_strdup (_("There is a loop in the path to the home directory."));
		case ENOMEM:  buf = g_strdup (_("Not enough memory to check the home directory."));
		case ENAMETOOLONG: buf = g_strdup (_("The path to the home directory is too long."));
		}
	}
*/
	if (buf) {
		show_error_message ("user_settings_dialog", buf);
		g_free (buf);
		
		return FALSE;
	} else {
		return TRUE;
	}
}

static gboolean
is_user_uid_valid (xmlNodePtr node, const gchar *uid)
{
	gchar *buf = NULL;

	if (!is_valid_id (uid))
		buf = g_strdup (_("User id must be a positive number."));
	else if ((is_user_root (node)) && (strcmp (uid, "0") != 0))
		buf = g_strdup (_("root uid shouldn't be modified."));
	else if (node_exists (node, "uid", uid)) {
		buf = g_strdup (_("Such user id already exists."));
	}
	
	if (buf) {
		show_error_message ("user_settings_dialog", buf);
		g_free (buf);
		
		return FALSE;
	} else {
		return TRUE;
	}
}


static gboolean
is_shell_valid (const gchar *val)
{
	gchar *buf = NULL;

	if (strlen (val) > 0 && *val != '/')
		buf = g_strdup (_("Please give shell with full path."));

	if (buf) {
		show_error_message ("user_settings_dialog", buf);
		g_free (buf);
		
		return FALSE;
	} else {
		return TRUE;
	}
}

static gboolean
is_password_valid (const gchar *passwd1, const gchar *passwd2) 
{
	gchar *buf = NULL;
	
	if (!passwd1 || !passwd2 || strlen (passwd1) < 1 || strlen (passwd2) < 1)
		buf = g_strdup (_("Password must not be empty."));
	
	else if (strcmp (passwd1, passwd2) != 0)
		buf = g_strdup (_("Password confirmation isn't correct."));

	if (buf) {
		show_error_message ("user_settings_dialog", buf);
		g_free (buf);
		
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
	GtkWidget *user_settings_members = gst_dialog_get_widget (tool->main_dialog, "user_settings_members");
	UserAccountData *data;
	gboolean change_password = FALSE;
	GList *list;
	
	data = g_new (UserAccountData, 1);
	
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
	data->shell = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_settings_shell_entry")));
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
	data->group = g_list_nth_data (groups_list, gtk_option_menu_get_history (GTK_OPTION_MENU (gst_dialog_get_widget (tool->main_dialog, "user_settings_group"))));
	if (strcmp (data->group, "$user") == 0)
		data->group = data->login;
	
	data->gid = group_check (ud->node, data->group);
	if (!data->gid) {
		return FALSE;
	}

	g_list_free (data->extra_groups);
	data->extra_groups = g_list_copy (g_object_get_data (G_OBJECT (user_settings_members), "list"));

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
	GtkWidget *user_settings_all = gst_dialog_get_widget (tool->main_dialog, "user_settings_all");
	GtkWidget *user_settings_members = gst_dialog_get_widget (tool->main_dialog, "user_settings_members");
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	GtkWidget *w0;
	gchar *txt, *name;
	GList *user_settings_all_list = NULL;
	GList *user_settings_members_list = NULL;
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
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell_entry");
	gst_ui_entry_set_text (w0, txt);
	g_free (txt);
	
	/* set main group option menu */
	option_menu_add_groups (option_menu, FALSE);
	txt = get_group_name (gst_xml_get_child_content (ud->node, "gid"));

	if (txt != NULL) {
		element = g_list_first (groups_list);

		while ((element != NULL) && (strcmp (element->data, txt) != 0))
		{
			element = element->next;
			counter++;
		}
		gtk_option_menu_set_history (GTK_OPTION_MENU (gst_dialog_get_widget (tool->main_dialog, "user_settings_group")), counter);
		g_free (txt);
	}
	
	/* Fill groups lists */
	create_groups_gtk_trees ();
	create_groups_lists (ud->node, &user_settings_all_list, &user_settings_members_list);
	populate_gtk_tree_list (GTK_TREE_VIEW (user_settings_all), user_settings_all_list);
	populate_gtk_tree_list (GTK_TREE_VIEW (user_settings_members), user_settings_members_list);
	
	/* Attach the GLists to the GtkTreeViews */
	g_object_set_data (G_OBJECT (user_settings_all), "list", user_settings_all_list);
	g_object_set_data (G_OBJECT (user_settings_members), "list", user_settings_members_list);
	
	/* Set password */
	user_settings_dialog_prepare_password ();
	
	/* Set maxdays */
	txt = gst_xml_get_child_content (ud->node, "passwd_max_life");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_max");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	/* Set mindays */
	txt = gst_xml_get_child_content (ud->node, "passwd_min_life");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_min");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	/* Set warndays */
	txt = gst_xml_get_child_content (ud->node, "passwd_exp_warn");
	w0 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_days");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
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
