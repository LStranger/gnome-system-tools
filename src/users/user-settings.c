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

#include "xst.h"
#include "users-table.h"
#include "table.h"
#include "callbacks.h"
#include "user-group-xml.h"
#include "user-settings.h"
#include "user_group.h"
#include "passwd.h"

#define MAX_TOKENS 8

extern XstTool *tool;

static int reply;

GtkWidget *user_settings_all = NULL;
GtkWidget *user_settings_members = NULL;

static void
user_fill_groups_gtktree (GtkWidget *widget, xmlNodePtr node)
{
	GList *groups, *members, *items;
	
	members = user_get_groups (node);
	
	if (widget == user_settings_all) {
		groups = get_list_from_node ("name", node);
		items = my_g_list_remove_duplicates (groups, members);
	} else {
		items = members;
	}

	my_gtktree_list_append_items (GTK_TREE_VIEW (widget), items);
}

static void
create_groups_lists (void)
{
	GtkWidget *add_button, *remove_button;
	
	/* We create the widgets, connect signals and attach data if they haven't been created already */
	if (user_settings_all == NULL) {
		user_settings_all = create_gtktree_list (xst_dialog_get_widget (tool->main_dialog, "user_settings_all"));
		gtk_object_set_data (GTK_OBJECT (user_settings_all), "button", "user_settings_add");
		gtk_signal_connect (GTK_OBJECT (user_settings_all),
		                    "cursor_changed",
		                    G_CALLBACK (on_list_select_row),
		                    NULL);

		user_settings_members = create_gtktree_list (xst_dialog_get_widget (tool->main_dialog, "user_settings_members"));
		gtk_object_set_data (GTK_OBJECT (user_settings_members), "button", "user_settings_remove");
		gtk_signal_connect (GTK_OBJECT (user_settings_members),
		                    "cursor_changed",
		                    G_CALLBACK (on_list_select_row),
		                    NULL);
		
		/* We also need to attach some data to the 'add' and 'remove' buttons */
		add_button = xst_dialog_get_widget (tool->main_dialog, "user_settings_add");
		gtk_object_set_data (GTK_OBJECT (add_button), "in", user_settings_all);
		gtk_object_set_data (GTK_OBJECT (add_button), "out", user_settings_members);
		
		remove_button = xst_dialog_get_widget (tool->main_dialog, "user_settings_remove");
		gtk_object_set_data (GTK_OBJECT (remove_button), "in", user_settings_members);
		gtk_object_set_data (GTK_OBJECT (remove_button), "out", user_settings_all);
	}
}

static void
reply_cb (gint val, gpointer data)
{
        reply = val;
}

static gboolean
check_user_delete (xmlNodePtr node)
{
	gchar *login, *buf;
	GtkWindow *parent;
	GnomeDialog *dialog;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	login = xst_xml_get_child_content (node, "login");

	if (!login)
	{
		g_warning ("check_user_delete: Can't get username");
		return FALSE;
	}

	if (strcmp (login, "root") == 0)
	{
		buf = g_strdup (_("The root user must not be deleted."));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, parent));
		gnome_dialog_run (dialog);
		g_free (login);
		g_free (buf);
		return FALSE;
	}

	buf = g_strdup_printf (_("Are you sure you want to delete user %s?"), login);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (buf, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (buf);
	g_free (login);
	
	if (reply)
		return FALSE;
        else
		return TRUE;
}

void
delete_user (xmlNodePtr node)
{
	if (check_user_delete (node))
	{
		delete_selected_row (TABLE_USER);
		del_user_groups (node);
		xst_xml_element_destroy (node);
		xst_dialog_modify (tool->main_dialog);
		users_table_update_content ();
		actions_set_sensitive (TABLE_USER, FALSE);
	} 
}

static void
shells_combo_setup ()
{
	xmlNodePtr root, node;
	GtkWidget *combo;
	GList *shells = NULL;
	gchar *shell;

	root = xst_xml_doc_get_root (tool->config);
	root = xst_xml_element_find_first (root, "shelldb");
	combo = xst_dialog_get_widget (tool->main_dialog, "user_settings_shell");

	if (!root)
		return;
	
	for (node = xst_xml_element_find_first (root, "shell"); node != NULL; node = xst_xml_element_find_next (node, "shell"))
	{
		shell = xst_xml_element_get_content (node);
		shells = g_list_append (shells, shell);
	}

	gtk_combo_set_popdown_strings (GTK_COMBO (combo), shells);
	g_list_free (shells);
}

