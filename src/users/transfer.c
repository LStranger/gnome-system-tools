/* transfer.c: this file is part of users-admin, a helix-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Tambet Ingo <tambeti@sa.ee> and Arturo Espinosa <arturo@helixcode.com>.
 */

/* Functions for transferring information between XML tree and UI */


#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>
#include "global.h"

#include "transfer.h"
#include "callbacks.h"

GList *user_list = NULL;
user *current_user = NULL;

GList *group_list = NULL;
group *current_group = NULL;

_logindefs logindefs;

const gchar *user_list_data_key = "user_list_data";
const gchar *group_list_data_key = "group_list_data";

/* Prototypes */
gint user_sort_by_uid (gconstpointer a, gconstpointer b);
gint user_sort_by_login (gconstpointer a, gconstpointer b);
gint group_sort_by_name (gconstpointer a, gconstpointer b);


/* Structure with some hard-coded defaults, just in case any of the tags is not present. */
/* These were taken form RH 6.2's default values. Any better suggestions? */
/* NULL means not present for string members. */

const static _logindefs default_logindefs = {
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

static gchar *
my_xml_element_get_content (xmlNodePtr node)
{
	gchar *ret;
	
	ret = xml_element_get_content (node);
	if (!ret) 
	{
		ret = g_new0 (gchar, 1);
	}
	
	return ret;
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
	
	node = xml_element_find_first (root, "logindefs");
	if (!node)
	{
		g_warning ("transfer_logindefs_from_xml: couldn't find logindefs node.");
		return;
	}

	/* make login.defs struct */

	for (i = 0, tag = logindefs_tags[0]; tag; i++, tag = logindefs_tags[i]) 
	{
		n0 = xml_element_find_first (node, tag);

		if (n0) 
		{
			switch (i)
			{
			 case  0: logindefs.qmail_dir = my_xml_element_get_content (n0); break;
			 case  1: logindefs.mailbox_dir = my_xml_element_get_content (n0); break;
			 case  2: logindefs.mailbox_file = my_xml_element_get_content (n0); break;
			 case  3: logindefs.passwd_max_day_use = my_atoi (my_xml_element_get_content (n0)); break;
			 case  4: logindefs.passwd_min_day_use = my_atoi (my_xml_element_get_content (n0)); break;
			 case  5: logindefs.passwd_min_length = my_atoi (my_xml_element_get_content (n0)); break;
			 case  6: logindefs.passwd_warning_advance_days = my_atoi (my_xml_element_get_content (n0)); break;
			 case  7: logindefs.new_user_min_id = my_atoi (my_xml_element_get_content (n0)); break;
			 case  8: logindefs.new_user_max_id = my_atoi (my_xml_element_get_content (n0)); break;
			 case  9: logindefs.new_group_min_id = my_atoi (my_xml_element_get_content (n0)); break;
			 case 10: logindefs.new_group_max_id = my_atoi (my_xml_element_get_content (n0)); break;
			 case 11: logindefs.del_user_additional_command = my_xml_element_get_content (n0); break;
			 case 12: 
				tmp = my_xml_element_get_content (n0);
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
transfer_user_list_xml_to_glist (xmlNodePtr root)
{
	xmlNodePtr users_node, node, n0;
	user *u;
	
	gchar *tag, *tmp;
	gint i;

	/* Find userdb */
	
	users_node = xml_element_find_first (root, "userdb");
	if (!users_node)
	{
		g_warning ("transfer_user_list_xml_to_glist: couldn't find userdb node.");
		return;
	}
	
	/* For every user */
	for (node = xml_element_find_first (users_node, "user");
			 node;
			 node = xml_element_find_next (node, "user"))
	{

		u = g_new0 (user, 1);

		/* For every property in the user */
		for (i = 0, tag = user_tags[0]; tag; i++, tag = user_tags[i])
		{
			n0 = xml_element_find_first (node, tag);
			
			if (n0)
			{
				switch (i)
				{
				 case  0: u->key = my_xml_element_get_content (n0); break;
				 case  1: u->login = my_xml_element_get_content (n0); break;
				 case  2: u->password = my_xml_element_get_content (n0); break;
				 case  3: u->uid = my_atoi (my_xml_element_get_content (n0)); break;
				 case  4: u->gid = my_atoi (my_xml_element_get_content (n0)); break;
				 case  5: u->comment = my_xml_element_get_content (n0); break;
				 case  6: u->home = my_xml_element_get_content (n0); break;
				 case  7: u->shell = my_xml_element_get_content (n0); break;
				 case  8: u->last_mod = my_atoi (my_xml_element_get_content (n0)); break;
				 case  9: u->passwd_min_life = my_atoi (my_xml_element_get_content (n0)); break;
				 case 10: u->passwd_max_life = my_atoi (my_xml_element_get_content (n0)); break;
				 case 11: u->passwd_exp_warn = my_atoi (my_xml_element_get_content (n0)); break;
				 case 12: 
					tmp = my_xml_element_get_content (n0);
					u->is_passwd_exp_disable = FALSE;
					if (*tmp)
					{
						u->is_passwd_exp_disable = TRUE;
						u->passwd_exp_disable = my_atoi (tmp);
					}
					break;
				 case 13:
					tmp = my_xml_element_get_content (n0);
					u->is_passwd_disable = FALSE;
					if (*tmp)
					{
						u->is_passwd_disable = TRUE;
						u->passwd_disable = my_atoi (tmp);
					}
					break;
				 case 14: u->reserved = my_xml_element_get_content (n0); break; 
				 case 15: u->is_shadow = my_atoi (my_xml_element_get_content (n0)); break;
				}
			}
		}
		
		user_list = g_list_append (user_list, u);
	}
}


static void
transfer_group_list_xml_to_glist (xmlNodePtr root)
{
	xmlNodePtr group_node, node, n0;
	xmlNodePtr sub_node, user_node;
	group *g;
	gint i;
	gchar *tag;

	/* Find groupdb */
	
	group_node = xml_element_find_first (root, "groupdb");
	if (!group_node)
	{
		g_warning ("transfer_group_list_xml_to_glist: couldn't find groupdb node.");
		return;
	}

	for (node = xml_element_find_first (group_node, "group");
			 node;
			 node = xml_element_find_next (node, "group"))
	{

		g = g_new0 (group, 1);
		
		for (i = 0, tag = group_tags[0]; tag; i++, tag = group_tags[i])
		{
			n0 = xml_element_find_first (node, tag);

			if (n0)
			{
				switch (i)
				{
				 case 0: g->key = my_xml_element_get_content (n0); break;
				 case 1: g->name = g_strdup (my_xml_element_get_content (n0)); break;
				 case 2: g->password = g_strdup (my_xml_element_get_content (n0)); break;
				 case 3: g->gid = my_atoi (my_xml_element_get_content (n0)); break;
				}
			}
		}
			
		/* Get all users in group */

		g->users = NULL;

		sub_node = xml_element_find_first (node, "users");

		if (!sub_node)
			g_warning ("Couldn't find group users node.");

		for (user_node = xml_element_find_first (sub_node, "user"); user_node;
				user_node = xml_element_find_next (user_node, "user"))
				g->users = g_list_append (g->users, my_xml_element_get_content (user_node));

		group_list = g_list_append (group_list, g);
	}
}


static void
transfer_user_list_to_gui (void)
{
	GList *tmp_list;
	GtkCList *list;
	user *current_u;
	gchar *entry[2];
	gint row;

	entry[1] = NULL;

	list = GTK_CLIST (tool_widget_get ("user_list"));
	gtk_clist_set_auto_sort (list, TRUE); 

	gtk_clist_freeze (list);
	
	tmp_list = user_list;
	while (tmp_list)
	{
		current_u = tmp_list->data;
		tmp_list = tmp_list->next;
		
		if (current_u->uid >= logindefs.new_user_min_id &&
				current_u->uid <= logindefs.new_user_max_id)
		{
			entry[0] = current_u->login;
			row = gtk_clist_append (list, entry);
			gtk_clist_set_row_data (list, row, current_u);
		}
	}

	gtk_clist_thaw (list);

	/* Select first item (and make it current) */

	gtk_clist_select_row (list, 0, 0);
	current_user = gtk_clist_get_row_data (list, 0); 
}

static void
transfer_group_list_to_gui (void)
{
	GList *tmp_list;
	GtkCList *list;
	group *current_g;
	gchar *entry[2];
	gint row;

	entry[1] = NULL;

	list = GTK_CLIST (tool_widget_get ("group_list"));
	gtk_clist_set_auto_sort (list, TRUE); 

	gtk_clist_freeze (list);
	
	tmp_list = group_list;
	while (tmp_list)
	{
		current_g = tmp_list->data;
		tmp_list = tmp_list->next;

/*		if (current_g->gid >= logindefs.new_group_min_id &&
				current_g->gid <= logindefs.new_group_max_id) */
		{
			entry[0] = current_g->name;
			row = gtk_clist_append (list, entry);
			gtk_clist_set_row_data (list, row, current_g);
		}
	}

	gtk_clist_thaw (list);

	/* Select first item (and make it current) */

	gtk_clist_select_row (list, 0, 0);
	current_group = gtk_clist_get_row_data (list, 0);
}

static void
transfer_user_list_glist_to_xml (xmlNodePtr root)
{
	xmlNodePtr userdb_node, node;
	GList *tmplist;
	user *u;
	gint i;
	gchar *tag;
	gchar buf[15];	/*Max buf size for non-char tags */
	gchar *str = NULL;

	/* Delete old users and make a new ones */
	/* Should we sort it first by key? */
	
	userdb_node = xml_element_find_first (root, "userdb");
	xml_element_destroy_children (userdb_node);

	for (tmplist = g_list_first (user_list); tmplist; tmplist = g_list_next (tmplist))
	{
		u = (user *)tmplist->data;
		
		node = xml_element_add (userdb_node, "user");

		for (i = 0, tag = user_tags[0]; tag; i++, tag = user_tags[i])
		{
			switch (i)
			{
			 case  0: str = u->key; break;
			 case  1: str = u->login; break;
			 case  2: str = u->password; break;
			 case  3: snprintf (buf, 15, "%d", u->uid); str = buf; break;
			 case  4: snprintf (buf, 15, "%d", u->gid); str = buf; break;
			 case  5: str = u->comment; break;
			 case  6: str = u->home; break;
			 case  7: str = u->shell; break;
			 case  8: snprintf (buf, 15, "%d", u->last_mod); str = buf; break;
			 case  9: snprintf (buf, 15, "%d", u->passwd_min_life); str = buf; break;
			 case 10: snprintf (buf, 15, "%d", u->passwd_max_life); str = buf; break;
			 case 11: snprintf (buf, 15, "%d", u->passwd_exp_warn); str = buf; break;
			 case 12:
				if (u->is_passwd_exp_disable) 
				{
					snprintf (buf, 15, "%d", u->passwd_exp_disable); 
					str = buf;
				}
				else
					str = "";
				break;
			 case 13:
				if (u->is_passwd_disable) 
				{
					snprintf (buf, 15, "%d", u->passwd_disable); 
					str = buf;
				}
				else
					str = "";
				break;
			 case 14: str = u->reserved; break;
			 case 15: snprintf (buf, 15, "%d", u->is_shadow); str = buf; break;
			}
			
			xml_element_add_with_content (node, tag, str);
		}
	}
}


static void
transfer_group_list_glist_to_xml (xmlNodePtr root)
{
	xmlNodePtr groupdb_node, users_node, node, u_node;
	group *g;
	GList *tmplist, *list;
	gchar buf[15]; /*Max buf size for non-char tags */
	gchar *tmp;

	
	/* Delete old users and make a new ones */
	/* Should we sort it first by key? */
	
	groupdb_node = xml_element_find_first (root, "groupdb");
	xml_element_destroy_children (groupdb_node);

	for (tmplist = g_list_first (group_list); tmplist; tmplist = g_list_next (tmplist))
	{
		g = (group *)tmplist->data;
		node = xml_element_add (groupdb_node, "group");

		xml_element_add_with_content (node, "key", g->key);
		xml_element_add_with_content (node, "name", g->name);
		xml_element_add_with_content (node, "password", g->password);

		snprintf (buf, 15, "%d", g->gid);
		xml_element_add_with_content (node, "gid", buf);

		users_node = xml_element_add (node, "users");

		for (list = g_list_first (g->users); list; list = g_list_next (list))

		{
			tmp = list->data;
			u_node = xml_element_add (users_node, "user");
			xml_element_set_content (u_node, tmp);
		}
	}
}

gint
user_sort_by_uid (gconstpointer a, gconstpointer b)
{
	user *u_a, *u_b;

	u_a = (user *)a;
	u_b = (user *)b;
	
	if (u_a->uid > u_b->uid) return 1;
	if (u_a->uid == u_b->uid) return 0;
	
	return -1;
}

gint
user_sort_by_login (gconstpointer a, gconstpointer b)
{
	user *u_a, *u_b;

	u_a = (user *)a;
	u_b = (user *)b;
	
	return (strcmp (u_a->login, u_b->login));
}

gint
group_sort_by_name (gconstpointer a, gconstpointer b)
{
	group *g_a, *g_b;

	g_a = (group *)a;
	g_b = (group *)b;

	return (strcmp (g_a->name, g_b->name));
}



void
transfer_xml_to_gui (xmlNodePtr root)
{
	transfer_logindefs_from_xml (root);
	transfer_user_list_xml_to_glist (root);
	transfer_group_list_xml_to_glist (root);
	
	transfer_user_list_to_gui ();
	transfer_group_list_to_gui ();
	
}

void
transfer_gui_to_xml (xmlNodePtr root)
{
	transfer_user_list_glist_to_xml (root);
	transfer_group_list_glist_to_xml (root);
}

