/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Hans Petter Jansson <hpj@helixcode.com> and Arturo Espinosa <arturo@helixcode.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>

#include "global.h"

#include "callbacks.h"
#include "transfer.h"

#define d(x) x

GtkWidget *searchdomain_entry_selected = NULL;
GtkWidget *aliases_settings_entry_selected = NULL;
GtkWidget *dns_entry_selected = NULL;
int statichost_row_selected = -1;

#define SEARCHDOMAIN_ROW_SELECTED gtk_list_child_position (GTK_LIST (tool_widget_get ("searchdomain_list")), searchdomain_entry_selected)
#define ALIASES_SETTINGS_ROW_SELECTED gtk_list_child_position (GTK_LIST (tool_widget_get ("aliases_settings_list")), aliases_settings_entry_selected)
#define DNS_ROW_SELECTED gtk_list_child_position (GTK_LIST (tool_widget_get ("dns_list")), dns_entry_selected)


/* libglade callbacks */
void on_network_notebook_switch_page (GtkWidget *notebook, 
				      GtkNotebookPage *page,
				      gint page_num, gpointer user_data);

void on_statichost_changed (GtkWidget *w, gpointer null);

void on_statichost_add_clicked (GtkWidget *w, gpointer null);
void on_statichost_update_clicked (GtkWidget *w, gpointer null);
void on_statichost_delete_clicked (GtkWidget *w, gpointer null);

void on_statichost_list_select_row (GtkCList *clist, gint row, gint column, 
				    GdkEvent * event, gpointer user_data);
void on_statichost_list_unselect_row (GtkCList *clist, gint row, gint column, 
				      GdkEvent * event, gpointer user_data);

gint update_hint (GtkWidget *w, GdkEventFocus *e, gpointer null);

void on_connection_configure_clicked (GtkWidget *w, gpointer null);
void on_connection_delete_clicked (GtkWidget *w, gpointer null);
void on_connection_add_clicked (GtkWidget *w, gpointer null);

void on_dns_dhcp_toggled (GtkWidget *w, gpointer null);
void on_samba_use_toggled (GtkWidget *w, gpointer null);

static void
statichost_actions_set_sensitive (gboolean state)
{
	gtk_widget_set_sensitive (tool_widget_get ("statichost_delete"), state);
	gtk_widget_set_sensitive (tool_widget_get ("statichost_update"), state); 
}

static char *
fixup_text_list (GtkWidget *text)
{
	char *s2, *s;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (GTK_IS_EDITABLE (text), NULL);

	s = gtk_editable_get_chars (GTK_EDITABLE (text), 0, -1);
	
	for (s2 = strchr (s, '\n'); s2; s2 = strchr (s2, '\n'))
		*s2 = ' ';

	return s;
}

static char *
fixdown_text_list (char *s)
{
	char *s2;

	g_return_val_if_fail (s != NULL, NULL);

	for (s2 = strchr (s, ' '); s2; s2 = strchr (s2, ' '))
		*s2 = '\n';

	return s;
}

void 
on_network_admin_show (GtkWidget *w, gpointer user_data)
{
	char *access_no[] = { 
		"general_hbox",
		"samba_use",
		"samba_frame",
		"connections_bbox",
		"dns_dhcp",
		"dns_table",
		"statichost_table",
		NULL };

	char *access_yes[] = {
		"dns_list", 
		"search_list", 
		NULL} ;

	char *unsensitive[] = {
		"connection_delete",
		"connection_configure",
		"statichost_add",
		"statichost_update",
		"statichost_delete",
		NULL };
	int i;

	/* Those widgets that won't be available if you don't have the access. */
	for (i = 0; access_no[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (access_no[i]), tool_get_access());
	
	/* Those widgets that will be available, even if you don't have the access. */
	for (i = 0; access_yes[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (access_yes[i]), TRUE);
	
	/* Those widgets you should never have access to, and will be activated later on. */
	for (i = 0; unsensitive[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (unsensitive[i]), FALSE);
	
	gtk_clist_set_column_auto_resize (GTK_CLIST (tool_widget_get ("statichost_list")), 0, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (tool_widget_get ("statichost_list")), 1, TRUE);
}

void
on_network_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
																				gint page_num, gpointer user_data)
{
	gchar *entry[] = { "hostname", "connection_list", "dns_dhcp", "statichost_list" };
	
	if (tool_get_access () && entry[page_num])
		gtk_widget_grab_focus (tool_widget_get (entry[page_num]));
}

void
on_statichost_list_select_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	gchar *row_data;
	GtkWidget *w;
	gint pos = 0;

	statichost_row_selected = row;
	statichost_actions_set_sensitive (TRUE);
	
	/* Load aliases into entry widget */

	w = tool_widget_get ("statichost_enabled");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				      get_clist_checkmark (GTK_CLIST (clist), row, 0));

	pos = 0;
	gtk_clist_get_text (GTK_CLIST (clist), row, 1, &row_data);
	w = tool_widget_get ("ip");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), row_data, strlen (row_data), &pos);

	pos = 0;
	gtk_clist_get_text (GTK_CLIST (clist), row, 2, &row_data);
	row_data = fixdown_text_list (g_strdup (row_data));
	
	w = tool_widget_get ("alias");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), row_data, strlen (row_data), &pos);
	g_free (row_data);
}


