/* user_group.c: this file is part of users-admin, a ximian-setup-tool frontend 
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

#include <stdlib.h>
#include <gnome.h>

#include "global.h"
#include "callbacks.h"
#include "user_group.h"
#include "transfer.h"
#include "e-table.h"

extern XstTool *tool;

/* Local globals */
static int reply;

login_defs logindefs;

/* Static prototypes */

static void user_settings_prepare (ug_data *ud);
static void group_settings_prepare (ug_data *ud);
static void adv_user_new (xmlNodePtr node);
static xmlNodePtr user_add_blank_xml (xmlNodePtr parent);
static void user_update_xml (xmlNodePtr node, gboolean adv);
static void group_update_users (xmlNodePtr node, gchar *old_name, gchar *new_name);
static xmlNodePtr group_add_blank_xml (xmlNodePtr db_node);
static void group_add_from_user (ug_data *ud);
static void group_update_xml (xmlNodePtr node, gboolean adv);
static gboolean node_exsists (xmlNodePtr node, gchar *name, gchar *val);
static GList *get_group_list (gchar *field, xmlNodePtr node, gboolean adv);
static GList *get_user_list (gchar *field, xmlNodePtr node, gboolean adv);
static GList *get_group_users (xmlNodePtr node);
static void user_fill_settings_group (GtkCombo *combo, xmlNodePtr node, gboolean adv);
static GList *group_fill_members_list (xmlNodePtr node);
static void group_fill_all_users_list (xmlNodePtr node, GList *exclude);
static void del_group_users (xmlNodePtr node);
static void add_group_users (xmlNodePtr node, gchar *name);
static gboolean is_valid_name (gchar *str);
static gchar *find_new_id (xmlNodePtr parent);
static gchar *find_new_key (xmlNodePtr parent);
static void reply_cb (gint val, gpointer data);
static gint char_sort_func (gconstpointer a, gconstpointer b);

/* Global functions */

xmlNodePtr
get_root_node (gint tbl)
{
	xmlNodePtr node, root;

	root = xml_doc_get_root (tool->config);

	switch (tbl)
	{
	case TABLE_USER:
		node = xml_element_find_first (root, "userdb");
		break;
	case TABLE_GROUP:
		node = xml_element_find_first (root, "groupdb");
		break;
	case TABLE_NET_USER:
		node = xml_element_find_first (root, "nis_userdb");
		break;
	case TABLE_NET_GROUP:
		node = xml_element_find_first (root, "nis_groupdb");
		break;
	default:
		node = NULL;
		break;
	}

	return node;
}

xmlNodePtr
get_user_root_node (void)
{
	return xml_element_find_first (xml_doc_get_root (tool->config), "userdb");
}

xmlNodePtr
get_group_root_node (void)
{
	return xml_element_find_first (xml_doc_get_root (tool->config), "groupdb");
}

xmlNodePtr
get_nis_group_root_node (void)
{
	return xml_element_find_first (xml_doc_get_root (tool->config), "nis_groupdb");
}

xmlNodePtr
get_nis_user_root_node (void)
{
	return xml_element_find_first (xml_doc_get_root (tool->config), "nis_userdb");
}

xmlNodePtr
get_db_node (xmlNodePtr node)
{
	while (node)
	{
		if (strstr (node->name, "db"))
			return node;

		node = node->parent;
	}

	return NULL;
}

void
adv_user_settings (xmlNodePtr node, gboolean show)
{
	GtkWidget *win, *adv, *w0;
	gchar *content;

	win = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	adv = xst_dialog_get_widget (tool->main_dialog, "user_settings_advanced");

	if (show)
	{
		g_return_if_fail (node != NULL);

		/* Shell. */
		w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
		content = xml_get_child_content (node, "shell");
		gtk_entry_set_text (GTK_ENTRY (w0), content);
		g_free (content);
		gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));

		/* Home dir. */
		w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_home");
		content = xml_get_child_content (node, "home");
		gtk_entry_set_text (GTK_ENTRY (w0), content);
		g_free (content);
		gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));

		/* User ID. */
		w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
		content = xml_get_child_content (node, "uid");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (content, NULL));
		g_free (content);
		gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));

		/* Show advanced frame. */
		gtk_widget_show (adv);
	}
	else
	{
		/* Clear home, shell and user id. */
		gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												    "user_settings_shell")), "");
		
		gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												    "user_settings_home")), "");
		
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (
			xst_dialog_get_widget (tool->main_dialog, "user_settings_uid")), 0);

		/* Hide advanced frame. */
		gtk_widget_hide (adv);
	}
}

