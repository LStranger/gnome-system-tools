/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* profile.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2001 Ximian, Inc.
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
#include "profile.h"
#include "user_group.h"
#include "callbacks.h"

typedef struct
{
	GtkCombo *name;
	GnomeFileEntry *home_prefix;
	GtkCombo *shell;
	GtkEntry *group;
	GtkSpinButton   *umin;
	GtkSpinButton   *umax;
	GtkSpinButton   *gmin;
	GtkSpinButton   *gmax;
	GtkSpinButton   *pwd_maxdays;
	GtkSpinButton   *pwd_mindays;
	GtkSpinButton   *pwd_warndays;
	GtkToggleButton *pwd_random;
} ProfileTab;

extern XstTool *tool;
ProfileTable *profile_table;
static ProfileTab *pft;


static guint
my_atoi (gchar *str) 
{
	if (!str || !*str)
		return 0;
	return atoi (str);
}

static void
profile_tab_init (void)
{
	XstDialog *xd;

	xd = tool->main_dialog;
	pft = g_new (ProfileTab, 1);
	
	pft->name = GTK_COMBO (xst_dialog_get_widget (xd, "pro_name"));

	pft->home_prefix = GNOME_FILE_ENTRY (xst_dialog_get_widget (xd, "pro_home"));
	pft->shell       = GTK_COMBO (xst_dialog_get_widget (xd, "pro_shell"));
	pft->group       = GTK_ENTRY (xst_dialog_get_widget (xd, "pro_group"));
	
	pft->umin = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_umin"));
	pft->umax = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_umax"));
	pft->gmin = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_gmin"));
	pft->gmax = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_gmax"));

	pft->pwd_maxdays  = GTK_SPIN_BUTTON   (xst_dialog_get_widget (xd, "pro_maxdays"));
	pft->pwd_mindays  = GTK_SPIN_BUTTON   (xst_dialog_get_widget (xd, "pro_mindays"));
	pft->pwd_warndays = GTK_SPIN_BUTTON   (xst_dialog_get_widget (xd, "pro_between"));
	pft->pwd_random   = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (xd, "pro_random"));
}

static void
profile_fill (Profile *pf)
{
	if (!pft)
		profile_tab_init ();

	gtk_signal_handler_block_by_func (GTK_OBJECT (pft->name->entry),
					  GTK_SIGNAL_FUNC (on_pro_name_changed),
					  NULL);
	
	gtk_entry_set_text (GTK_ENTRY (pft->name->entry), pf->name);

	gtk_signal_handler_unblock_by_func (GTK_OBJECT (pft->name->entry),
					    GTK_SIGNAL_FUNC (on_pro_name_changed),
					    NULL);
	
	my_gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (pft->home_prefix)),
					pf->home_prefix);	
	my_gtk_entry_set_text (GTK_ENTRY (pft->shell->entry), pf->shell);
	my_gtk_entry_set_text (GTK_ENTRY (pft->group), pf->group);
	
	gtk_spin_button_set_value (pft->umin, (gfloat) pf->umin);
	gtk_spin_button_set_value (pft->umax, (gfloat) pf->umax);
	gtk_spin_button_set_value (pft->gmin, (gfloat) pf->gmin);
	gtk_spin_button_set_value (pft->gmax, (gfloat) pf->gmax);

	gtk_spin_button_set_value (pft->pwd_maxdays,  (gfloat) pf->pwd_maxdays);
	gtk_spin_button_set_value (pft->pwd_mindays,  (gfloat) pf->pwd_mindays);
	gtk_spin_button_set_value (pft->pwd_warndays, (gfloat) pf->pwd_warndays);
	gtk_toggle_button_set_active (pft->pwd_random, pf->pwd_random);
}

