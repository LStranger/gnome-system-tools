/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user_settings.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#include <stdlib.h>
#include <ctype.h>
#include <gnome.h>

#include "xst.h"
#include "user_settings.h"
#include "profile.h"
#include "passwd.h"

#define USER_ACCOUNT_PASSWD_CHANGED "changed"

extern XstTool *tool;

static void user_account_gui_setup_profile (UserAccountGui *gui);

static void
user_account_gui_add (GtkButton *button, gpointer data)
{
	UserAccountGui *gui = data;
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = gui->all;
	members = gui->member;

	while (all->selection) {
		row = GPOINTER_TO_INT (all->selection->data);
		gtk_clist_get_text (all, row, 0, &name);
		my_gtk_clist_append (members, g_strdup (name));
		gtk_clist_remove (all, row);
	}
}

static void
user_account_gui_remove (GtkButton *button, gpointer data)
{
	UserAccountGui *gui = data;
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = gui->all;
	members = gui->member;

	while (members->selection) {
		row = GPOINTER_TO_INT (members->selection->data);
		gtk_clist_get_text (members, row, 0, &name);
		my_gtk_clist_append (all, g_strdup (name));
		gtk_clist_remove (members, row);
	}
}

static void
user_account_gui_member_select (GtkCList *clist, gint row, gint column, GdkEventButton *event,
				gpointer data)
{
	UserAccountGui *gui = data;

	g_return_if_fail (xst_tool_get_access (tool));

	if (clist->selection) {
		gtk_widget_set_sensitive (GTK_WIDGET (gui->remove), TRUE);
	} else {
		gtk_widget_set_sensitive (GTK_WIDGET (gui->remove), FALSE);
	}
}

static void
user_account_gui_all_select (GtkCList *clist, gint row, gint column, GdkEventButton *event,
			     gpointer data)
{
	UserAccountGui *gui = data;

	g_return_if_fail (xst_tool_get_access (tool));

	if (clist->selection)
		gtk_widget_set_sensitive (GTK_WIDGET (gui->add), TRUE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (gui->add), FALSE);
}

static void
user_account_gui_profile_changed (GtkMenuShell *menushell, gpointer data)
{
	UserAccountGui *gui = data;
	UserAccount *account;
	gchar *profile;	

	user_account_destroy (gui->account);
	profile = xst_ui_option_menu_get_selected_string (gui->profile_menu);
	account = user_account_new (profile);
	g_free (profile);
	gui->account = account;
	user_account_gui_setup_profile (gui);
}

enum {
	NOTEBOOK_PAGE_MANUAL,
	NOTEBOOK_PAGE_RANDOM,
};

static void
user_account_passwd_toggled (GtkToggleButton *toggle, gpointer data)
{
	UserAccountGui *gui = data;

	if (gtk_toggle_button_get_active (gui->pwd_manual))
		gtk_notebook_set_page (gui->pwd_notebook, NOTEBOOK_PAGE_MANUAL);
	else {
		gtk_notebook_set_page (gui->pwd_notebook, NOTEBOOK_PAGE_RANDOM);
		gtk_signal_emit_by_name (GTK_OBJECT (gui->pwd_random_new),
					 "clicked", gui);
	}
}

static void
user_account_passwd_random_new (GtkButton *button, gpointer data)
{
	gchar *passwd;
	UserAccountGui *gui = data;

	passwd = passwd_get_random ();
	gtk_label_set_text (gui->pwd_random_label, passwd);
	g_free (passwd);
}

static void
user_account_passwd_changed (GtkEditable *entry, gpointer data)
{
	gtk_object_set_data (GTK_OBJECT (entry), USER_ACCOUNT_PASSWD_CHANGED, GINT_TO_POINTER (TRUE));
}