gboolean
check_node_complexity (xmlNodePtr node)
{
	xmlNodePtr db_node;
	gchar *field, *content;
	gint min, max, val;

	db_node = get_db_node (node);
	get_min_max (db_node, &min, &max);

	if (!strcmp (db_node->name, "userdb"))
		field = g_strdup ("uid");

	else if (!strcmp (db_node->name, "groupdb"))
		field = g_strdup ("gid");

	else
		return TRUE;

	node = xml_element_find_first (node, field);
	g_free (field);
	content = xml_element_get_content (node);
	val = atoi (content);
	g_free (content);

	if (val >= min && val <= max)
		return TRUE;

	else
		return FALSE;
}

gboolean
check_user_login (xmlNodePtr node, gchar *login)
{
	gchar *buf = NULL;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (login != NULL, FALSE);

	/* If !empty. */
	if (strlen (login) < 1)
		buf = g_strdup (_("The username is empty."));

	/* if valid. */
	else if (!is_valid_name (login))
		buf = g_strdup (_("Please set a valid username, using only lower-case letters."));

	/* if !exsist. */
	else if (node_exsists (node, "login", login))
		buf = g_strdup (_("Username already exsists."));

	/* If anything is wrong. */
	if (buf)
	{
		GtkWindow *win;
		GnomeDialog *dialog;

		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, win));
		gnome_dialog_run (dialog);
		g_free (buf);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_name"));

		return FALSE;
	}

	else
		return TRUE;
}

gboolean
check_user_uid (xmlNodePtr node, gchar *val)
{
	gboolean retval = TRUE;
	GtkWindow *win;
	GnomeDialog *dialog;

	/* Check if uid is available */

	if (node_exsists (node, "uid", val))
	{
		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
				(_("Such user id already exsists."), win));

		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_uid"));
		retval = FALSE;
	}

	return retval;
}

gboolean
check_user_comment (xmlNodePtr node, gchar *val)
{
	/* What could be wrong with comment? */
	return TRUE;
}

gboolean
check_user_home (xmlNodePtr node, gchar *val)
{
	GtkWindow *win;
	GnomeDialog *dialog;

	/* I think every user has to have home dir... FIXME, if I'm wrong. */

	if (strlen (val) < 1)
	{
		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
				(_("Home directory must not be empty."), win));

		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_home"));
		return FALSE;
	}

	return TRUE;
}

gboolean
check_user_shell (xmlNodePtr node, gchar *val)
{
	/* TODO: check shell. */

	return TRUE;
}

gint
check_user_group (xmlNodePtr node)
{
	GtkCombo *combo;
	gchar *buf;
	GtkWindow *win;
	GnomeDialog *dialog;
	xmlNodePtr group_node;

	g_return_val_if_fail (node != NULL, -1);

	combo = GTK_COMBO (xst_dialog_get_widget (tool->main_dialog, "user_settings_group"));
	buf = gtk_editable_get_chars (GTK_EDITABLE (combo->entry), 0, -1);

	group_node = get_corresp_field (node);

	/* We have to give childs to node_exsists, cause it gets parent inside */
	if (node_exsists (group_node->childs, "name", buf))
		return 0;

	if (!is_valid_name (buf))
	{
		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented(_(
				"Please set a valid main group name, with only lower-case letters,"
				"\nor select one from pull-down menu."), win));

		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (GTK_WIDGET (combo));

		return -1;
	}

	/* Group not found, but name is valid. */
	return 1;
}

gboolean
check_group_name (xmlNodePtr node, gchar *name)
{
	gchar *buf = NULL;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	/* If !empty. */
	if (strlen (name) < 1)
		buf = g_strdup (_("Group name is empty."));

	/* if valid. */
	else if (!is_valid_name (name))
		buf = g_strdup (_("Please set a valid group name, using only lower-case letters."));

	/* if !exsist. */
	else if (node_exsists (node, "name", name))
		buf = g_strdup (_("Group already exsists."));

	/* If anything is wrong. */
	if (buf)
	{
		GtkWindow *win;
		GnomeDialog *dialog;

		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, win));
		gnome_dialog_run (dialog);
		g_free (buf);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "group_settings_name"));

		return FALSE;
	}

	else
		return TRUE;
}

