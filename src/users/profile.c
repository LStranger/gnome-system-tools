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

extern XstTool *tool;
ProfileTable *profile_table;
static ProfileTab *pft;

static void
profile_tab_init (void)
{
	XstDialog *xd;

	xd = tool->main_dialog;
	pft = g_new (ProfileTab, 1);
	
	pft->name = GTK_COMBO (xst_dialog_get_widget (xd, "pro_name"));

	pft->create_home = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (xd, "pro_create_home"));
	pft->home_prefix = GNOME_FILE_ENTRY (xst_dialog_get_widget (xd, "pro_home"));
	pft->shell       = GTK_COMBO (xst_dialog_get_widget (xd, "pro_shell"));
	
	pft->umin = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_umin"));
	pft->umax = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_umax"));
	pft->gmin = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_gmin"));
	pft->gmax = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_gmax"));

	pft->pwd_maxdays  = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_maxdays"));
	pft->pwd_mindays  = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_mindays"));
	pft->pwd_warndays = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_between"));
	pft->pwd_len      = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "pro_pwd_len"));
}

static void
profile_fill (Profile *pf)
{
	GtkWidget *li;
	
	if (!pft)
		profile_tab_init ();

	/* add name to combo box */
	li = gtk_list_item_new_with_label (pf->name);
	gtk_widget_show (li);
	gtk_container_add (GTK_CONTAINER (pft->name->list), li);
	gtk_entry_set_text (GTK_ENTRY (pft->name->entry), pf->name);

	gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (pft->home_prefix)),
					pf->home_prefix);	
	gtk_entry_set_text (GTK_ENTRY (pft->shell->entry), pf->shell);
	
	gtk_spin_button_set_value (pft->umin, (gfloat) pf->umin);
	gtk_spin_button_set_value (pft->umax, (gfloat) pf->umax);
	gtk_spin_button_set_value (pft->gmin, (gfloat) pf->gmin);
	gtk_spin_button_set_value (pft->gmax, (gfloat) pf->gmax);

	gtk_spin_button_set_value (pft->pwd_maxdays,  (gfloat) pf->pwd_maxdays);
	gtk_spin_button_set_value (pft->pwd_mindays,  (gfloat) pf->pwd_mindays);
	gtk_spin_button_set_value (pft->pwd_warndays, (gfloat) pf->pwd_warndays);
	gtk_spin_button_set_value (pft->pwd_len,      (gfloat) pf->pwd_len);	
}

Profile *
profile_get_default (void)
{
	/* It's currently only a wrapper around logindefs, will change in future. */
	
	Profile *pf;
	
	pf = g_new (Profile, 1);
	pf->name = g_strdup (N_("Default"));

	/* FIXME: Bad bad hardcoded values. */
	pf->home_prefix = g_strdup ("/home/");
	pf->shell = g_strdup ("/bin/bash");

	pf->umin = logindefs.new_user_min_id;
	pf->umax = logindefs.new_user_max_id;
	pf->gmin = logindefs.new_group_min_id;
	pf->gmax = logindefs.new_group_max_id;
	pf->pwd_maxdays = logindefs.passwd_max_day_use;
	pf->pwd_mindays = logindefs.passwd_min_day_use;
	pf->pwd_warndays = logindefs.passwd_warning_advance_days;
	pf->pwd_len = logindefs.passwd_min_length;

	return pf;
}

void
profile_destroy (Profile *pf)
{
	if (!pf)
		return;

	/* FIXME: remove listitem from name list also */
	g_free (pf->name);
	g_free (pf->home_prefix);
	g_free (pf->shell);

	g_free (pf);
}

void
profile_table_init (void)
{
	if (!profile_table)
	{
		profile_table = g_new (ProfileTable, 1);

		profile_table->selected = NULL;
		profile_table->hash = g_hash_table_new (g_direct_hash, g_str_equal);
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

void
profile_table_add_profile (Profile *pf, gboolean select)
{
	g_hash_table_insert (profile_table->hash, pf->name, pf);

	if (select || g_hash_table_size (profile_table->hash) == 1)
	{
		profile_table->selected = pf->name;
		profile_fill (pf);
	}
}

void
profile_table_del_profile (gchar *name)
{
	Profile *pf;
	
	pf = (Profile *)g_hash_table_lookup (profile_table->hash, name);

	if (pf)
	{
		g_hash_table_remove (profile_table->hash, name);
		profile_destroy (pf);

		if (g_hash_table_size (profile_table->hash) == 0)
			profile_table->selected = NULL;
		/* FIXME: else: point profile_table->selected to next key */
	}
}

Profile *
profile_table_get_profile (gchar *name)
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
profile_table_set_selected (gchar *name)
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