void
on_statichost_list_unselect_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	GtkWidget *w;

	statichost_row_selected = -1;
	statichost_actions_set_sensitive (FALSE);
	
	w = tool_widget_get ("ip");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);

	w = tool_widget_get ("alias");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
}

void
on_statichost_changed (GtkWidget *w, gpointer null)
{
	gboolean enabled;

	enabled = tool_get_access () &&
		gtk_text_get_length (GTK_TEXT (tool_widget_get ("alias"))) &&
		check_ip_entry (GTK_ENTRY (tool_widget_get ("ip")), FALSE);
			
	gtk_widget_set_sensitive (tool_widget_get ("statichost_add"), enabled);
	gtk_widget_set_sensitive (tool_widget_get ("statichost_update"), enabled && 
				  GTK_WIDGET_SENSITIVE (tool_widget_get ("statichost_delete")));
}

static gboolean
is_char_ok (char c, EditableFilterRules rules)
{
	return isdigit (c) || c == '.' || 
		((rules & EF_ALLOW_ENTER) && c == '\n') ||
		((rules & EF_ALLOW_SPACE) && c == ' ') ||
		((rules & EF_ALLOW_TEXT) && isalpha (c));
}

void
filter_editable (GtkEditable *editable, const gchar *text, gint length, gint *pos, gpointer data)
{
	int i, l = 0;
	char *s = NULL;
	EditableFilterRules rules = GPOINTER_TO_INT (data);

	d(g_print ("got: (%d) `%.*s'\n", length, length, text));

	/* thou shalt optimize for the common case */
	if (length == 1) {
		if (is_char_ok (*text, rules))
			goto text_changed_success;
		else
			goto text_changed_fail;
	}

	/* if it is a paste we have to check all of things */
	s = g_new0 (char, length);

	for (i=0; i<length; i++)
		if (is_char_ok (text[i], rules))
			s[l++] = text[i];
		else
			d(g_print ("rejecting: (%d) `%c'\n", text[i], text[i]));

	if (l == length)
		goto text_changed_success;

	d(g_print ("setting: (%d) `%.*s'\n", l, l, s));

 text_changed_fail:
	gdk_beep ();
	
	gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");
	if (s) {
		gtk_signal_handler_block_by_func (GTK_OBJECT (editable),
						  filter_editable, data);
		gtk_editable_insert_text (editable, s, l, pos);
		gtk_signal_handler_unblock_by_func (GTK_OBJECT (editable),
						    filter_editable, data);
	}
	return;

 text_changed_success:
	d(g_message ("success!"));
#if 0
	if (! (rules & EF_STATIC_HOST))
		tool_modified_cb ();
#endif
}

void
on_statichost_add_clicked (GtkWidget * button, gpointer user_data)
{
	GtkWidget *clist, *w;
	char *entry[4];
	int row;

	g_return_if_fail (tool_get_access());

	entry[0] = entry[3] = NULL;
	
	entry[1] = gtk_editable_get_chars (
		GTK_EDITABLE (tool_widget_get ("ip")), 0, -1);

	entry[2] = fixup_text_list (tool_widget_get ("alias"));
		
	clist = tool_widget_get ("statichost_list");

	row = gtk_clist_append (GTK_CLIST (clist), entry);

	w = tool_widget_get ("statichost_enabled");
	set_clist_checkmark (GTK_CLIST (clist), row, 0,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));

	w = tool_widget_get ("alias");

#if 0
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), entry[2], strlen (entry[2]), &pos);
#endif

}


