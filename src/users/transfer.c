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


/* Global, with some hard-coded defaults, just in case any of the tags is not present. */
/* These were taken form RH 6.2's default values. Any better suggestions? */
/* NULL is not present for string members. */

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
	gchar *tag;
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
			 case  0: logindefs.qmail_dir = xml_element_get_content (n0); break;
			 case  1: logindefs.mailbox_dir = xml_element_get_content (n0); break;
			 case  2: logindefs.mailbox_file = xml_element_get_content (n0); break;
			 case  3: logindefs.passwd_max_day_use = atoi (xml_element_get_content (n0)); break;
			 case  4: logindefs.passwd_min_day_use = atoi (xml_element_get_content (n0)); break;
			 case  5: logindefs.passwd_min_length = atoi (xml_element_get_content (n0)); break;
			 case  6: logindefs.passwd_warning_advance_days = atoi (xml_element_get_content (n0)); break;
			 case  7: logindefs.new_user_min_id = atoi (xml_element_get_content (n0)); break;
			 case  8: logindefs.new_user_max_id = atoi (xml_element_get_content (n0)); break;
			 case  9: logindefs.new_group_min_id = atoi (xml_element_get_content (n0)); break;
			 case 10: logindefs.new_group_max_id = atoi (xml_element_get_content (n0)); break;
			 case 11: logindefs.del_user_additional_command = xml_element_get_content (n0); break;
			 case 12: logindefs.create_home = atoi (xml_element_get_content (n0)); break;
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
	
	gchar *user_tags[] = {
		"key", "login", "password", "uid", "gid", "comment", "home", "shell", "last_mod", 
		"passwd_max_life", "passwd_exp_warn", "passwd_exp_disable", "passwd_disable", "reserved",
		"is_shadow", NULL
	};
	gchar *tag;
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
				 case  0: u->key = xml_element_get_content (n0); break;
				 case  1: u->login = xml_element_get_content (n0); break;
				 case  2: u->password = xml_element_get_content (n0); break;
				 case  3: u->uid = atoi (xml_element_get_content (n0)); break;
				 case  4: u->gid = atoi (xml_element_get_content (n0)); break;
				 case  5: u->comment = xml_element_get_content (n0); break;
				 case  6: u->home = xml_element_get_content (n0); break;
				 case  7: u->shell = xml_element_get_content (n0); break;
				 case  8: u->last_mod = atoi (xml_element_get_content (n0)); break;
				 case  9: u->passwd_max_life = atoi (xml_element_get_content (n0)); break;
				 case 10: u->passwd_exp_warn = atoi (xml_element_get_content (n0)); break;
				 case 11: u->passwd_exp_disable = atoi (xml_element_get_content (n0)); break; /* FIXME if -1 TRUE */
				 case 12: u->passwd_disable = atoi (xml_element_get_content (n0)); break; /* FIXME if -1 TRUE */
				 case 13: u->reserved = xml_element_get_content (n0); break; 
				 case 14: u->is_shadow = atoi (xml_element_get_content (n0)); break;
				}
			}
		}
		
		user_list = g_list_append (user_list, u);
	}
}


static void
transfer_group_list_xml_to_glist (xmlNodePtr root)
{
	xmlNodePtr users_node, node, n0, n1, n2, n3;
	group *g;

	/* Find groupdb */
	
	users_node = xml_element_find_first (root, "groupdb");
	if (!users_node)
	{
		g_warning ("transfer_group_list_xml_to_glist: couldn't find groupdb node.");
		return;
	}

	for (node = xml_element_find_first (users_node, "group");
			node;
		       	node = xml_element_find_next (node, "group"))
	{

		n0 = xml_element_find_first (node, "key");
		n1 = xml_element_find_first (node, "name");
		n2 = xml_element_find_first (node, "password");
		n3 = xml_element_find_first (node, "gid");
		
		g = g_new0 (group, 1);

		if (n0)
			g->key = atoi (xml_element_get_content (n0));
		
		if (n1)
			g->name = g_strdup (xml_element_get_content (n1));
		
		if (n2)
			g->password = g_strdup (xml_element_get_content (n2));

		if (n3)
			g->gid = atoi (xml_element_get_content (n3));

		group_list = g_list_append (group_list, g);
	}
}


static void
transfer_user_list_to_gui (void)
{
	GList *u, *rows = NULL;
	GtkList *list;
	user *current_u;
	gint num_rows;
	GtkWidget *list_item;

	/* Ok, first our users should be sorted by login */
	user_list = g_list_sort (user_list, user_sort_by_login);

	list = GTK_LIST (tool_widget_get ("user_list"));

	for (u = g_list_first (user_list), num_rows = 0; u; u = g_list_next (u), num_rows++)
	{
		
		current_u = (user *)u->data;
		list_item = gtk_list_item_new_with_label (current_u->login);
		gtk_widget_show (list_item);
		gtk_object_set_data (GTK_OBJECT (list_item), user_list_data_key, current_u);
		rows = g_list_append (rows, list_item);
	}

	gtk_list_append_items (list, rows);

	/* Select last item (and make it current) */
	u = g_list_last (user_list);
	current_user = (user *)u->data;
	gtk_list_select_item (list, --num_rows);
}

static void
transfer_group_list_to_gui (void)
{
	GList *g, *rows = NULL;
	GtkList *list;
	group *current_g;
	gint num_rows;
	GtkWidget *list_item;

	/* Ok, first our groups should be sorted by name */
	group_list = g_list_sort (group_list, group_sort_by_name);

	list = GTK_LIST (tool_widget_get ("group_list"));

	for (g = g_list_first (group_list), num_rows = 0; g; g = g_list_next (g), num_rows++)
	{
		current_g = (group *)g->data;
		list_item = gtk_list_item_new_with_label (current_g->name);
		gtk_widget_show (list_item);
		gtk_object_set_data (GTK_OBJECT (list_item), group_list_data_key, current_g);
		rows = g_list_append (rows, list_item);
	}

	gtk_list_append_items (list, rows);

	/* Select last item (and make it current) */
	g = g_list_last (group_list);
	current_group = (group *)g->data;
	gtk_list_select_item (list, --num_rows);
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
}