gboolean
check_group_gid (xmlNodePtr node, gchar *val)
{
	gboolean retval = TRUE;
	GtkWindow *win;
	GnomeDialog *dialog;

	/* Check if gid is available */

	if (node_exsists (node, "gid", val))
	{
		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
				(_("Such group id already exsists."), win));

		gnome_dialog_run (dialog);
		retval = FALSE;
	}

	return retval;
}

gboolean
get_min_max (xmlNodePtr db_node, gint *min, gint *max)
{
	g_return_val_if_fail (db_node != NULL, FALSE);

	if (!strcmp (db_node->name, "userdb"))
	{
		*min = logindefs.new_user_min_id;
		*max = logindefs.new_user_max_id;

		return TRUE;
	}

	if (!strcmp (db_node->name, "groupdb"))
	{
		*min = logindefs.new_group_min_id;
		*max = logindefs.new_group_max_id;

		return TRUE;
	}

	else
	{
		/* What is that? let's put min very small and max very BIG */
		*min = 0;
		*max = 100000;
		return TRUE;
	}

	return FALSE;
}

xmlNodePtr
get_corresp_field (xmlNodePtr node)
{
	xmlNodePtr root = xml_doc_get_root (tool->config);
	
	node = get_db_node (node);

	if (!strcmp (node->name, "userdb"))
		return xml_element_find_first (root, "groupdb");

	if (!strcmp (node->name, "groupdb"))
		return xml_element_find_first (root, "userdb");

	if (!strcmp (node->name, "nis_groupdb"))
		return xml_element_find_first (root, "nis_userdb");

	if (!strcmp (node->name, "nis_userdb"))
		return xml_element_find_first (root, "nis_groupdb");

	return NULL;
}

xmlNodePtr
get_node_by_data (xmlNodePtr dbnode, gchar *field, gchar *fdata)
{
	xmlNodePtr node;
	gchar *buf;

	g_return_val_if_fail (dbnode != NULL, NULL);
	g_return_val_if_fail (field != NULL, NULL);
	g_return_val_if_fail (fdata != NULL, NULL);

	for (node = dbnode->childs; node; node = node->next)
	{
		buf = xml_get_child_content (node, field);
		if (!buf)
			continue;
		
		if (!strcmp (buf, fdata))
		{
			g_free (buf);
			return node;
		}

		g_free (buf);
	}

	return NULL;
}


/* Extern functions */

/* User related */

extern void
settings_prepare (ug_data *ud)
{
	gchar *buf;
	
	g_return_if_fail (ud != NULL);

	buf = xml_get_child_content (ud->node, "login");

	if (buf)
	{
		/* Has to be some kind of user */
		g_free (buf);

		user_settings_prepare (ud);
		return;
	}

	buf = xml_get_child_content (ud->node, "name");

	if (buf)
	{
		/* Has to be some kind of group */
		g_free (buf);

		group_settings_prepare (ud);
		return;
	}

	g_warning ("settings_prepare: shouldn't be here");
}

extern void
user_new_prepare (ug_data *ud)
{
	GtkWidget *w0;
	gboolean adv;

	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);

	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	user_fill_settings_group (GTK_COMBO (w0), ud->node, adv);

	if (adv)
		adv_user_new (ud->node);

	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), "Create New User");
	gtk_object_set_data (GTK_OBJECT (w0), "data", ud);
	gtk_widget_show (w0);
}

