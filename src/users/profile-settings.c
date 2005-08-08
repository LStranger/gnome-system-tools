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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gst.h"
#include "user_group.h"
#include "table.h"
#include "user-group-xml.h"
#include "profile-settings.h"

extern GstTool *tool;
extern GList *groups_list;

ProfileWidget profile_widgets [] = {
	/* widget name                    xml label       */
	{ "profile_settings_name",        "name"          },
	{ "profile_settings_comment",     "comment"       },
	{ "profile_settings_home",        "home_prefix"   },
	{ "profile_settings_shell",       "shell",        },
	{ "profile_settings_group",       "group",        },
	{ "profile_settings_maxdays",     "pwd_maxdays",  },
	{ "profile_settings_mindays",     "pwd_mindays",  },
	{ "profile_settings_between",     "pwd_warndays", },
	{ "profile_settings_minuid",      "umin",         },
	{ "profile_settings_mingid",      "gmin",         },
	{ "profile_settings_maxuid",      "umax",         },
	{ "profile_settings_maxgid",      "gmax",         },
	{ NULL }
};

void
profile_settings_clear_dialog ()
{
	GtkWidget     *widget;
	GtkTreeModel  *model;
	ProfileWidget *w;

	for (w = profile_widgets; w->name; w++) {
		widget = gst_dialog_get_widget (tool->main_dialog, w->name);

		if (GTK_IS_SPIN_BUTTON (widget))
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), 0);
		else if (GTK_IS_ENTRY (widget))
			gtk_entry_set_text (GTK_ENTRY (widget), "");
		else if (GTK_IS_COMBO_BOX (widget)) {
			model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
			gtk_list_store_clear (GTK_LIST_STORE (model));
		} else
			g_warning ("Should not be here");
	}
}

void
profile_settings_save_data (xmlNodePtr node)
{
	GtkWidget     *widget;
	GtkTreeModel  *model;
	GtkTreeIter    iter;
	ProfileWidget *w;
	gchar         *str;

	for (w = profile_widgets; w->name; w++) {
		widget = gst_dialog_get_widget (tool->main_dialog, w->name);

		if (GTK_IS_SPIN_BUTTON (widget)) {
			gst_xml_element_add_with_content (node,
							  w->xml_tag,
							  g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget))));
		} else if (GTK_IS_ENTRY (widget)) {
			gst_xml_element_add_with_content (node, w->xml_tag, gtk_entry_get_text (GTK_ENTRY (widget)));
		} else if (GTK_IS_COMBO_BOX (widget)) {
			model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

			if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
				gtk_tree_model_get (model, &iter, 0, &str, -1);
				gst_xml_element_add_with_content (node, w->xml_tag, str);
			}
		} else {
			g_warning ("Should not be here");
		}
	}
	   
	gst_xml_element_add_with_content (node, "mailbox_dir", "/var/mail");
	gst_xml_element_add_with_content (node, "skel_dir",    "/etc/skel/");
	gst_xml_element_add_with_content (node, "login_defs",  "1");
	profile_groups_save_data (node);
}

gchar*
profile_settings_check (void)
{
	GtkWidget *widget;
	gchar *value;
	   
	widget = gst_dialog_get_widget (tool->main_dialog, "profile_settings_name");
	value = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
	if (strlen (value) <= 0)
		return _("The profile must have a name");

	widget = gst_dialog_get_widget (tool->main_dialog, "profile_settings_home");
	value = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
	if (strlen (value) <= 0)
		return _("The profile must have a default home");

	widget = gst_dialog_get_widget (tool->main_dialog, "profile_settings_shell");
	value = (gchar *) gtk_entry_get_text (GTK_ENTRY (GTK_BIN (widget)->child));
	if (strlen (value) <= 0)
		return _("The profile must have a default shell");

	widget = gst_dialog_get_widget (tool->main_dialog, "profile_settings_shell");
	value = (gchar *) gtk_entry_get_text (GTK_ENTRY (GTK_BIN (widget)->child));
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

	for (w = profile_widgets; w->name; w++) {
		widget = gst_dialog_get_widget (tool->main_dialog, w->name);
		value = gst_xml_get_child_content (node, w->xml_tag);

		if (!value)
			continue;

		if (GTK_IS_SPIN_BUTTON (widget))
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), g_ascii_strtod (value, NULL));
		else if (GTK_IS_ENTRY (widget))
			gtk_entry_set_text (GTK_ENTRY (widget), value);
		else if (GTK_IS_COMBO_BOX_ENTRY (widget))
			gtk_entry_set_text (GTK_ENTRY (GTK_BIN (widget)->child), value);
		else if (GTK_IS_COMBO_BOX (widget)) {
			element = g_list_first (groups_list);

			while ((element != NULL) && (strcmp (element->data, value) != 0)) {
				element = element->next;
				counter++;
			}

			gtk_combo_box_set_active (GTK_COMBO_BOX (widget), counter);
		} else {
			g_warning ("Should not be here");
		}

		g_free (value);
	}
}

static gboolean
check_profile_delete (xmlNodePtr node)
{
	gchar *profile_name;
	GtkWidget *dialog, *parent;
	gboolean  is_default;
	gint reply;

	g_return_val_if_fail (node != NULL, FALSE);

	profile_name = gst_xml_get_child_content   (node, "name");
	is_default   = gst_xml_element_get_boolean (node, "default");

	parent = gst_dialog_get_widget (tool->main_dialog, "profile_settings_dialog");

	if (!profile_name)
	{
		g_warning ("check_profile_delete: Can't get profile name");
		return FALSE;
	}

	if (is_default)
	{
		show_error_message ("profile_settings_dialog",
				    _("The default profile should not be deleted"),
				    _("This profile is used for setting default data for new users"));
		g_free (profile_name);
		return FALSE;
	}

	dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("Delete profile \"%s\"?"), profile_name);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("You will not be able to recover this "
						    "profile after pressing \"apply\"."));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				NULL);

	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (profile_name);

	if (reply == GTK_RESPONSE_ACCEPT)
		return TRUE;
	else
		return FALSE;
}

gboolean
profile_delete (xmlNodePtr node)
{
	if (check_profile_delete (node)) {
		gst_xml_element_destroy (node);
		return TRUE;
	}

	return FALSE;
}
