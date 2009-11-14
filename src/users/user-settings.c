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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>,
 *          Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include "gst.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <utmp.h>
#include <ctype.h>

#include "users-table.h"
#include "table.h"
#include "callbacks.h"
#include "user-settings.h"
#include "privileges-table.h"
#include "groups-table.h"
#include "group-settings.h"
#include "test-battery.h"
#include "user-profiles.h"

extern GstTool *tool;

static gboolean
check_user_delete (OobsUser *user)
{
	GtkWidget *dialog;
	gint response;

	if (oobs_user_get_uid (user) == 0) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Administrator account cannot be deleted"));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("This would leave the system unusable."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return FALSE;
	}

	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("Are you sure you want to delete account \"%s\"?"),
					 oobs_user_get_login_name (user));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("This will disable this user's access to the system "
						    "without deleting the user's home directory."));

	if (oobs_user_get_active (user)) {
		GtkWidget *alignment, *box, *image, *label;
		gint image_width;

		image = gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
		gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &image_width, NULL);

		label = gtk_label_new (_("This user is currently using this computer"));
		gtk_label_set_selectable (GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (label), 0., 0.);

		box = gtk_hbox_new (FALSE, 12);
		gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);

		alignment = gtk_alignment_new (0., 0., 0., 0.);
		gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, image_width, 0);
		gtk_container_add (GTK_CONTAINER (alignment), box);
		gtk_widget_show_all (alignment);


		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), alignment, FALSE, FALSE, 0);
	}

	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				NULL);

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	gtk_widget_destroy (dialog);

	return (response == GTK_RESPONSE_ACCEPT);
}

gboolean
user_delete (GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	OobsUsersConfig *config;
	OobsUser *user;
	OobsList *users_list;
	OobsListIter *list_iter;
	gboolean retval = FALSE;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return FALSE;

	gtk_tree_model_get (model, &iter,
			    COL_USER_OBJECT, &user,
			    COL_USER_ITER, &list_iter,
			    -1);

	if (check_user_delete (user)) {
		config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
		users_list = oobs_users_config_get_users (config);
		oobs_list_remove (users_list, list_iter);

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		retval = TRUE;
	}

	g_object_unref (user);
	oobs_list_iter_free (list_iter);

	return retval;
}

static void
set_entry_text (GtkWidget *entry, const gchar *text)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (text) ? text : "");
}

static void
select_main_group (OobsUser *user)
{
	GtkWidget *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsGroup *main_group, *group;
	gboolean valid, found;

	combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");

	if (!user) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), -1);
		return;
	}

	main_group = oobs_user_get_main_group (user);
	found = FALSE;

	if (!main_group)
		main_group = oobs_users_config_get_default_group (OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config));

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid && !found) {
		gtk_tree_model_get (model, &iter,
				    COL_GROUP_OBJECT, &group,
				    -1);

		if (main_group == group) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
			found = TRUE;
		}

		g_object_unref (group);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	if (!found)
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), -1);
}

static gboolean
set_main_group (OobsUser *user)
{
	GtkWidget *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsGroup *group;
	OobsGroupsConfig *config;
	OobsList *groups_list;
	OobsListIter list_iter;
	const gchar *name;
	gboolean is_group_new = FALSE;


	name = oobs_user_get_login_name (user);
	combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter)) {
		group =	group_settings_get_group_from_name (name);
		/* Most standard case when creating user:
		 * no group using the login already exists, so create it */
		if (group == NULL) {
			is_group_new = TRUE;
			group = oobs_group_new (name);
			oobs_group_set_gid (group, group_settings_find_new_gid ());

			/* FIXME: this should be in a generic function */
			config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
			groups_list = oobs_groups_config_get_groups (config);
			oobs_list_append (groups_list, &list_iter);
			oobs_list_set (groups_list, &list_iter, group);
		}
		/* Else group exists, use it */
	}
	else { /* Group has been chosen in the list, use it */
		gtk_tree_model_get (model, &iter,
				    COL_GROUP_OBJECT, &group,
				    -1);
	}

	oobs_group_add_user (group, user);

	/* Try to commit before doing anything, to avoid displaying wrong data */
	if (gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config) == OOBS_RESULT_OK) {
		if (is_group_new)
			groups_table_add_group (group, &list_iter);

		/* This will be committed with the user himself */
		oobs_user_set_main_group (user, group);

		g_object_unref (group);
		return TRUE;
	}

	/* Something bad is happening, don't try to create the user.
	 * gst_tool_commit () must already have displayed an error. */
	return FALSE;
}