extern gboolean
user_update (ug_data *ud)
{
	gboolean ok = TRUE;
	gboolean adv;
	gint group;
	gchar *buf;
	gchar *old_name, *new_name;

	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);

	new_name = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
													"user_settings_name")));
	old_name = xml_get_child_content (ud->node, "login");
	
	if (!check_user_login (ud->node, new_name))
		ok = FALSE;

	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
													"user_settings_comment")));
	if (!check_user_comment (ud->node, buf))
		ok = FALSE;

	group = check_user_group (ud->node);
	switch (group)
	{
		case 0:
			/* Group exsisted, everything is ok. */
			break;
		case 1:
			/* Make new group */
			if (ok)
				group_add_from_user (ud);
			break;
		case -1:
		default:
			/* Error. */
			ok = FALSE;
			break;
	}

	if (adv)
	{
		GtkSpinButton *spin = GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_settings_uid"));
		gint i = gtk_spin_button_get_value_as_int (spin);

		buf = g_strdup_printf ("%d", i);
		if (!check_user_uid (ud->node, buf))
			ok = FALSE;

		g_free (buf);

		buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_home")));
		if (!check_user_home (ud->node, buf))
			ok = FALSE;

		buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "user_settings_shell")));
		if (!check_user_shell (ud->node, buf))
			ok = FALSE;
	}

	if (ok)
	{
		if (ud->new)
		{
			/* Add new user, update table. */
			ud->node = user_add_blank_xml (ud->node);
			user_update_xml (ud->node, adv);
			current_table_new_row (ud);

			/* Ask for password too */
			user_passwd_dialog_prepare (ud->node);

			return ok;
		}

		else
		{
			/* Entered data ok, not new: just update. */
			user_update_xml (ud->node, adv);
			group_update_users (ud->node, old_name, new_name);
			current_table_update_row (ud);

			g_free (new_name);
			
			return ok;
		}
	}

	return FALSE;
}

extern void
user_passwd_dialog_prepare (xmlNodePtr node)
{
	GtkWidget *w0;
	gchar *txt, *name;

	name = xml_get_child_content (node, "login");

	w0 = xst_dialog_get_widget (tool->main_dialog, "user_passwd_dialog");
	txt = g_strdup_printf (_("Password for User %s"), name);
	g_free (name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);
	gtk_widget_show (w0);
	gtk_object_set_data (GTK_OBJECT (w0), "name", node);

#ifndef HAVE_LIBCRACK
	gtk_widget_hide (xst_dialog_get_widget (tool->main_dialog, "user_passwd_quality"));
#endif
}

extern gboolean
check_login_delete (xmlNodePtr node)
{
	gchar *name, *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	name = xml_get_child_content (node, "login");

	if (!strcmp (name, "root"))
	{
		g_free (name);
		txt = g_strdup (_("The root user must not be deleted."));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
		gnome_dialog_run (dialog);
		g_free (txt);
		return FALSE;
	}

	txt = g_strdup_printf (_("Are you sure you want to delete user %s?"), name);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);
	g_free (name);

	if (reply)
		return FALSE;
        else
		return TRUE;
}

/* Group related externs. */

extern void
group_new_prepare (ug_data *ud)
{
	GtkWidget *w0;

	/* Fill all users list, don't exclude anything */
	group_fill_all_users_list (ud->node, NULL);

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), _("Create New Group"));
	gtk_object_set_data (GTK_OBJECT (w0), "data", ud);
	gtk_widget_show (w0);
}

extern gboolean
group_update (ug_data *ud)
{
	gboolean ok = TRUE;
	gboolean adv;
	gchar *buf;

	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);


	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "group_settings_name")));
	if (!check_group_name (ud->node, buf))
		ok = FALSE;

	if (ok)
	{
		if (ud->new)
		{
			/* Add new group, update table. */
			ud->node = group_add_blank_xml (ud->node);
			group_update_xml (ud->node, adv);
			current_table_new_row (ud);

			return ok;
		}

		else
		{
			/* Entered data ok, not new: just update */
			group_update_xml (ud->node, adv);
			current_table_update_row (ud);
			return ok;
		}
	}

	return FALSE;
}

extern gboolean
check_group_delete (xmlNodePtr node)
{
	gchar *name, *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	name = xml_get_child_content (node, "name");

	if (!name)
	{
		g_warning ("check_group_delete: Can't get group's name");
		return FALSE;
	}

	if (!strcmp (name, "root"))
	{
		g_free (name);
		txt = g_strdup (_("The root group must not be deleted."));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
		gnome_dialog_run (dialog);
		g_free (txt);
		return FALSE;
	}

	txt = g_strdup_printf (_("Are you sure you want to delete group %s?"), name);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);
	g_free (name);

	if (reply)
		return FALSE;
        else
		return TRUE;
}


