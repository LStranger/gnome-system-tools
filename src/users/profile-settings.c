/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* profile-settings.c: this file is part of users-admin, a gnome-system-tool frontend 
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
 * MERCHANBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <garnacho@tuxerver.net>.
 */

#include <gtk/gtk.h>
#include "xst.h"
#include "user_group.h"
#include "table.h"
#include "user-group-xml.h"
#include "profile-settings.h"

extern XstTool *tool;
/*GList *groups_list = NULL;*/
extern GList *groups_list;

ProfileWidget profile_widgets [] = {
	/* widget name                    xml label         widget type */
	{ "profile_settings_name",        "name",           PROFILE_WIDGET_ENTRY },
	{ "profile_settings_comment",     "comment",        PROFILE_WIDGET_ENTRY },
	{ "profile_settings_home",        "home_prefix",    PROFILE_WIDGET_ENTRY },
	{ "profile_settings_shell_entry", "shell",          PROFILE_WIDGET_ENTRY },
	{ "profile_settings_group",       "group",          PROFILE_WIDGET_OPTION_MENU },
	{ "profile_settings_maxdays",     "pwd_maxdays",    PROFILE_WIDGET_SPIN_BUTTON },
	{ "profile_settings_mindays",     "pwd_mindays",    PROFILE_WIDGET_SPIN_BUTTON },
	{ "profile_settings_between",     "pwd_warndays",   PROFILE_WIDGET_SPIN_BUTTON },
	{ "profile_settings_minuid",      "umin",           PROFILE_WIDGET_SPIN_BUTTON },
	{ "profile_settings_mingid",      "gmin",           PROFILE_WIDGET_SPIN_BUTTON },
	{ "profile_settings_maxuid",      "umax",           PROFILE_WIDGET_SPIN_BUTTON },
	{ "profile_settings_maxgid",      "gmax",           PROFILE_WIDGET_SPIN_BUTTON },
	{NULL}
};

void
profile_settings_clear_dialog ()
{
	   GtkWidget *widget;
	   ProfileWidget *w;

	   for (w = profile_widgets;
		w->name != NULL;
		w++)
	   {
		   widget = xst_dialog_get_widget (tool->main_dialog, w->name);
		   switch (w->widget_type)
		   {
		   case PROFILE_WIDGET_ENTRY:
			   gtk_entry_set_text (GTK_ENTRY (widget), "");
			   break;
		   case PROFILE_WIDGET_SPIN_BUTTON:
			   gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), 0);
			   break;
		   case PROFILE_WIDGET_OPTION_MENU:
			   gtk_option_menu_remove_menu (GTK_OPTION_MENU (widget));
			   break;
		   default:
			   g_warning ("Should not be here");
		   }
	   }
}

void
profile_settings_save_data (xmlNodePtr node)
{
	   GtkWidget *widget;
	   ProfileWidget *w;

	   for (w = profile_widgets;
		w->name != NULL;
		w++)
	   {
		   widget = xst_dialog_get_widget (tool->main_dialog, w->name);
		   switch (w->widget_type)
		   {
		   case PROFILE_WIDGET_ENTRY:
			   xst_xml_element_add_with_content (node, w->xml_tag, gtk_entry_get_text (GTK_ENTRY (widget)));
			   break;
		   case PROFILE_WIDGET_SPIN_BUTTON:
			   xst_xml_element_add_with_content (node,
							     w->xml_tag,
							     g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget))));
			   break;
		   case PROFILE_WIDGET_OPTION_MENU:
			   xst_xml_element_add_with_content (node,
							     w->xml_tag,
							     g_list_nth_data (groups_list, gtk_option_menu_get_history (GTK_OPTION_MENU (widget))));
			   break;
		   default:
			   g_warning ("Should not be here");
		   }
	   }
	   
	   xst_xml_element_add_with_content (node, "mailbox_dir", "/var/mail");
	   xst_xml_element_add_with_content (node, "skel_dir", "/etc/skel/");
	   xst_xml_element_add_with_content (node, "login_defs", "1");
}

gchar*
profile_settings_check (void)
{
	   GtkWidget *widget;
	   gchar *value;
	   
	   widget = xst_dialog_get_widget (tool->main_dialog, "profile_settings_name");
	   value = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
	   if (strlen (value) <= 0)
			 return _("The profile must have a name");

	   widget = xst_dialog_get_widget (tool->main_dialog, "profile_settings_home");
	   value = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
	   if (strlen (value) <= 0)
			 return _("The profile must have a default home");

	   widget = xst_dialog_get_widget (tool->main_dialog, "profile_settings_shell_entry");
	   value = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
	   if (strlen (value) <= 0)
			 return _("The profile must have a default shell");

	   widget = xst_dialog_get_widget (tool->main_dialog, "profile_settings_shell_entry");
	   value = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
	   if (strlen (value) <= 0)
			 return _("The profile must have a default shell");
	   
	   return NULL;
}

void
profile_settings_set_data (xmlNodePtr node)
{
	GtkWidget *widget;
	ProfileWidget *w;
	gchar *value;
	GList *element;
	gint counter = 0;

	for (w = profile_widgets;
	     w->name != NULL;
	     w++)
	{
		widget = xst_dialog_get_widget (tool->main_dialog, w->name);
		switch (w->widget_type)
		{
		case PROFILE_WIDGET_ENTRY:
			value = xst_xml_get_child_content (node, w->xml_tag);
			gtk_entry_set_text (GTK_ENTRY (widget), value);
			g_free (value);
			break;
		case PROFILE_WIDGET_SPIN_BUTTON:
			value = xst_xml_get_child_content (node, w->xml_tag);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), g_ascii_strtod (value, NULL));
			g_free (value);
			break;
		case PROFILE_WIDGET_OPTION_MENU:
			value = xst_xml_get_child_content (node, w->xml_tag);
			element = g_list_first (groups_list);

			while ((element != NULL) && (strcmp (element->data, value) != 0))
			{
				element = element->next;
				counter++;
			}
			gtk_option_menu_set_history (GTK_OPTION_MENU (widget), counter);
			g_free (value);
			break;
		default:
			g_warning ("Should not be here");
		}
	}	
}

static gboolean
check_profile_delete (xmlNodePtr node)
{
	gchar *buf, *profile_name;
	GtkWindow *parent;
	GtkWidget *dialog;
	gint reply;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	profile_name = xst_xml_get_child_content (node, "name");

	if (!profile_name)
	{
		g_warning ("check_profile_delete: Can't get profile name");
		return FALSE;
	}

	if (strcmp (profile_name, "Default") == 0)
	{
		buf = g_strdup (_("The profile Default must not be deleted."));
		show_error_message ("profile_settings_dialog", buf);
		g_free (profile_name);
		g_free (buf);
		return FALSE;
	}

	buf = g_strdup_printf (_("Are you sure you want to delete the profile called %s?"), profile_name);
	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, buf);
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (buf);
	g_free (profile_name);
	
	if (reply == GTK_RESPONSE_NO)
		return FALSE;
        else
		return TRUE;
}

gboolean
profile_delete (xmlNodePtr node)
{
	if (check_profile_delete (node)) {
		xst_xml_element_destroy (node);
		return TRUE;
	}

	return FALSE;
}