static void
user_account_comment_changed (GtkEditable *editable, gchar *new_text, gint new_text_length,
			      gint *position, gpointer data)
{
	gint i;
	gchar *buf = NULL;
	UserAccountGui *gui = data;
	
	gtk_signal_handler_block_by_func (GTK_OBJECT (editable),
					  GTK_SIGNAL_FUNC (user_account_comment_changed),
					  data);

	for (i = 0; i < new_text_length; i++) {
		if (iscntrl (new_text[i]) || new_text[i] == ',' ||
		    new_text[i] == '=' || new_text[i] == ':') {
			buf = g_strdup_printf (N_("Invalid character '%c'.\n"
				"This field doesn't accept control characters ',' ':' and '='."),
					       new_text[i]);
			break;
		}
	}

	if (buf)
		user_account_gui_error (GTK_WINDOW (gui->top), buf);
	else
		gtk_editable_insert_text (editable, new_text, new_text_length, position);	

	gtk_signal_handler_unblock_by_func (GTK_OBJECT (editable),
					    GTK_SIGNAL_FUNC (user_account_comment_changed),
					    data);
	
	gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");
}

static void
user_account_grab_focus (GtkWidget *w, gpointer data)
{
	GtkWidget *widget = GTK_WIDGET (data);

	gtk_widget_grab_focus (widget);
}

static gint
char_sort_func (gconstpointer a, gconstpointer b)
{
	return (strcmp (a, b));
}

static GList *
get_group_list (gchar *field, xmlNodePtr user_node)
{
	GList *list = NULL;
	xmlNodePtr node, u;

	node = get_corresp_field (user_node);
	
	if (!node)
		return NULL;

	for (u = xst_xml_element_find_first (node, "group");
	     u;
	     u = xst_xml_element_find_next (u, "group"))
	{

		if (check_node_complexity (u))
			list = g_list_prepend (list, xst_xml_get_child_content (u, field));
	}

	return list;
}

static void
user_account_connect_signals (UserAccountGui *gui)
{
	gtk_signal_connect (GTK_OBJECT (gui->profile_button), "clicked",
			    GTK_SIGNAL_FUNC (profile_table_run), NULL);
	
	gtk_signal_connect (GTK_OBJECT (gui->add), "clicked",
			    GTK_SIGNAL_FUNC (user_account_gui_add), gui);
	gtk_signal_connect (GTK_OBJECT (gui->remove), "clicked",
			    GTK_SIGNAL_FUNC (user_account_gui_remove), gui);

	gtk_signal_connect (GTK_OBJECT (gui->all), "select_row",
			    GTK_SIGNAL_FUNC (user_account_gui_all_select), gui);
	gtk_signal_connect (GTK_OBJECT (gui->all), "unselect_row",
			    GTK_SIGNAL_FUNC (user_account_gui_all_select), gui);

	gtk_signal_connect (GTK_OBJECT (gui->member), "select_row",
			    GTK_SIGNAL_FUNC (user_account_gui_member_select), gui);
	gtk_signal_connect (GTK_OBJECT (gui->member), "unselect_row",
			    GTK_SIGNAL_FUNC (user_account_gui_member_select), gui);

	gtk_signal_connect (GTK_OBJECT (gui->pwd_manual), "toggled",
			    GTK_SIGNAL_FUNC (user_account_passwd_toggled), gui);
	gtk_signal_connect (GTK_OBJECT (gui->pwd_random), "toggled",
			    GTK_SIGNAL_FUNC (user_account_passwd_toggled), gui);
	gtk_signal_connect (GTK_OBJECT (gui->pwd_random_new), "clicked",
			    GTK_SIGNAL_FUNC (user_account_passwd_random_new), gui);

	gtk_signal_connect (GTK_OBJECT (gui->pwd1), "changed",
			    GTK_SIGNAL_FUNC (user_account_passwd_changed), gui);
	gtk_signal_connect (GTK_OBJECT (gui->pwd2), "changed",
			    GTK_SIGNAL_FUNC (user_account_passwd_changed), gui);

	gtk_signal_connect (GTK_OBJECT (gui->name), "activate",
			    GTK_SIGNAL_FUNC (user_account_grab_focus), (gpointer) gui->comment);
	gtk_signal_connect (GTK_OBJECT (gui->comment), "activate",
			    GTK_SIGNAL_FUNC (user_account_grab_focus), (gpointer) gui->office);
	gtk_signal_connect (GTK_OBJECT (gui->office), "activate",
			    GTK_SIGNAL_FUNC (user_account_grab_focus), (gpointer) gui->wphone);
	gtk_signal_connect (GTK_OBJECT (gui->wphone), "activate",
			    GTK_SIGNAL_FUNC (user_account_grab_focus), (gpointer) gui->hphone);
	gtk_signal_connect (GTK_OBJECT (gui->pwd1), "activate",
			    GTK_SIGNAL_FUNC (user_account_grab_focus), (gpointer) gui->pwd2);
	gtk_signal_connect (GTK_OBJECT (gui->uid), "activate",
			    GTK_SIGNAL_FUNC (user_account_grab_focus), (gpointer) gui->home);
	
	gtk_signal_connect (GTK_OBJECT (gui->comment), "insert-text",
			    GTK_SIGNAL_FUNC (user_account_comment_changed),
			    (gpointer) gui);
	gtk_signal_connect (GTK_OBJECT (gui->office), "insert-text",
			    GTK_SIGNAL_FUNC (user_account_comment_changed),
			    (gpointer) gui);
	gtk_signal_connect (GTK_OBJECT (gui->wphone), "insert-text",
			    GTK_SIGNAL_FUNC (user_account_comment_changed),
			    (gpointer) gui);
	gtk_signal_connect (GTK_OBJECT (gui->hphone), "insert-text",
			    GTK_SIGNAL_FUNC (user_account_comment_changed),
			    (gpointer) gui);	
}