/* Static functions */

static void
user_settings_prepare (ug_data *ud)
{
	GtkWidget *w0;
	GList *tmp_list;
	gchar *txt;
	gboolean found = FALSE;
	gchar *login, *comment, *name = NULL;
	gint gid, id = 0;
	gint new_id = 0;
	gboolean adv;
	GtkRequisition req;

	g_return_if_fail (ud != NULL);
	g_return_if_fail (login = xml_get_child_content (ud->node, "login"));

	/* Get tool state (advanced/basic) */
	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);

	/* Fill login name entry */
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_name");
	gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));
	my_gtk_entry_set_text (w0, login);
	g_free (login);

	/* Fill groups combo, use node->parent to pass <userdb> */
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));
	user_fill_settings_group (GTK_COMBO (w0), ud->node, adv);

	txt = xml_get_child_content (ud->node, "gid");
	gid = atoi (txt);
	g_free (txt);

	tmp_list = get_group_list ("gid", ud->node, adv);
	while (tmp_list)
	{
		id = atoi (tmp_list->data);
		g_free (tmp_list->data);
		tmp_list = tmp_list->next;

		if (!found && id == gid)
		{
			new_id = id;
			found = TRUE;
		}
	}
	g_list_free (tmp_list);

	if (!found)
	{
		g_warning ("The GID for the main user's group was not found.");
		name = g_strdup (_("Unkown User"));
	}
	else
	{
		xmlNodePtr group_node;

		txt = g_strdup_printf ("%d", new_id);
		group_node = get_corresp_field (get_db_node (ud->node));
		group_node = get_node_by_data (group_node, "gid", txt);
		name = xml_get_child_content (group_node, "name");
		my_gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w0)->entry), name);
	}

	/* Fill comment entry */
	comment = xml_get_child_content (ud->node, "comment");
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_comment");
	gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));
	my_gtk_entry_set_text (w0, comment);
	g_free (comment);

	/* If state == advanced, fill advanced settings too. */
	if (adv)
		adv_user_settings (ud->node, TRUE);

	/* Set dialog's title and show it */
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	txt = g_strdup_printf (_("Settings for User %s"), name);
	g_free (name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);

	/* Resize it to minimum */
	gtk_widget_size_request (w0, &req);
	gtk_window_set_default_size (GTK_WINDOW (w0), req.width, req.height);

	gtk_object_set_data (GTK_OBJECT (w0), "data", ud);

	gtk_widget_show (w0);
}

static void
group_settings_prepare (ug_data *ud)
{
	GtkWidget *w0;
	GList *member_rows;
	gchar *txt, *name;

	g_return_if_fail (ud != NULL);
	g_return_if_fail (name = xml_get_child_content (ud->node, "name"));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));
	my_gtk_entry_set_text (w0, name);

	/* Fill group members */
	member_rows = group_fill_members_list (ud->node);

	/* Fill all users list */
	group_fill_all_users_list (ud->node, member_rows);

	while (member_rows)
	{
		g_free (member_rows->data);
		member_rows = member_rows->next;
	}
	g_list_free (member_rows);

	/* Show group settings dialog */

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	txt = g_strdup_printf (_("Settings for Group %s"), name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (name);
	g_free (txt);

	gtk_object_set_data (GTK_OBJECT (w0), "data", ud);
	gtk_widget_show (w0);
}

static void
adv_user_new (xmlNodePtr node)
{
	GtkWidget *w0;
	gfloat uid;

	/* Set new first available UID */
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_uid");
	uid = g_strtod (find_new_id (node), NULL);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), uid);

	/* Set bash as the default shell. */
	/* FIXME: Maybe search for the first desirable shell for other systems */
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_shell");
	gtk_entry_set_text (GTK_ENTRY (w0), "/bin/bash");

	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_advanced");
	gtk_widget_show (w0);
}