/* Retrieve the NO_PASSWD_LOGIN_GROUP.
 * Don't forget to unref the returned group when done. */
static OobsGroup *
get_no_passwd_login_group ()
{
	OobsGroupsConfig *config;
	OobsList *groups_list;
	OobsListIter iter;
	OobsGroup *group;
	gboolean valid;
	const gchar *group_name;
	gid_t gid;

	config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	groups_list = oobs_groups_config_get_groups (config);
	valid = oobs_list_get_iter_first (groups_list, &iter);

	while (valid) {
		group = OOBS_GROUP (oobs_list_get (groups_list, &iter));
		group_name = oobs_group_get_name (group);

		if (group_name && strcmp (NO_PASSWD_LOGIN_GROUP, group_name) == 0)
			return group;

		valid = oobs_list_iter_next (groups_list, &iter);
	}

	return NULL;
}

static gboolean
is_user_in_group (OobsUser  *user,
		  OobsGroup *group)
{
	OobsUser *tmp_user;
	GList *users = NULL;
	GList *l;

	if (!user || !group)
		return FALSE;

	users = oobs_group_get_users (group);
	for (l = users; l; l = l->next) {
		tmp_user = l->data;
		if (tmp_user == user)
			break;
	}
	g_list_free (users);

	return l != NULL;
}

static gboolean
is_user_root (OobsUser *user)
{
	const gchar *login;

	if (!user)
		return FALSE;

	login = oobs_user_get_login_name (user);

	if (!login)
		return FALSE;

	return (strcmp (login, "root") == 0);
}

static gboolean
uid_exists (uid_t uid)
{
	OobsUsersConfig *config;
	OobsList *list;
	OobsListIter list_iter;
	GObject *user;
	gboolean valid;
	uid_t user_uid;

	config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
	list = oobs_users_config_get_users (config);
	valid = oobs_list_get_iter_first (list, &list_iter);

	while (valid) {
		user = oobs_list_get (list, &list_iter);
		user_uid = oobs_user_get_uid (OOBS_USER (user));
		g_object_unref (user);

		if (user_uid == uid)
			return TRUE;

		valid = oobs_list_iter_next (list, &list_iter);
	}

	return FALSE;
}

static uid_t
find_new_uid (gint uid_min,
	      gint uid_max)
{
	OobsUsersConfig *config;
	OobsList *list;
	OobsListIter list_iter;
	GObject *user;
	gboolean valid;
	uid_t new_uid, uid;

	config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
	list = oobs_users_config_get_users (config);
	valid = oobs_list_get_iter_first (list, &list_iter);
	new_uid = uid_min - 1;

	while (valid) {
		user = oobs_list_get (list, &list_iter);
		uid = oobs_user_get_uid (OOBS_USER (user));
		g_object_unref (user);

		if (uid <= uid_max && uid >= uid_min && new_uid < uid)
			new_uid = uid;

		valid = oobs_list_iter_next (list, &list_iter);
	}

	new_uid++;

	return new_uid;
}

static void
set_login_length (GtkWidget *entry)
{
	gint max_len;
#ifdef __FreeBSD__
	max_len = UT_NAMESIZE;
#else
	struct utmp ut;

	max_len = sizeof (ut.ut_user);
#endif

	gtk_entry_set_max_length (GTK_ENTRY (entry), max_len);
}