UserAccountGui *
user_account_gui_new (UserAccount *account, GtkWidget *parent)
{
	UserAccountGui *gui;

	if (!account)
		return NULL;
	
	gui = g_new0 (UserAccountGui, 1);
	gui->account = account;
	gui->xml = glade_xml_new (tool->glade_path, NULL);
	gui->top = parent;

	gui->basic_frame = glade_xml_get_widget (gui->xml, "user_settings_basic");
	gui->name    = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_name"));
	gui->comment = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_comment"));
	gui->contact_frame = glade_xml_get_widget (gui->xml, "user_settings_contact_info");
	gui->office  = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_office"));
	gui->wphone  = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_wphone"));
	gui->hphone  = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_hphone"));
	gui->home    = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_home"));
	gui->shell   = GTK_COMBO (glade_xml_get_widget (gui->xml, "user_settings_shell"));
	gui->uid     = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_settings_uid"));
	gui->advanced = glade_xml_get_widget (gui->xml, "user_settings_advanced");
	gui->profile_box = glade_xml_get_widget (gui->xml, "user_settings_profile_box");
	gui->profile_menu = GTK_OPTION_MENU (glade_xml_get_widget (gui->xml, "user_settings_profile_menu"));
	gui->profile_button = glade_xml_get_widget (gui->xml, "user_settings_profile_button");

	gui->group_box = glade_xml_get_widget (gui->xml, "user_settings_group_box");
	gui->group_extra = glade_xml_get_widget (gui->xml, "user_settings_group_extra");
	gui->group   = GTK_COMBO (glade_xml_get_widget (gui->xml, "user_settings_group"));
	gui->all     = GTK_CLIST (glade_xml_get_widget (gui->xml, "user_settings_gall"));
	gui->member  = GTK_CLIST (glade_xml_get_widget (gui->xml, "user_settings_gmember"));
	gui->add     = glade_xml_get_widget (gui->xml, "user_settings_add");
	gui->remove  = glade_xml_get_widget (gui->xml, "user_settings_remove");

	gui->pwd_box = glade_xml_get_widget (gui->xml, "user_passwd_box");
	gui->pwd_notebook = GTK_NOTEBOOK (glade_xml_get_widget (gui->xml, "user_passwd_notebook"));
	gui->pwd_manual = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_manual"));
	gui->pwd_random = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_random"));
	gui->pwd_random_label = GTK_LABEL (glade_xml_get_widget (gui->xml, "user_passwd_random_label"));
	gui->pwd_random_new = glade_xml_get_widget (gui->xml, "user_passwd_random_new");
	gui->pwd_frame = glade_xml_get_widget (gui->xml, "user_passwd_frame");
	gui->pwd1 = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_passwd_entry1"));
	gui->pwd2 = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_passwd_entry2"));
	gui->quality = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_quality"));
	gui->optional = glade_xml_get_widget (gui->xml, "user_passwd_optional");
	gui->min  = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_min"));
	gui->max  = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_max"));
	gui->days = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_days"));

	user_account_connect_signals (gui);
	
	return gui;
}