static xmlNodePtr
user_add_blank_xml (xmlNodePtr parent)
{
	xmlNodePtr user;

	g_return_val_if_fail (parent != NULL, NULL);

	user = xml_element_add (parent, "user");

	xml_element_add_with_content (user, "key", find_new_key (parent));
	xml_element_add (user, "login");
	xml_element_add (user, "password");
	xml_element_add_with_content (user, "uid", find_new_id (parent));
	xml_element_add (user, "gid");
	xml_element_add (user, "comment");
	xml_element_add (user, "home");
	xml_element_add (user, "shell");
	xml_element_add (user, "last_mod");

	xml_element_add_with_content (user, "passwd_min_life",
			g_strdup_printf ("%d", logindefs.passwd_min_day_use));

	xml_element_add_with_content (user, "passwd_max_life",
			g_strdup_printf ("%d", logindefs.passwd_max_day_use));

	xml_element_add_with_content (user, "passwd_exp_warn",
			g_strdup_printf ("%d", logindefs.passwd_warning_advance_days));

	xml_element_add (user, "passwd_exp_disable");
	xml_element_add (user, "passwd_disable");
	xml_element_add (user, "reserved");
	xml_element_add_with_content (user, "is_shadow", g_strdup ("1"));

	return user;
}

static void
user_update_xml (xmlNodePtr node, gboolean adv)
{
	gchar *buf;
	gint id;
	xmlNodePtr group_node;

	/* Login */
	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
													"user_settings_name")));
	
	xml_set_child_content (node, "login", buf);

	/* Comment */
	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
													"user_settings_comment")));
	
	xml_set_child_content (node, "comment", buf);

	/* Group */
	buf = gtk_entry_get_text (
			GTK_ENTRY (GTK_COMBO (xst_dialog_get_widget (tool->main_dialog,
												"user_settings_group"))->entry));

	group_node = get_corresp_field (get_db_node (node));
	group_node = get_node_by_data (group_node, "name", buf);
	buf = xml_get_child_content (group_node, "gid");
	
	xml_set_child_content (node, "gid", buf);

	/* TODO hardcoded default shell and home dir prefix are BAD */
	/* Home */
	buf = adv ?
		gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												    "user_settings_home"))) :
		
		g_strdup_printf ("/home/%s", gtk_entry_get_text
					  (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												  "user_settings_name"))));
	
	xml_set_child_content (node, "home", buf);

	/* Shell */
	buf = adv ?
		gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												    "user_settings_shell"))) :
		
		g_strdup ("/bin/bash");

	xml_set_child_content (node, "shell", buf);

	/* UID */
	if (adv)
	{
		id = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
									    (xst_dialog_get_widget (tool->main_dialog,
													    "user_settings_uid")));
		
		xml_set_child_content (node, "uid", g_strdup_printf ("%d", id));
	}
}

static xmlNodePtr
group_add_blank_xml (xmlNodePtr db_node)
{
	xmlNodePtr group;

	g_return_val_if_fail (db_node != NULL, NULL);

	group = xml_element_add (db_node, "group");

	xml_element_add_with_content (group, "key", find_new_key (db_node));
	xml_element_add (group, "name");
	xml_element_add (group, "password");
	xml_element_add_with_content (group, "gid", find_new_id (db_node));
	xml_element_add (group, "users");

	return group;
}

static void
group_add_from_user (ug_data *ud)
{
	xmlNodePtr node;
	gchar *buf;
	ug_data *group_data;

	node = get_corresp_field (ud->node);

	group_data = g_new (ug_data, 1);
	
	group_data->node = group_add_blank_xml (node);

	buf = gtk_entry_get_text (
			GTK_ENTRY (GTK_COMBO (xst_dialog_get_widget (tool->main_dialog, "user_settings_group"))->entry));

	xml_set_child_content (group_data->node, "name", buf);

	/* Add it to e-table. */
	if (ud->table == TABLE_USER)
		group_data->table = TABLE_GROUP;
	else if (ud->table == TABLE_NET_USER)
		group_data->table = TABLE_NET_GROUP;
	else
	{
		g_warning ("group_add_from_user: shouldn't be here");
		g_free (group_data);
		return;
	}
	
	current_table_new_row (group_data);
	g_free (group_data);
}

static void
group_update_xml (xmlNodePtr node, gboolean adv)
{
	GtkCList *clist;
	gchar *buf;
	gint row = 0;

	/* Name */
	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "group_settings_name")));
	xml_set_child_content (node, "name", buf);

	/* Users */
	del_group_users (node);
	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));

	while (gtk_clist_get_text (clist, row++, 0, &buf))
		add_group_users (node, buf);
}