GdkPixbuf *
user_settings_get_user_face (OobsUser *user, int size)
{
	GdkPixbuf *pixbuf;
	const gchar *homedir;
	gchar *face_path;

	homedir = oobs_user_get_home_directory (user);
	face_path = g_strdup_printf ("%s/.face", homedir);
	pixbuf = gdk_pixbuf_new_from_file_at_size (face_path, size, size, NULL);

	if (!pixbuf)
		pixbuf = gtk_icon_theme_load_icon (tool->icon_theme, "stock_person", size, 0, NULL);

	g_free (face_path);

	return pixbuf;
}

void
user_settings_set (OobsUser *user)
{
	OobsUsersConfig *config;
	OobsGroup *no_passwd_login_group;
	GtkWidget *dialog, *widget, *notice;
	const gchar *login = NULL;
	gchar *title;
	gint uid;
	GdkPixbuf *face;

	notice = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid_disabled");

	/* Set this before setting the UID so that it's not rejected */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (widget), 0, G_MAXINT32);

	if (!user) {
		config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
		set_entry_text (GTK_BIN (widget)->child,
				oobs_users_config_get_default_shell (config));

		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
		uid = find_new_uid (GST_USERS_TOOL (tool)->minimum_uid,
				    GST_USERS_TOOL (tool)->maximum_uid);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), uid);
		gst_dialog_try_set_sensitive (tool->main_dialog, widget, TRUE);
		gtk_widget_hide (notice);
	} else {
		login = oobs_user_get_login_name (user);

		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
		set_entry_text (GTK_BIN (widget)->child, oobs_user_get_shell (user));

		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), oobs_user_get_uid (user));
		/* Show a notice if the user is logged in,
		 * except if we don't have the required permissions to edit UID anyway */
		if (is_user_root (user)) {
			gst_dialog_try_set_sensitive (tool->main_dialog, widget, FALSE);
			gtk_widget_hide (notice);
		}
		else if (oobs_user_get_active (user) &&
			 gst_dialog_is_authenticated (tool->main_dialog)) {
			gst_dialog_try_set_sensitive (tool->main_dialog, widget, FALSE);
			gtk_widget_show (notice);
		}
		else {
			gst_dialog_try_set_sensitive (tool->main_dialog, widget, TRUE);
			gtk_widget_hide (notice);
		}
	}

	privileges_table_set_from_user (user);
	select_main_group (user);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_real_name");
	gtk_label_set_text (GTK_LABEL (widget),  oobs_user_get_full_name (user));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_face");
	face = user_settings_get_user_face (user, 60);
	gtk_image_set_from_pixbuf (GTK_IMAGE (widget), face);
	g_object_unref (face);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_room_number");
	set_entry_text (widget, (user) ? oobs_user_get_room_number (user) : NULL);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_wphone");
	set_entry_text (widget, (user) ? oobs_user_get_work_phone_number (user) : NULL);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_hphone");
	set_entry_text (widget, (user) ? oobs_user_get_home_phone_number (user) : NULL);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
	set_entry_text (widget, NULL);
	g_object_set_data (G_OBJECT (widget), "changed", GINT_TO_POINTER (FALSE));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd2");
	set_entry_text (widget, NULL);

	/* set always the first page */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);

	if (!login)
		table_set_default_profile (GST_USERS_TOOL (tool));

	if (user) {
		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
		set_entry_text (widget, oobs_user_get_home_directory (user));
	}
}

static gboolean
login_exists (const gchar *login)
{
	OobsUsersConfig *config;
	OobsList *users_list;
	OobsListIter iter;
	GObject *user;
	gboolean valid;
	const gchar *user_login;

	config = OOBS_USERS_CONFIG (GST_USERS_TOOL (tool)->users_config);
	users_list = oobs_users_config_get_users (config);
	valid = oobs_list_get_iter_first (users_list, &iter);

	while (valid) {
		user = oobs_list_get (users_list, &iter);
		user_login = oobs_user_get_login_name (OOBS_USER (user));
		g_object_unref (user);

		if (user_login && strcmp (login, user_login) == 0)
			return TRUE;

		valid = oobs_list_iter_next (users_list, &iter);
	}

	return FALSE;
}