void
profile_save (gchar *name)
{
	gchar *buf;
	Profile *pf;
	GtkWidget *li;
	
	if (name)
		buf = g_strdup (name);
	else
		buf = g_strdup (profile_table->selected);
	
	pf = (Profile *)g_hash_table_lookup (profile_table->hash, buf);
	g_free (buf);

	if (pf)
	{
		/* Name */
/* FIXME: Can't modify name, messes up hash table.
                if (pf->name)
		{
			buf = g_strdup (pf->name);
			g_free (pf->name);
		}

		pf->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (pft->name->entry)));
		profile_table->selected = pf->name;
		
		xst_ui_combo_remove_by_label (pft->name, buf);
		g_free (buf);
		
		li = gtk_list_item_new_with_label (pf->name);
		gtk_widget_show (li);
		gtk_container_add (GTK_CONTAINER (pft->name->list), li);
*/		

		if (pf->home_prefix)
			g_free (pf->home_prefix);
		pf->home_prefix = g_strdup (gtk_entry_get_text
					    (GTK_ENTRY (gnome_file_entry_gtk_entry (pft->home_prefix))));

		if (pf->shell)
			g_free (pf->shell);
		pf->shell = g_strdup (gtk_entry_get_text (GTK_ENTRY (pft->shell->entry)));

		if (pf->group)
			g_free (pf->group);
		pf->group = g_strdup (gtk_entry_get_text (GTK_ENTRY (pft->group)));
		
		pf->umin = gtk_spin_button_get_value_as_int (pft->umin);
		pf->umax = gtk_spin_button_get_value_as_int (pft->umax);
		pf->gmin = gtk_spin_button_get_value_as_int (pft->gmin);
		pf->gmax = gtk_spin_button_get_value_as_int (pft->gmax);

		pf->pwd_maxdays  = gtk_spin_button_get_value_as_int (pft->pwd_maxdays);
		pf->pwd_mindays  = gtk_spin_button_get_value_as_int (pft->pwd_mindays);
		pf->pwd_warndays = gtk_spin_button_get_value_as_int (pft->pwd_warndays);
		pf->pwd_random = gtk_toggle_button_get_active (pft->pwd_random);
	}
}	

gboolean
profile_add (Profile *old_pf, const gchar *new_name, gboolean select)
{	
	Profile *pf;
	GtkWidget *d;

	pf = profile_table_get_profile (new_name);
	if (pf)
	{
		d = gnome_error_dialog_parented (N_("Sorry, profile with given name already exists."),
						 GTK_WINDOW (tool->main_dialog));
		gnome_dialog_run (GNOME_DIALOG (d));
		return FALSE;
	}
	
	pf = g_new0 (Profile, 1);

	pf->name = g_strdup (new_name);

	if (old_pf)
	{
		/* Let's make copy of old profile. */
		
		pf->home_prefix = g_strdup (old_pf->home_prefix);
		pf->shell = g_strdup (old_pf->shell);
		pf->group = g_strdup (old_pf->group);
		
		pf->umin = old_pf->umin;
		pf->umax = old_pf->umax;
		pf->gmin = old_pf->gmin;
		pf->gmax = old_pf->gmax;
		pf->pwd_maxdays = old_pf->pwd_maxdays;
		pf->pwd_mindays = old_pf->pwd_mindays;
		pf->pwd_warndays = old_pf->pwd_warndays;
		pf->pwd_random = old_pf->pwd_random;
	}

	profile_table_add_profile (pf, select);

	return TRUE;
}

void
profile_destroy (Profile *pf)
{
	if (!pf)
		return;

	g_free (pf->name);
	g_free (pf->home_prefix);
	g_free (pf->shell);
	g_free (pf->group);

	g_free (pf);
}

/* Profile table functions */

void
profile_table_init (void)
{
	if (!profile_table)
	{
		profile_table = g_new (ProfileTable, 1);

		profile_table->selected = NULL;
		profile_table->hash = g_hash_table_new (g_str_hash, g_str_equal);
	}
}

void
profile_table_destroy (void)
{
	if (!profile_table)
		return;

	g_free (profile_table->selected);
	g_hash_table_destroy (profile_table->hash);

	g_free (profile_table);
}


static void
profile_get_default (void)
{
	/* It's currently only a wrapper around logindefs, will change in future. */
	
	Profile *pf;
	
	pf = g_new (Profile, 1);
	pf->name = g_strdup (N_("Default"));

	/* FIXME: Bad bad hardcoded values. */
	pf->home_prefix = g_strdup ("/home/$user");
	pf->shell = g_strdup ("/bin/bash");
	pf->group = g_strdup ("$user");

	pf->umin = logindefs.new_user_min_id;
	pf->umax = logindefs.new_user_max_id;
	pf->gmin = logindefs.new_group_min_id;
	pf->gmax = logindefs.new_group_max_id;
	pf->pwd_maxdays = logindefs.passwd_max_day_use;
	pf->pwd_mindays = logindefs.passwd_min_day_use;
	pf->pwd_warndays = logindefs.passwd_warning_advance_days;
	pf->pwd_random = FALSE;
	pf->logindefs = TRUE;

	profile_table_add_profile (pf, TRUE);
}