static void
group_update_users (xmlNodePtr node, gchar *old_name, gchar *new_name)
{
	xmlNodePtr dbnode, gnode;
	gchar *buf;

	g_return_if_fail (node != NULL);
	g_return_if_fail (old_name != NULL);
	g_return_if_fail (new_name != NULL);

	if (!strcmp (old_name, new_name))
		return;
	
	dbnode = get_db_node (node);
	dbnode = get_corresp_field (dbnode);

	for (dbnode = xml_element_find_first (dbnode, "group");
		dbnode;
		dbnode = dbnode->next)
	{
		gnode = xml_element_find_first (dbnode, "users");

		if (!gnode)
			continue;

		for (gnode = gnode->childs; gnode; gnode = gnode->next)
		{
			buf = xml_element_get_content (gnode);

			if (!buf)
				continue;

			if (!strcmp (buf, old_name))
				xml_element_set_content (gnode, new_name);

			g_free (buf);
		}
	}
}

static gboolean
node_exsists (xmlNodePtr node, gchar *name, gchar *val)
{
	xmlNodePtr n0;
	gchar *buf;
	xmlNodePtr parent = get_db_node (node);

	for (n0 = parent->childs; n0; n0 = n0->next)
	{
		if (n0 == node)
			continue;  /* Self */

		buf = xml_get_child_content (n0, name);

		if (!buf)
			continue;  /* No content */

		if (!strcmp (val, buf))
		{
			g_free (buf);  /* Woohoo! found! */
			return TRUE;
		}

		g_free (buf);
	}

	return FALSE;
}

static GList *
get_group_list (gchar *field, xmlNodePtr node, gboolean adv)
{
	GList *list = NULL;
	xmlNodePtr u;
	gint gid, min, max;
	gchar *txt;

	node = get_corresp_field (node);
	get_min_max (node, &min, &max);
	
	if (!node)
		return NULL;

	for (u = xml_element_find_first (node, "group"); u; u = xml_element_find_next (u, "group"))
	{
		txt = xml_get_child_content (u, "gid");
		gid = atoi (txt);

		if (adv || (gid >= min && gid <= max))
		{
			if (strcmp (field, "gid"))
			{
				g_free (txt);
				txt = xml_get_child_content (u, field);
			}

			list = g_list_prepend (list, txt);
		}
	}

	return list;
}

static GList *
get_user_list (gchar *field, xmlNodePtr node, gboolean adv)
{
	GList *list = NULL;
	xmlNodePtr u;
	gint uid, min, max;
	gchar *txt;

	node = get_corresp_field (node);
	get_min_max (node, &min, &max);

	for (u = xml_element_find_first (node, "user"); u; u = xml_element_find_next (u, "user"))
	{
		txt = xml_get_child_content (u, "uid");
		uid = atoi (txt);

		if (adv || (uid >= min && uid <= max))
		{
			if (strcmp (field, "uid"))
			{
				g_free (txt);
				txt = xml_get_child_content (u, field);
			}

			list = g_list_prepend (list, txt);
		}
	}

	return list;
}

static GList *
get_group_users (xmlNodePtr node)
{
	GList *userlist = NULL;
	xmlNodePtr u;

	g_return_val_if_fail (node != NULL, NULL);

	node = xml_element_find_first (node, "users");
	if (!node)
	{
		g_warning ("get_group_users: can't get current group's users node.");
		return NULL;
	}

	for (u = xml_element_find_first (node, "user"); u; u = xml_element_find_next (u, "user"))
		userlist = g_list_prepend (userlist, xml_element_get_content (u));

	return userlist;
}

static void
user_fill_settings_group (GtkCombo *combo, xmlNodePtr node, gboolean adv)
{
	GList *tmp_list, *items;
	gchar *name;

	items = NULL;
	tmp_list = get_group_list ("name", node, adv);
	while (tmp_list)
	{
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		items = g_list_append (items, name);
	}
	g_list_free (tmp_list);

	items = g_list_sort (items, char_sort_func);

	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
}