static void
groups_combo_setup (gchar *entry)
{
	GList *groups;
	GtkWidget *combo = xst_dialog_get_widget (tool->main_dialog, "user_settings_group_combo");
	gboolean found = FALSE;
	ug_data *ud;

	ud = gtk_object_get_data (GTK_OBJECT (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog")), "data");

	groups = get_list_from_node ("name", ud->node);
	
	g_return_if_fail (groups != NULL);
	
	if (entry) {
		GList *list = groups;
		while (list && !found) {
			if (strcmp (list->data, entry) == 0) {
				found = TRUE;
				break;
			}
			list = list->next;
		}
		
		if (!found) {
			groups = g_list_prepend (groups, entry);
		}
	}
	
	gtk_combo_set_popdown_strings (GTK_COMBO (combo), groups);
	gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);
	
	g_list_free (groups);
}

void
user_set_profile (const gchar *profile)
{
	GtkWidget *widget;
	xmlNodePtr node, root;
	gchar *buf;
	
	g_return_if_fail (profile != NULL);
	
	root = xst_xml_doc_get_root (tool->config);
	root = xst_xml_element_find_first (root, "profiledb");
	
	for (node = xst_xml_element_find_first (root, "profile"); node != NULL; xst_xml_element_find_next (node, "profile")) {
		if (strcmp (profile, xst_xml_get_child_content (node, "name")) == 0)
			break;
	}
	
	xst_ui_entry_set_text (xst_dialog_get_widget (tool->main_dialog, "user_settings_home"), xst_xml_get_child_content (node, "home_prefix"));
	xst_ui_entry_set_text (xst_dialog_get_widget (tool->main_dialog, "user_settings_shell_entry"), xst_xml_get_child_content (node, "shell"));
	
	buf = xst_xml_get_child_content (node, "group");
	groups_combo_setup (buf);
	xst_ui_entry_set_text (xst_dialog_get_widget (tool->main_dialog, "user_settings_group_combo_entry"), buf);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_max")), g_strtod (xst_xml_get_child_content (node, "pwd_maxdays"), NULL));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_min")), g_strtod (xst_xml_get_child_content (node, "pwd_mindays"), NULL));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_days")), g_strtod (xst_xml_get_child_content (node, "pwd_warndays"), NULL));
}

static void
profiles_option_menu_setup (void)
{
	GtkWidget *menu_item;
	GtkWidget *menu = xst_dialog_get_widget (tool->main_dialog, "user_settings_profile_menu");
	GList *profiles = get_profile_list ();
	GList *list = profiles;
	gint index, i=0;
	
	while (list) {
		if (strcmp (list->data, "Default") == 0)
			index = i;
		menu_item = xst_ui_option_menu_add_string (GTK_OPTION_MENU (menu), list->data);
		gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (on_user_settings_profile_changed), list->data);
		list = list->next;
		i++;
	}
	
	/* we set the option menu to the 'Default' option */
	gtk_option_menu_set_history (GTK_OPTION_MENU (menu), index);
	user_set_profile ("Default");

	g_list_free (profiles);
}

void
user_new_prepare (ug_data *ud)
{
	GtkWidget *widget, *button;
	gchar *buf;

	widget = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (widget), _("User Account Editor"));

	/* Attach the data to the dialog */
	gtk_object_set_data (GTK_OBJECT (widget), "data", ud);
	
	/* Profiles are disabled at the moment, showing advanced settings instead */
	gtk_widget_show (xst_dialog_get_widget (tool->main_dialog, "user_settings_advanced"));
	gtk_widget_hide (xst_dialog_get_widget (tool->main_dialog, "user_settings_profile_box"));

	/* Creates and fills GtkTrees */
	create_groups_lists ();
	user_fill_groups_gtktree (user_settings_all, ud->node);
	
	/* Fills profiles option menu */
	profiles_option_menu_setup ();
	
	/* Gets a new uid */
	buf = (gchar *) find_new_id (ud->node);
	if (buf) {
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_settings_uid")), 
		                           g_strtod (buf, NULL));
		g_free (buf);
	}
	
	/* If we have libcrack, password quality check is enabled */
	button = xst_dialog_get_widget (tool->main_dialog, "user_passwd_quality");