static void
user_account_shells_setup (UserAccountGui *gui)
{
	xmlNodePtr root, node;
	GtkWidget *li;

	root = xst_xml_doc_get_root (tool->config);
	root = xst_xml_element_find_first (root, "shells");

	if (!root)
		return;

	gtk_list_clear_items (GTK_LIST (gui->shell->list), 0, -1);
	
	node = xst_xml_element_find_first (root, "shell");
	while (node) {
		li = gtk_list_item_new_with_label (xst_xml_element_get_content (node));
		gtk_widget_show (li);
		gtk_container_add (GTK_CONTAINER (gui->shell->list), li);
		
		node = xst_xml_element_find_next (node, "shell");
	}
}

static void
user_account_groups_setup (GtkCombo *combo, xmlNodePtr node)
{
	GList *tmp_list, *items;
	gchar *name;

	items = NULL;
	tmp_list = get_group_list ("name", node);
	while (tmp_list) {
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		items = g_list_append (items, name);
	}
	g_list_free (tmp_list);

	items = g_list_sort (items, char_sort_func);

	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
}

static void
user_account_comment_setup (UserAccountGui *gui)
{
	gint i;
	gchar **comment = gui->account->comment;
	GtkEntry *entries[] = {
		GTK_ENTRY (gui->comment),
		GTK_ENTRY (gui->office),
		GTK_ENTRY (gui->wphone),
		GTK_ENTRY (gui->hphone)
	};

	if (!comment)
		return;
	
	for (i = 0; i < 4; i++) {
		if (comment[i] == NULL)
			break;
		else
			xst_ui_entry_set_text (entries[i], comment[i]);
	}
}

static void
user_account_gui_setup_profile (UserAccountGui *gui)
{
	UserAccount *account = gui->account;
	
	xst_ui_entry_set_text (gui->home, account->home);
	xst_ui_entry_set_text (GTK_ENTRY (gui->shell->entry), account->shell);
	gtk_spin_button_set_value (gui->uid, atoi (account->uid));
	xst_ui_entry_set_text (GTK_ENTRY (gui->group->entry), account->group);
	gtk_spin_button_set_value (gui->min, account->pwd_mindays);
	gtk_spin_button_set_value (gui->max, account->pwd_maxdays);
	gtk_spin_button_set_value (gui->days, account->pwd_warndays);
}