void
on_statichost_delete_clicked (GtkWidget * button, gpointer user_data)
{
	gchar *txt, *name;
	GtkWidget *parent, *dialog;
	gint res;

	g_return_if_fail (tool_get_access());
	g_return_if_fail (statichost_row_selected != -1);

	parent = tool_get_top_window ();
	gtk_clist_get_text (GTK_CLIST (tool_widget_get ("statichost_list")),
			    statichost_row_selected, 2, &name);

	txt = g_strdup_printf (_("Are you sure you want to delete the aliases for %s?"), name);
	dialog = gnome_question_dialog_parented (txt, NULL, NULL, GTK_WINDOW (parent));
	res = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
	g_free (txt);

	if (res) return;
		
	gtk_clist_remove (GTK_CLIST (tool_widget_get ("statichost_list")), statichost_row_selected);
	tool_set_modified (TRUE);
	statichost_actions_set_sensitive (FALSE);
}

void
on_statichost_update_clicked (GtkWidget *b, gpointer null)
{
	GtkWidget *clist, *w;
	char *s;

	g_return_if_fail (tool_get_access());

	clist = tool_widget_get ("statichost_list");

	w = tool_widget_get ("statichost_enabled");
	set_clist_checkmark (GTK_CLIST (clist), statichost_row_selected, 0,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));

	gtk_clist_set_text (GTK_CLIST (clist), statichost_row_selected, 1,
			    gtk_editable_get_chars (
				    GTK_EDITABLE (tool_widget_get ("ip")), 0, -1));
	
	w = tool_widget_get ("alias");
	s = fixup_text_list (w);

	gtk_clist_set_text (GTK_CLIST (clist), statichost_row_selected, 2, s);

#if 0
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), s, strlen (s), &pos);
#endif
	g_free (s);
}

/* yeah, I don't like this formatting either */
static const char *hint_entry[][3] = { {
	"hostname", "general_help", 		
	N_("This is your host name.  It is ending one day at a time.") 
},{
	"description", "general_help", 
	N_("Your computer; what not.")
},{
	"workgroup", "general_help",
	N_("dum de dum")
},{
	"wins_ip", "general_help", 
	N_("this is boring")
},{
	"domain", "dns_help",     
	N_("Enter your DNS domain (bug-buddy.org)")
},{
	"dns_list", "dns_help",
	N_("Enter a list of your DNS servers' IP addresses: 1.2.3.4, 2.3.4.5"),
},{
	"search_list", "dns_help",
	N_("Enter a list of domains where hosts will be searched for (andrew.cmu.edu, res.cmu.edu, helixcode.com)") 
},{
	"ip", "hosts_help",
	N_("Enter the IP address of the host: 1.2.4.9") 
},{
	"alias", "hosts_help",
	N_("Enter one alias per line") 
},{
	NULL
} };

static GHashTable *help_hash;

void
init_hint_entries (void)
{
	int i;
	help_hash = g_hash_table_new (g_str_hash, g_str_equal);

	for (i=0; hint_entry[i][0]; i++)
		g_hash_table_insert (help_hash, (char *)hint_entry[i][0], hint_entry[i]);
}

gint
update_hint (GtkWidget *w, GdkEventFocus *event, gpointer null)
{
	const char *name;
	char **entry;
	GtkWidget *label;

	name = glade_get_widget_name (w);
	if (!name)
		return FALSE;

	entry = g_hash_table_lookup (help_hash, name);

	if (!entry)
		return FALSE;
	
	label = tool_widget_get (entry[1]);
	gtk_label_set_text (GTK_LABEL (label), _(entry[2]));

	return FALSE;
}

void
on_connection_configure_clicked (GtkWidget *w, gpointer null)
{
	g_message ("connection configure");
}

void
on_connection_delete_clicked (GtkWidget *w, gpointer null)
{
	g_message ("connection delete");
}

void
on_connection_add_clicked (GtkWidget *w, gpointer null)
{
	g_message ("connection add");
}

void
on_dns_dhcp_toggled (GtkWidget *w, gpointer null)
{
	char *ws[] = { "domain", "dns_list", "domain_label", "dns_list_label", NULL };
	int i, b;

	b = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	for (i=0; ws[i]; i++)
		gtk_widget_set_sensitive (tool_widget_get (ws[i]), b);

}

void
on_samba_use_toggled (GtkWidget *w, gpointer null)
{
	gtk_widget_set_sensitive (tool_widget_get ("samba_frame"),
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));}