#ifdef HAVE_LIBCRACK
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
		widget = xst_dialog_get_widget (tool->main_dialog, text_boxes[i]);
		gtk_entry_set_text (GTK_ENTRY (widget), "");
	}

	/* Clear both lists */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_settings_all));
	gtk_tree_store_clear (GTK_TREE_STORE (model));

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_settings_members));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	/* Set unsensitive the add and remove buttons */
	widget = xst_dialog_get_widget (tool->main_dialog, "user_settings_add");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = xst_dialog_get_widget (tool->main_dialog, "user_settings_remove");
	gtk_widget_set_sensitive (widget, FALSE);
	
	/* Set the manual password */
	widget = xst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
	
	/* Set the first notebook page as default for the next time the dialog is opened */
	widget = xst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");
	gtk_notebook_set_page (GTK_NOTEBOOK (widget), 0);
	
	/* Clear user data attached to the widget */
	widget = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	ud = gtk_object_get_data (GTK_OBJECT (widget), "data");
	g_free (ud);
	gtk_object_remove_data (GTK_OBJECT (widget), "data");
	gtk_widget_hide (widget);
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
		login = xst_xml_get_child_content (node, "login");
	
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
	else if (strlen (login) > sizeof (ut.ut_user))
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
		gid = find_new_id (groupdb);
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
	
	data = g_new (UserAccountData, 1);
	
	/* check user login */
	data->login = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_name")));
	if (!is_login_valid (ud->node, data->login)) {
		return FALSE;
	}
	
	/* check user comments */
	data->name = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_comment")));
	data->location = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_office")));
	data->work_phone = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_wphone")));
	data->home_phone = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_hphone")));
	if (!is_comment_valid (data->name, data->location, data->work_phone, data->home_phone)) {
		return FALSE;
	}

	/* check user home */
	if (ud->is_new) {
		/* The user is new, home must be parsed to replace the '$user' tag */
		data->home = parse_user_home ((gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_home"))), data->login);
	} else {
		data->home = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_home")));
	}
	if (!is_home_valid (ud->node, data->home)) {
		return FALSE;
	}
	
	/* check user uid */
	data->uid = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_settings_uid"))));
	if (!is_user_uid_valid (ud->node, data->uid)) {
		return FALSE;
	}
	
	/* check user shell */
	data->shell = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_shell_entry")));
	if (!is_shell_valid (data->shell)) {
		return FALSE;
	}
	
	/* check user password */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_manual")))) {
		/* Manual password enabled */
		if (!ud->is_new) {
			/* If it is a modified user, we have to check that the entries are unchanged */
			gboolean d1, d2;
			d1 = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1")), "changed"));
			d2 = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2")), "changed"));
			if (d1 || d2) {
				data->password1 = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1")));
				data->password2 = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2")));
				if (!is_password_valid (data->password1, data->password2)) {
					return FALSE;
				}
				change_password = TRUE;
			}
		} else {
			/* If it is a new user, we have to check it anyway */
			data->password1 = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1")));
			data->password2 = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2")));
			if (!is_password_valid (data->password1, data->password2)) {
				return FALSE;
			}
		}
	} else {
		/* Automatic password generation enabled */
		data->password1 = (gchar *) gtk_label_get_text (GTK_LABEL (xst_dialog_get_widget (tool->main_dialog, "user_passwd_random_label")));
	}

	/* get user main group GID*/
	data->group = (gchar *) gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_group_combo_entry")));
	if (strcmp (data->group, "$user") == 0)
		data->group = data->login;
	
	data->gid = group_check (ud->node, data->group);
	if (!data->gid) {
		return FALSE;
	}
	
	data->extra_groups = extract_members_list (GTK_TREE_VIEW (user_settings_members));
	
	data->pwd_maxdays = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_max"))));
	data->pwd_mindays = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_min"))));
	data->pwd_warndays = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_days"))));
	
	if (ud->is_new) {
		/* Add new user, update table */
		xmlNodePtr node;
		
		node = user_add_blank_xml (ud->node);
		user_update_xml (node, data, TRUE);
	} else {
		/* Entered data ok, not new: just update */
		user_update_xml (ud->node, data, change_password);
	}
	
	tables_update_content ();
	
	return TRUE;
}