static void
setup_advanced_add (UserAccountGui *gui, GtkWidget *notebook)
{
	GtkWidget *box, *label, *menu_item;
	GSList *list = profile_table_get_list ();
	Profile *pf = profile_table_get_profile (NULL);

	/* Reparent widgets */
	box = gtk_vbox_new (FALSE, 3);
	gtk_widget_reparent (GTK_WIDGET (gui->basic_frame), box);
	gtk_box_set_child_packing (GTK_BOX (box),
				   GTK_WIDGET (gui->basic_frame),
				   FALSE, FALSE, 0, GTK_PACK_START);
	gtk_widget_reparent (GTK_WIDGET (gui->contact_frame), box);
	gtk_box_set_child_packing (GTK_BOX (box),
				   GTK_WIDGET (gui->contact_frame),
				   FALSE, FALSE, 0, GTK_PACK_START);	
	gtk_widget_reparent (GTK_WIDGET (gui->profile_box), box);
	gtk_box_set_child_packing (GTK_BOX (box),
				   GTK_WIDGET (gui->profile_box),
				   FALSE, FALSE, 0, GTK_PACK_START);
	label = gtk_label_new (_("Identity"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
	gtk_widget_show_all (box);

	box = gtk_vbox_new (FALSE, 3);
	gtk_widget_reparent (GTK_WIDGET (gui->group_box), box);
	gtk_widget_reparent (GTK_WIDGET (gui->group_extra), box);
	label = gtk_label_new (_("Groups"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
	gtk_widget_show_all (box);
	
	box = gtk_vbox_new (FALSE, 3);
	gtk_widget_reparent (GTK_WIDGET (gui->pwd_box), box);
	gtk_widget_reparent (GTK_WIDGET (gui->optional), box);
	label = gtk_label_new (_("Password"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
	gtk_widget_show_all (box);

	while (list) {
		menu_item = xst_ui_option_menu_add_string (gui->profile_menu, list->data);
		gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
				    GTK_SIGNAL_FUNC (user_account_gui_profile_changed), gui);
		list = list->next;
	}
	g_slist_free (list);
	xst_ui_option_menu_set_selected_string (gui->profile_menu, pf->name);
}

static void
setup_basic_add (UserAccountGui *gui, GtkWidget *notebook)
{
	GtkWidget *container;
	
	container = glade_xml_get_widget (gui->xml, "user_druid_identity");
	gtk_widget_reparent (gui->basic_frame, container);
	gtk_widget_reparent (gui->contact_frame, container);
	gtk_widget_show_all (container);

	container = glade_xml_get_widget (gui->xml, "user_druid_password");
	gtk_widget_reparent (gui->pwd_frame, container);
	gtk_widget_show_all (container);
}

static void
setup_basic (UserAccountGui *gui, GtkWidget *notebook)
{
	GtkWidget *box, *label;

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

	label = glade_xml_get_widget (gui->xml, "user_settings_comment_label");
	gtk_label_set_text (GTK_LABEL (label), N_("Real name:"));
	
	/* Reparent widgets */
	box = gtk_vbox_new (FALSE, 3);
	gtk_widget_reparent (GTK_WIDGET (gui->basic_frame), box);
	gtk_widget_reparent (GTK_WIDGET (gui->contact_frame), box);
	gtk_widget_reparent (GTK_WIDGET (gui->pwd_frame), box);
	label = gtk_label_new (_("Identity"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
	gtk_widget_show_all (box);
}

static void
setup_advanced (UserAccountGui *gui, GtkWidget *notebook)
{
	GtkWidget *box, *label;

	/* Reparent widgets */
	box = gtk_vbox_new (FALSE, 3);
	gtk_widget_reparent (GTK_WIDGET (gui->basic_frame), box);
	gtk_widget_reparent (GTK_WIDGET (gui->contact_frame), box);
	gtk_widget_reparent (GTK_WIDGET (gui->advanced), box);
	label = gtk_label_new (_("Identity"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
	gtk_widget_show_all (box);

	box = gtk_vbox_new (FALSE, 3);
	gtk_widget_reparent (GTK_WIDGET (gui->group_box), box);
	gtk_widget_reparent (GTK_WIDGET (gui->group_extra), box);
	label = gtk_label_new (_("Groups"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
	gtk_widget_show_all (box);
	
	box = gtk_vbox_new (FALSE, 3);
	gtk_widget_reparent (GTK_WIDGET (gui->pwd_box), box);
	gtk_widget_reparent (GTK_WIDGET (gui->optional), box);
	label = gtk_label_new (_("Password"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
	gtk_widget_show_all (box);
}

void
user_account_gui_setup (UserAccountGui *gui, GtkWidget *top)
{
	GtkWidget *notebook;
	GList *users, *items;
	UserAccount *account = gui->account;
	
	user_account_shells_setup (gui);
	user_account_groups_setup (gui->group, account->node);
	user_account_comment_setup (gui);
	
	xst_ui_entry_set_text (gui->name, account->name);

	user_account_gui_setup_profile (gui);

	gtk_clist_set_auto_sort (gui->all, TRUE);
	gtk_clist_set_auto_sort (gui->member, TRUE);
	
	my_gtk_clist_append_items (gui->member, (GList *)account->extra_groups);
	/* Others */
	users = get_group_list ("name", account->node);
	items = my_g_list_remove_duplicates (users, (GList *)account->extra_groups);
	my_gtk_clist_append_items (gui->all, items);

	if (account->password && (strlen (account->password) > 2)) {
		gtk_signal_handler_block_by_func (GTK_OBJECT (gui->pwd1),
						  user_account_passwd_changed,
						  gui);
		gtk_signal_handler_block_by_func (GTK_OBJECT (gui->pwd2),
						  user_account_passwd_changed,
						  gui);
						  
		gtk_entry_set_text (gui->pwd1, "********");
		gtk_entry_set_text (gui->pwd2, "********");

		gtk_signal_handler_unblock_by_func (GTK_OBJECT (gui->pwd1),
						  user_account_passwd_changed,
						  gui);
		gtk_signal_handler_unblock_by_func (GTK_OBJECT (gui->pwd2),
						  user_account_passwd_changed,
						  gui);
	}
	
#ifdef HAVE_LIBCRACK
	gtk_widget_show (GTK_WIDGET (gui->quality));
	gtk_toggle_button_set_active (gui->quality, TRUE);
#endif
	/* Make notebook */

	notebook = gtk_notebook_new ();
	
	if (account->new) {
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED)
			setup_advanced_add (gui, notebook);
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_BASIC)
			setup_basic_add (gui, notebook);
	} else {
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED)
			setup_advanced (gui, notebook);
		
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_BASIC)
			setup_basic (gui, notebook);
	}

	if (top) {
		gtk_widget_show (notebook);
		gtk_container_add (GTK_CONTAINER (top), notebook);
	}
}

static gchar**
user_account_gui_save_comment (UserAccountGui *gui)
{
	gchar *buf;
	gchar **val;

	buf = g_strjoin (",",
			 gtk_entry_get_text (gui->comment),
			 gtk_entry_get_text (gui->office),
			 gtk_entry_get_text (gui->wphone),
			 gtk_entry_get_text (gui->hphone),
			 NULL);

	val = g_strsplit (buf, ",", 4);
	g_free (buf);
	
	return val;
}

static gchar *
warnings_join (GList *warnings)
{
	GList *l;
	gchar *text = NULL;
	gchar *text2;

	for (l = warnings; l; l = l->next) {
		text2 = text;
		if (text)
			text = g_strdup_printf ("%s\n%s", text, (gchar *) l->data);
		else
			text = g_strdup ((gchar *) l->data);
		g_free (text2);
	}

	text2 = text;
	text = g_strdup_printf ("The information you just entered created the following warnings:\n\n%s\n\nContinue anyways?", text);
	g_free (text2);

	return text;
}

static void
gui_warnings_cb (gint button, gpointer data)
{
	gboolean *reply = (gboolean *) data;

	if (button == 0) /* Yes button */
		*reply = TRUE;
	else /* No button */
		*reply = FALSE;
}

static gboolean
user_account_gui_warnings (GtkWindow *parent, GList *warnings)
{
	GtkWidget *d;
	gchar *text;
	gboolean reply;
	
	g_return_val_if_fail (warnings != NULL, TRUE);

	text = warnings_join (warnings);
	g_return_val_if_fail (text != NULL, TRUE);

	if (parent)
		d = gnome_question_dialog_parented (text, gui_warnings_cb, &reply, parent);
	else
		d = gnome_question_dialog (text, gui_warnings_cb, &reply);

	gnome_dialog_run (GNOME_DIALOG (d));
	g_free (text);

	return reply;
}

gboolean
user_account_gui_save (UserAccountGui *gui)
{
	UserAccount *account;
	xmlNodePtr node = gui->account->node;
	GtkWindow *parent = GTK_WINDOW (gui->top);
	GList *warnings;
	gchar *buf, *error;
	gint row = 0;

	account = g_new0 (UserAccount, 1);
	account->node = node;
	account->new = gui->account->new;
	
	buf = gtk_entry_get_text (gui->name);
	account->name = g_strdup (buf);
	
	account->comment = user_account_gui_save_comment (gui);
	
	buf = gtk_entry_get_text (gui->home);
	account->home = g_strdup (buf);
	
	buf = gtk_entry_get_text (GTK_ENTRY (gui->shell->entry));
	account->shell = g_strdup (buf);
	
	buf = g_strdup_printf ("%d", gtk_spin_button_get_value_as_int (gui->uid));
	account->uid = (buf);

	buf = gtk_entry_get_text (GTK_ENTRY (gui->group->entry));
	account->group = g_strdup (buf);

	/* Password */
	if (gtk_toggle_button_get_active (gui->pwd_random)) {
		gtk_label_get (gui->pwd_random_label, &buf);
		account->password = g_strdup (buf);
	} else {
		gboolean changed1 = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (gui->pwd1),
									  USER_ACCOUNT_PASSWD_CHANGED));
		gboolean changed2 = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (gui->pwd2),
									  USER_ACCOUNT_PASSWD_CHANGED));
		if (changed1 || changed2) {
			gchar *buf1 = gtk_entry_get_text (gui->pwd1);

			buf = gtk_entry_get_text (gui->pwd2);
			if ((error = passwd_check (buf1, buf, gtk_toggle_button_get_active (gui->quality)))) {
				user_account_gui_error (parent, error);
				user_account_destroy (account);
				return FALSE;
			} else
				account->password = g_strdup (buf);
		} else
			account->password = NULL;
	}
	
	account->pwd_mindays = gtk_spin_button_get_value_as_int (gui->min);
	account->pwd_maxdays = gtk_spin_button_get_value_as_int (gui->max);
	account->pwd_warndays = gtk_spin_button_get_value_as_int (gui->days);

	account->extra_groups = NULL;
	while (gtk_clist_get_text (gui->member, row++, 0, &buf))
		account->extra_groups = g_slist_prepend (account->extra_groups, buf);

	if ((error = user_account_check (account))) {
		user_account_gui_error (parent, error);
		user_account_destroy (account);
		return FALSE;
	} else if ((warnings = user_account_check_warnings (account))) {
		if (!user_account_gui_warnings (parent, warnings)) {
			user_account_destroy_warnings (warnings);
			user_account_destroy (account);
			return FALSE;
		}
		user_account_destroy_warnings (warnings);
	}

	user_account_destroy (gui->account);
	gui->account = account;
	return TRUE;
}

void
user_account_gui_error (GtkWindow *parent, gchar *error)
{
	GtkWidget *d;

	if (parent)
		d = gnome_error_dialog_parented (error, parent);
	else
		d = gnome_error_dialog (error);

	gnome_dialog_run (GNOME_DIALOG (d));
	g_free (error);
}

void
user_account_gui_destroy (UserAccountGui *gui)
{
	if (gui) {
		user_account_destroy (gui->account);
		gtk_object_unref (GTK_OBJECT (gui->xml));
		g_free (gui);
	}
}