/* FIXME: this function is duplicated in group-settings.c */
static gboolean
is_valid_name (const gchar *name)
{
	/*
	 * User/group names must start with a letter, and may not
	 * contain colons, commas, newlines (used in passwd/group
	 * files...) or any non-printable characters.
	 */
        if (!*name || !isalpha(*name))
                return FALSE;

        while (*name) {
		if (!isdigit (*name) && !islower (*name) && *name != '-')
                        return FALSE;
                name++;
        }

        return TRUE;
}

static void
check_login (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsUser *user;
	GtkWidget *widget;
	GtkWidget *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	const gchar *login;
	gchar *group;

	user = g_object_get_data (G_OBJECT (data), "user");

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_name");
	login = gtk_entry_get_text (GTK_ENTRY (widget));

	combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");

	if (strlen (login) < 1) {
		*primary_text = g_strdup (_("User name is empty"));
		*secondary_text = g_strdup (_("A user name must be specified."));
	} else if (!is_valid_name (login)) {
		*primary_text = g_strdup (_("User name has invalid characters"));
		*secondary_text = g_strdup (_("Please set a valid user name consisting of "
		                              "a lower case letter followed by lower case "
		                              "letters and numbers."));
	} else if (!user && login_exists (login)) {
		*primary_text = g_strdup_printf (_("User name \"%s\" already exists"), login);
		*secondary_text = g_strdup (_("Please choose a different user name."));
	/* Check that the new user won't be added to an already existing group
	 * without that group being explicitly chosen in the list (temporary security check) */
	} else if (!user && gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) == -1
	                 && group_settings_group_exists (login)) {
		*primary_text = g_strdup_printf (_("Group \"%s\" already exists"), login);
		*secondary_text = g_strdup (_("Please choose a different user name."));
	}
}

static void
check_comments (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	GtkWidget *name, *room_number, *wphone, *hphone;
	gchar *comment, *ch;

	name = gst_dialog_get_widget (tool->main_dialog, "user_settings_real_name");
	room_number = gst_dialog_get_widget (tool->main_dialog, "user_settings_room_number");
	wphone = gst_dialog_get_widget (tool->main_dialog, "user_settings_wphone");
	hphone = gst_dialog_get_widget (tool->main_dialog, "user_settings_hphone");

	comment = g_strjoin (" ",
			     gtk_entry_get_text (GTK_ENTRY (name)),
			     gtk_entry_get_text (GTK_ENTRY (room_number)),
			     gtk_entry_get_text (GTK_ENTRY (wphone)),
			     gtk_entry_get_text (GTK_ENTRY (hphone)),
			     NULL);

	if ((ch = g_utf8_strchr (comment, -1, ',')) ||
	    (ch = g_utf8_strchr (comment, -1, '=')) ||
	    (ch = g_utf8_strchr (comment, -1, ':'))) {
		*primary_text   = g_strdup_printf (_("Invalid character \"%c\" in comment"), *ch);
		*secondary_text = g_strdup (_("Check that this character is not used."));
	}

	g_free (comment);
}

static void
check_home (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsUser *user;
	GtkWidget *widget;
	const gchar *home;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	home = gtk_entry_get_text (GTK_ENTRY (widget));
	user = g_object_get_data (G_OBJECT (data), "user");

	if (strlen (home) < 1 || !g_path_is_absolute (home)) {
		*primary_text   = g_strdup (_("Incomplete path in home directory"));
		*secondary_text = g_strdup (_("Please enter full path for home directory\n"
					      "<span size=\"smaller\">i.e.: /home/john</span>."));
	} else if (!user && g_file_test (home, G_FILE_TEST_EXISTS)) {
		*primary_text   = g_strdup (_("Home directory already exists"));
		*secondary_text = g_strdup (_("Please enter a different home directory path."));
	}
}