static void
user_settings_dialog_prepare_password (void){
	GtkWidget *pwd1, *pwd2;
	
	pwd1 = xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1");
	pwd2 = xst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2");
	
	gtk_entry_set_text (GTK_ENTRY (pwd1), "********");
	gtk_entry_set_text (GTK_ENTRY (pwd2), "********");
	
	gtk_object_set_data (GTK_OBJECT (pwd1), "changed", GINT_TO_POINTER (FALSE));
	gtk_object_set_data (GTK_OBJECT (pwd2), "changed", GINT_TO_POINTER (FALSE));
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
		w0 = xst_dialog_get_widget (tool->main_dialog, widgets[i]);
		xst_ui_entry_set_text (w0, buf[i]);
		g_free (buf[i]);
	}
}

static void
user_settings_dialog_prepare (ug_data *ud)
{
	GtkWidget *w0;
	gchar *txt, *name;
	
	g_return_if_fail (ud != NULL);

	/* Hides profiles combo, shows advanced settings */
	gtk_widget_show (xst_dialog_get_widget (tool->main_dialog, "user_settings_advanced"));
	gtk_widget_hide (xst_dialog_get_widget (tool->main_dialog, "user_settings_profile_box"));
	
	/* Attach the data to the dialog */
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	gtk_object_set_data (GTK_OBJECT (w0), "data", ud);

	/* Set user login */
	name = xst_xml_get_child_content (ud->node, "login");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_name");
	xst_ui_entry_set_text (w0, name);

	/* Set comments */
	user_settings_dialog_prepare_comments (xst_xml_get_child_content (ud->node, "comment"));
	
	/* Set UID */
	txt = xst_xml_get_child_content (ud->node, "uid");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	/* Set home directory */
	txt = xst_xml_get_child_content (ud->node, "home");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	xst_ui_entry_set_text (w0, txt);
	g_free (txt);
	
	/* Set shells combo */
	shells_combo_setup ();
	txt = xst_xml_get_child_content (ud->node, "shell");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_shell_entry");
	xst_ui_entry_set_text (w0, txt);
	g_free (txt);
	
	/* set main group combo */
	groups_combo_setup (NULL);
	txt = get_group_name (xst_xml_get_child_content (ud->node, "gid"));
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_group_combo_entry");
	xst_ui_entry_set_text (w0, txt);
	g_free (txt);
	
	/* Fill groups lists */
	create_groups_lists ();
	user_fill_groups_gtktree (user_settings_all, ud->node);
	user_fill_groups_gtktree (user_settings_members, ud->node);
	
	/* Set password */
	user_settings_dialog_prepare_password ();
	
	/* Set maxdays */
	txt = xst_xml_get_child_content (ud->node, "passwd_max_life");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_passwd_max");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	/* Set mindays */
	txt = xst_xml_get_child_content (ud->node, "passwd_min_life");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_passwd_min");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	/* Set warndays */
	txt = xst_xml_get_child_content (ud->node, "passwd_exp_warn");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_passwd_days");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));
	g_free (txt);
	
	/* Set window title */
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), g_strdup_printf (_("Settings for user %s"), name));
	g_free (name);

	gtk_widget_show (w0);
}

void
user_settings_prepare (ug_data *ud)
{
	gchar *buf;
	
	g_return_if_fail (ud != NULL);

	buf = xst_xml_get_child_content (ud->node, "name");

	if (buf)
	{
		/* Has to be some kind of group */
		g_free (buf);
		g_warning ("settings_prepare: Shouldn't be here");
		return;
	}

	buf = xst_xml_get_child_content (ud->node, "login");

	if (buf)
	{
		/* Has to be some kind of user */
		g_free (buf);

		user_settings_dialog_prepare (ud);
		return;
	}

	g_warning ("settings_prepare: shouldn't be here");
}