void
profile_table_from_xml (xmlNodePtr root)
{
	xmlNodePtr node, n0, pf_node;
	Profile *pf;
	gchar *profile_tags[] = {
		"home_prefix", "shell", "group", "pwd_maxdays",
		"pwd_mindays", "pwd_warndays", "umin",
		"umax", "gmin", "gmax", "pwd_random", "name", NULL
	};
	gchar *tag;
	gint i;

	profile_get_default ();
	
	node = xst_xml_element_find_first (root, "profiles");
	if (!node)
		return;

	pf_node = xst_xml_element_find_first (node, "profile");	
	while (pf_node)
	{
		pf = g_new (Profile, 1);
		pf->logindefs = FALSE;
		for (i = 0, tag = profile_tags[0]; tag; i++, tag = profile_tags[i]) 
		{
			n0 = xst_xml_element_find_first (pf_node, tag);
			
			if (n0) 
			{
				switch (i)
				{
				case  0: pf->home_prefix  = xst_xml_element_get_content (n0); break;
				case  1: pf->shell        = xst_xml_element_get_content (n0); break;
				case  2: pf->group        = xst_xml_element_get_content (n0); break;
				case  3: pf->pwd_maxdays  = my_atoi (xst_xml_element_get_content (n0)); break;
				case  4: pf->pwd_mindays  = my_atoi (xst_xml_element_get_content (n0)); break;
				case  5: pf->pwd_warndays = my_atoi (xst_xml_element_get_content (n0)); break;
				case  6: pf->umin         = my_atoi (xst_xml_element_get_content (n0)); break;
				case  7: pf->umax         = my_atoi (xst_xml_element_get_content (n0)); break;
				case  8: pf->gmin         = my_atoi (xst_xml_element_get_content (n0)); break;
				case  9: pf->gmax         = my_atoi (xst_xml_element_get_content (n0)); break;
				case 10: pf->pwd_random   = xst_xml_element_get_bool_attr (n0, "set"); break;
				case 11: pf->name         = xst_xml_element_get_content (n0); break;
					
				default: g_warning ("profile_get_from_xml: we shouldn't be here."); break;
				}
			}
		}
		profile_table_add_profile (pf, FALSE);
		pf_node = xst_xml_element_find_next (pf_node, "profile");
	}
}

static void
save_logindefs_xml (Profile *pf, xmlNodePtr root)
{
	gint i, val;
	xmlNodePtr node;
	gchar *buf;
	gchar *nodes[] = { "new_user_min_id", "new_user_max_id", "new_group_min_id",
			   "new_group_max_id", "passwd_max_day_use", "passwd_min_day_use",
			   "passwd_warning_advance_days", NULL };

	/* FIXME: */
	root = xst_xml_doc_get_root (tool->config);
	root = xst_xml_element_find_first (root, "logindefs");

	for (i = 0; nodes[i]; i++)
	{
		switch (i)
		{
		case 0: val = pf->umin; break;
		case 1: val = pf->umax; break;
		case 2: val = pf->gmin; break;
		case 3: val = pf->gmax; break;
		case 4: val = pf->pwd_maxdays; break;
		case 5: val = pf->pwd_mindays; break;
		case 6: val = pf->pwd_warndays; break;
		default:
			g_warning ("save_logindefs_xml: Shouldn't be here");
			continue;
		}

		buf = g_strdup_printf ("%d", val);

		node = xst_xml_element_find_first (root, nodes[i]);
		if (!node)
			node = xst_xml_element_add (root, nodes[i]);

		xst_xml_element_set_content (node, buf);
		g_free (buf);
	}
}