static void
check_uid (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsUser *user;
	GtkWidget *widget;
	gint uid;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	uid = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
	user = g_object_get_data (G_OBJECT (data), "user");

	if (is_user_root (user) && uid != 0) {
		*primary_text   = g_strdup (_("Administrator account's user ID should not be modified"));
		*secondary_text = g_strdup (_("This would leave the system unusable."));
	}

	if (!user && uid_exists (uid)) {
		*primary_text   = g_strdup_printf (_("User ID %d is already used"), uid);
		*secondary_text = g_strdup (_("Please choose a different numeric identifier for the new user."));
	}
}

static void
check_shell (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	GtkWidget *widget;
	const gchar *path;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	path = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (widget)->child));

	if (strlen (path) < 1 || !g_path_is_absolute (path)) {
		*primary_text = g_strdup (_("Incomplete path in shell"));
		*secondary_text = g_strdup (_("Please enter full path for shell\n"
					      "<span size=\"smaller\">i.e.: /bin/bash</span>."));
	}
}

static void
check_password (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsUser *user;
	GtkWidget *widget;
	const gchar *password, *confirmation;
	gboolean changed = TRUE;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	user = g_object_get_data (G_OBJECT (data), "user");

	/* manual password? */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
		password = gtk_entry_get_text (GTK_ENTRY (widget));
		changed = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "changed"));

		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd2");
		confirmation = gtk_entry_get_text (GTK_ENTRY (widget));
	} else {
		widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_random_passwd");
		password = confirmation = gtk_entry_get_text (GTK_ENTRY (widget));
	}

	if (user && !changed)
		return;

	if (strlen (password) < 6) {
		*primary_text = g_strdup (_("Password is too short"));
		*secondary_text = g_strdup (_("User passwords must be longer than 6 characters and preferably "
					      "formed by numbers, letters and special characters."));
	} else if (strcmp (password, confirmation) != 0) {
		*primary_text = g_strdup (_("Password confirmation is not correct"));
		*secondary_text = g_strdup (_("Check that you have provided the same password in both text fields."));
	}
}

gint
user_settings_dialog_run (GtkWidget *dialog)
{
	gint response;
	gboolean valid;
	TestBattery battery[] = {
		check_comments,
		check_home,
		check_uid,
		check_shell,
		check_password,
		NULL
	};

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);

	do {
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		valid = (response == GTK_RESPONSE_OK) ?
			test_battery_run (battery, GTK_WINDOW (dialog), dialog) : TRUE;
	} while (!valid);

	gtk_widget_hide (dialog);
	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	return response;
}