static GList *
group_fill_members_list (xmlNodePtr node)
{
	GList *tmp_list;
	GList *member_rows = NULL;
	GtkCList *clist;
	gint row;
	gchar *entry[2];

	entry[1] = NULL;

	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));
	gtk_clist_set_auto_sort (clist, TRUE);
	gtk_clist_freeze (clist);

	tmp_list = get_group_users (node);

	while (tmp_list)
	{
		entry[0] = tmp_list->data;
		tmp_list = tmp_list->next;

		row = gtk_clist_append (clist, entry);
		member_rows = g_list_append (member_rows, entry[0]);
	}

	gtk_clist_thaw (clist);
	return member_rows;
}

static void
group_fill_all_users_list (xmlNodePtr node, GList *exclude)
{
	GList *tmp_list, *member;
	GtkCList *clist;
	gchar *name, *gname;
	gboolean found;
	gchar *entry[2];
	gboolean adv;

	entry[1] = NULL;

	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_all"));
	gtk_clist_set_auto_sort (clist, TRUE);
	gtk_clist_freeze (clist);

	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);
	tmp_list = get_user_list ("login", node, adv);
	while (tmp_list)
	{
		gname = tmp_list->data;
		tmp_list = tmp_list->next;

		found = FALSE;

		member = exclude;
		while (member)
		{
			name = member->data;
			member = member->next;

			if (!strcmp (name, gname))
			{
				found = TRUE;
				break;
			}
		}

		if (!found)
		{
			entry[0] = gname;
			gtk_clist_append (clist, entry);
		}
	}
	g_list_free (tmp_list);

	gtk_clist_thaw (clist);
}

static void
del_group_users (xmlNodePtr node)
{
	g_return_if_fail (node != NULL);

	node = xml_element_find_first (node, "users");
	if (!node)
		return;

	xml_element_destroy_children (node);
}

static void
add_group_users (xmlNodePtr node, gchar *name)
{
	xmlNodePtr user;

	g_return_if_fail (node != NULL);

	user = xml_element_find_first (node, "users");
	if (!user)
		user = xml_element_add (node, "users");

	user = xml_element_add (user, "user");
	xml_element_set_content (user, name);
}

static gboolean
is_valid_name (gchar *str)
{
	if (!str || !*str)
		return FALSE;

	for (;*str; str++)
	{
		if (((*str < 'a') || (*str > 'z')) &&
				((*str < '0') || (*str > '9')))

			return FALSE;
	}

	return TRUE;
}

static gchar *
find_new_id (xmlNodePtr parent)
{
	gchar *field, *buf;
	gint id;
	gint min, max;
	gint ret = 0;
	xmlNodePtr n0;

	g_return_val_if_fail (parent != NULL, NULL);

	if (!strcmp (parent->name, "userdb"))
	{
		field = g_strdup ("uid");
		min = logindefs.new_user_min_id;
		max = logindefs.new_user_max_id;
	}

	else if (!strcmp (parent->name, "groupdb"))
	{
		field = g_strdup ("gid");
		min = logindefs.new_group_min_id;
		max = logindefs.new_group_max_id;
	}

	else
		return NULL;

	for (n0 = parent->childs; n0; n0 = n0->next)
	{
		buf = xml_get_child_content (n0, field);

		if (!buf)
			continue;

		id = atoi (buf);
		g_free (buf);

		if (ret < id)
			ret = id;
	}
	g_free (field);
	ret++;

	if (ret >= min && ret <= max)
		return g_strdup_printf ("%d", ret);

	return NULL;
}

static gchar *
find_new_key (xmlNodePtr parent)
{
	/* TODO: Possibily mix together find_new_id and find_new_key. */
	gchar *buf;
	gint id;
	gint ret = 0;
	xmlNodePtr n0;

	g_return_val_if_fail (parent != NULL, NULL);

	for (n0 = parent->childs; n0; n0 = n0->next)
	{
		buf = xml_get_child_content (n0, "key");

		if (!buf)
			continue;

		id = atoi (buf);
		g_free (buf);

		if (ret < id)
			ret = id;
	}

	return g_strdup_printf ("%d", ++ret);
}

static void
reply_cb (gint val, gpointer data)
{
        reply = val;
        gtk_main_quit ();
}

static gint
char_sort_func (gconstpointer a, gconstpointer b)
{
	return (strcmp (a, b));
}