static void
save_xml (gpointer key, gpointer value, gpointer user_data)
{
	xmlNodePtr root, node;
	Profile *pf;
	gint i, val;
	gchar *buf;
	gchar *nodes[] = {
		"pwd_maxdays", "pwd_mindays", "pwd_warndays",
		"umin", "umax", "gmin", "gmax", NULL};

	root = user_data;
	pf = value;

	if (pf->logindefs) /* Logindefs is "fake" profile. */
	{
		save_logindefs_xml (pf, root);
		return;
	}

	node = xst_xml_element_add (root, "profile");

	xst_xml_element_add_with_content (node, "name",        pf->name);
	xst_xml_element_add_with_content (node, "home_prefix", pf->home_prefix);
	xst_xml_element_add_with_content (node, "shell",       pf->shell);
	xst_xml_element_add_with_content (node, "group",       pf->group);
	xst_xml_element_set_bool_attr (xst_xml_element_add (node, "pwd_random"),
				       "set", pf->pwd_random);
	
	for (i = 0; nodes[i]; i++)
	{
		switch (i)
		{
		case 0: val = pf->pwd_maxdays;  break;
		case 1: val = pf->pwd_mindays;  break;
		case 2: val = pf->pwd_warndays; break;
		case 3: val = pf->umin;         break;
		case 4: val = pf->umax;         break;
		case 5: val = pf->gmin;         break;
		case 6: val = pf->gmax;         break;
		default:
			g_warning ("save_xml: Shouldn't be here");
			continue;
		}
		
		buf = g_strdup_printf ("%d", val);
		xst_xml_element_add_with_content (node, nodes[i], buf);
		g_free (buf);
	}
}

void
profile_table_to_xml (xmlNodePtr root)
{
	xmlNodePtr node;
	
	node = xst_xml_element_find_first (root, "profiles");

	if (!node)
		node = xst_xml_element_add (root, "profiles");
	else
		xst_xml_element_destroy_children (node);

	g_hash_table_foreach (profile_table->hash, save_xml, node);
}

void
profile_table_add_profile (Profile *pf, gboolean select)
{
	GtkWidget *li;
	
	g_hash_table_insert (profile_table->hash, pf->name, pf);

	if (!pft)
		profile_tab_init ();
	
	/* add name to combo box */
	gtk_signal_handler_block_by_func (GTK_OBJECT (pft->name->entry),
					  GTK_SIGNAL_FUNC (on_pro_name_changed),
					  NULL);
	li = gtk_list_item_new_with_label (pf->name);
	gtk_widget_show (li);
	gtk_container_add (GTK_CONTAINER (pft->name->list), li);
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (pft->name->entry),
					  GTK_SIGNAL_FUNC (on_pro_name_changed),
					  NULL);	
	if (select || g_hash_table_size (profile_table->hash) == 1)
	{
		profile_table->selected = pf->name;
		profile_fill (pf);
	}
}

gboolean
profile_table_del_profile (gchar *name)
{
	gchar *buf;
	Profile *pf;
	gboolean retval;

	retval = FALSE;
	
	if (name)
		buf = g_strdup (name);
	else
		buf = g_strdup (profile_table->selected);
	
	pf = (Profile *)g_hash_table_lookup (profile_table->hash, buf);

	if (pf)
	{
		if (pf->logindefs)
		{
			GnomeDialog *d;

			d = GNOME_DIALOG (gnome_error_dialog_parented (N_("Can't delete Default"),
								       GTK_WINDOW (tool->main_dialog)));
			gnome_dialog_run (d);
		}

		else
		{
			g_hash_table_remove (profile_table->hash, buf);
			profile_destroy (pf);

			if (g_hash_table_size (profile_table->hash) == 0)
				profile_table->selected = NULL;

			xst_ui_combo_remove_by_label (pft->name, buf);
			retval = TRUE;
		}
	}

	g_free (buf);

	return retval;
}

Profile *
profile_table_get_profile (const gchar *name)
{
	gchar *buf;
	Profile *pf;
	
	if (name)
		buf = g_strdup (name);
	else
		buf = g_strdup (profile_table->selected);

	pf = (Profile *)g_hash_table_lookup (profile_table->hash, buf);
	g_free (buf);

	return pf;
}

void
profile_table_set_selected (const gchar *name)
{
	Profile *pf;
	
	if (!name)
		return;

	pf = (Profile *)g_hash_table_lookup (profile_table->hash, name);

	if (!pf)
		return;
	
	profile_table->selected = pf->name;
	profile_fill (pf);
}