OobsUser *
user_settings_dialog_get_data (GtkWidget *dialog)
{
	GtkWidget *widget;
	OobsGroup *group;
	OobsGroup *no_passwd_login_group;
	OobsGroupsConfig *groups_config;
	OobsList *groups_list;
	OobsListIter list_iter;
	OobsUser *user;
	const gchar *str;
	gboolean password_changed;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_name");
	str = gtk_entry_get_text (GTK_ENTRY (widget));
	user = g_object_get_data (G_OBJECT (dialog), "user");

	if (!str || !*str)
		return NULL;

	if (!user) {
		user = oobs_user_new (str);
		g_object_set_data_full (G_OBJECT (dialog), "user",
					user, (GDestroyNotify) g_object_unref);
	}

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_real_name");
	oobs_user_set_full_name (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_room_number");
	oobs_user_set_room_number (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_wphone");
	oobs_user_set_work_phone_number (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_hphone");
	oobs_user_set_home_phone_number (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	oobs_user_set_shell (user, gtk_entry_get_text (GTK_ENTRY (GTK_BIN (widget)->child)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	oobs_user_set_home_directory (user, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	oobs_user_set_uid (user, gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));

	/* If FALSE, group could not be found or created, which means
	 * we won't be able to create the new user anyway, so stop here */
	if (!set_main_group (user))
		return NULL;

	privileges_table_save (user);

	return user;
}

void
user_settings_apply_profile (GstUsersTool   *users_tool,
			     GstUserProfile *profile)
{
	GstTool *tool;
	GtkWidget *widget;
	gint uid;

	if (!profile)
		return;

	tool = GST_TOOL (users_tool);

	/* default UID */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	uid = find_new_uid (profile->uid_min, profile->uid_max);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), uid);

	/* default shell */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	set_entry_text (GTK_BIN (widget)->child, profile->shell);

	/* default home prefix */
	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	g_object_set_data (G_OBJECT (widget), "default-home", profile->home_prefix);

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_name");
	on_user_settings_login_changed (GTK_EDITABLE (widget), NULL);

	/* default groups */
	privileges_table_set_from_profile (profile);
}


/*
 * Common to all modular edit dialogs: run the dialog after filling
 * the user's name and face and handling window settings.
 */
static int
run_edit_dialog (GtkDialog *dialog, GtkImage *face_image2, GtkLabel *name_label)
{
	OobsUser *user;
	GtkWidget *face_image1;
	GdkPixbuf *face;
	const char *name;
	int response;

	/* Set user face from the main dialog image */
	face_image1 = gst_dialog_get_widget (tool->main_dialog, "user_settings_face");
	face = gtk_image_get_pixbuf (GTK_IMAGE (face_image1));
	gtk_image_set_from_pixbuf (face_image2, face);

	/* Set user name */
	user = users_table_get_current ();
	name = oobs_user_get_full_name (user);
	gtk_label_set_text (name_label, name);
	g_object_unref (user);

	/* Run dialog with correct settings */
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
	                              GTK_WINDOW (tool->main_dialog));
	gst_dialog_add_edit_dialog (tool->main_dialog, GTK_WIDGET (dialog));
	response = gtk_dialog_run (dialog);
	gst_dialog_remove_edit_dialog (tool->main_dialog, GTK_WIDGET (dialog));
	gtk_widget_hide (GTK_WIDGET (dialog));

	return response;
}

/*
 * Callback for edit_user_name_button: run the dialog to change the user's
 * real name and apply changes if needed.
 */
void
on_edit_user_name (GtkButton *button, gpointer user_data)
{
	int response;
	GtkWidget *user_name_dialog;
	GtkWidget *user_name_entry;
	GtkWidget *face_image;
	GtkWidget *name_label;
	OobsUser *user;
	const char *fullname;

	user_name_dialog = gst_dialog_get_widget (tool->main_dialog, "user_name_dialog");
	user_name_entry = gst_dialog_get_widget (tool->main_dialog, "user_name_entry");
	face_image = gst_dialog_get_widget (tool->main_dialog, "user_name_face");
	name_label = gst_dialog_get_widget (tool->main_dialog, "user_name_name");

	user = users_table_get_current ();
	fullname = oobs_user_get_full_name (user);
	gtk_entry_set_text (GTK_ENTRY (user_name_entry), fullname);
	gtk_editable_select_region (GTK_EDITABLE (user_name_entry), 0, -1);

	response = run_edit_dialog (GTK_DIALOG (user_name_dialog),
	                            GTK_IMAGE (face_image), GTK_LABEL (name_label));

	if (response == GTK_RESPONSE_OK) {
		user_name_entry = gst_dialog_get_widget (tool->main_dialog,
		                                         "user_name_entry");

		fullname = gtk_entry_get_text (GTK_ENTRY (user_name_entry));
		oobs_user_set_full_name (user, fullname);
		gst_tool_commit (tool, GST_USERS_TOOL (tool)->users_config);
	}

	g_object_unref (user);
}

/*
 * Callback for edit_user_name_button: run the dialog to change the user's
 * password and apply changes if needed.
 */
void
on_edit_user_passwd (GtkButton *button, gpointer user_data)
{
	int response;
	GtkWidget *user_passwd_dialog;
	GtkWidget *face_image;
	GtkWidget *name_label;
	GtkWidget *manual_toggle;
	GtkWidget *passwd_entry;
	GtkWidget *nocheck_toggle;
	gboolean passwd_changed;
	OobsUser *user;
	OobsGroup *no_passwd_login_group;

	user_passwd_dialog = gst_dialog_get_widget (tool->main_dialog, "user_passwd_dialog");
	face_image = gst_dialog_get_widget (tool->main_dialog, "user_passwd_face");
	name_label = gst_dialog_get_widget (tool->main_dialog, "user_passwd_name");

	user = users_table_get_current ();

	/* set manual password */
	manual_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (manual_toggle), TRUE);

	/* set option to skip password check at login */
	nocheck_toggle = gst_dialog_get_widget (tool->main_dialog, "user_passwd_no_check");
	no_passwd_login_group = get_no_passwd_login_group ();
	/* root should not be allowed to login without password,
	 * and we disable the feature if the group does not exist */
	if (is_user_root (user) || no_passwd_login_group == NULL) {
		gtk_widget_set_sensitive (nocheck_toggle, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocheck_toggle),
		                              FALSE);
	}
	else {
		gst_dialog_try_set_sensitive (tool->main_dialog, nocheck_toggle, TRUE);
		if (is_user_in_group (user, no_passwd_login_group))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocheck_toggle),
			                              TRUE);
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocheck_toggle),
			                              FALSE);
	}
	if (no_passwd_login_group)
		g_object_unref (no_passwd_login_group);


	response = run_edit_dialog (GTK_DIALOG (user_passwd_dialog),
	                            GTK_IMAGE (face_image), GTK_LABEL (name_label));

	if (response != GTK_RESPONSE_OK){
		g_object_unref (user);
		return;
	}

	/* set manual or random password if needed */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (manual_toggle))) {
		passwd_entry = gst_dialog_get_widget (tool->main_dialog,
		                                      "user_settings_passwd1");
		passwd_changed =
			GPOINTER_TO_INT (g_object_get_data (G_OBJECT (passwd_entry),
			                                    "changed"));

		if (passwd_changed)
			oobs_user_set_password (user,
			                        gtk_entry_get_text (GTK_ENTRY (passwd_entry)));
	} else {
		passwd_entry = gst_dialog_get_widget (tool->main_dialog,
		                                      "user_settings_random_passwd");
		oobs_user_set_password (user,
		                        gtk_entry_get_text (GTK_ENTRY (passwd_entry)));
	}

	/* check whether user is allowed to login without password */
	no_passwd_login_group = get_no_passwd_login_group ();
	if (!is_user_root (user) && no_passwd_login_group != NULL) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nocheck_toggle)))
			oobs_group_add_user (no_passwd_login_group, user);
		else
			oobs_group_remove_user (no_passwd_login_group, user);
	}
	if (no_passwd_login_group)
		g_object_unref (no_passwd_login_group);

	/* commit both user and groups config
	 * because of the no_passwd_login_group membership */
	if (gst_tool_commit (tool, GST_USERS_TOOL (tool)->users_config) == OOBS_RESULT_OK)
		gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);

	g_object_unref (user);
}

/*
 * Callback for edit_user_profile_button: run the dialog to change the user's
 * account type and apply changes if needed.
 */
void
on_edit_user_profile (GtkButton *button, gpointer user_data)
{
	int response;
	GtkWidget *user_profile_dialog;
	GtkWidget *user_profile_entry;
	GtkWidget *face_image;
	GtkWidget *name_label;
	OobsUser *user;

	user_profile_dialog = gst_dialog_get_widget (tool->main_dialog, "user_profile_dialog");
	face_image = gst_dialog_get_widget (tool->main_dialog, "user_profile_face");
	name_label = gst_dialog_get_widget (tool->main_dialog, "user_profile_name");

	user = users_table_get_current ();

	response = run_edit_dialog (GTK_DIALOG (user_profile_dialog),
	                            GTK_IMAGE (face_image), GTK_LABEL (name_label));

	if (response == GTK_RESPONSE_OK) {
		gst_tool_commit (tool, GST_USERS_TOOL (tool)->users_config);
	}

	g_object_unref (user);
}
