/* 
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
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 *          Arturo Espinosa <arturo@ximian.com>
 *          Jacob Berkman <jacob@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>

#include "global.h"

#include "callbacks.h"
#include "transfer.h"
#include "connection.h"

#define d(x) x

extern XstTool *tool;

static int statichost_row_selected = -1;
static int connection_row_selected = -1;


static void
scrolled_window_scroll_bottom (GtkWidget *sw)
{
        GtkAdjustment *adj;
        
        while (gtk_events_pending ())
                gtk_main_iteration ();
        
        adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));
        gtk_adjustment_set_value (adj, adj->upper - adj->page_size);
}

static void
statichost_actions_set_sensitive (gboolean state)
{
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "statichost_delete"), state);
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "statichost_update"), state); 

}

static void
connection_actions_set_sensitive (gboolean state)
{
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "connection_delete"), state);
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "connection_configure"), state); 
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
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, access_no[i]), xst_tool_get_access (tool));
	
	/* Those widgets that will be available, even if you don't have the access. */
	for (i = 0; access_yes[i]; i++)
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, access_yes[i]), TRUE);
	
	/* Those widgets you should never have access to, and will be activated later on. */
	for (i = 0; unsensitive[i]; i++)
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, unsensitive[i]), FALSE);
	
	gtk_clist_set_column_auto_resize (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list")), 0, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list")), 1, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "connection_list")), 0, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "connection_list")), 1, TRUE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "connection_list")), 2, TRUE);
}

void
on_network_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
																				gint page_num, gpointer user_data)
{
	gchar *entry[] = { "hostname", "connection_list", "dns_dhcp", "statichost_list" };
	
	if (xst_tool_get_access (tool) && entry[page_num])
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, entry[page_num]));
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

	w = xst_dialog_get_widget (tool->main_dialog, "statichost_enabled");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				      get_clist_checkmark (GTK_CLIST (clist), row, 0));

	pos = 0;
	gtk_clist_get_text (GTK_CLIST (clist), row, 1, &row_data);
	w = xst_dialog_get_widget (tool->main_dialog, "ip");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (w), row_data, strlen (row_data), &pos);

	pos = 0;
	gtk_clist_get_text (GTK_CLIST (clist), row, 2, &row_data);
	row_data = fixdown_text_list (g_strdup (row_data));
	
	w = xst_dialog_get_widget (tool->main_dialog, "alias");
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
	
	w = xst_dialog_get_widget (tool->main_dialog, "ip");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);

	w = xst_dialog_get_widget (tool->main_dialog, "alias");
	gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
}

void
on_statichost_changed (GtkWidget *w, gpointer null)
{
	gboolean enabled;

	enabled = xst_tool_get_access (tool) &&
		gtk_text_get_length (GTK_TEXT (xst_dialog_get_widget (tool->main_dialog, "alias"))) &&
		check_ip_entry (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "ip")), FALSE);
			
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "statichost_add"), enabled);
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "statichost_update"), enabled && 
				  GTK_WIDGET_SENSITIVE (xst_dialog_get_widget (tool->main_dialog, "statichost_delete")));
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

	g_return_if_fail (xst_tool_get_access (tool));

	entry[0] = entry[3] = NULL;
	
	entry[1] = gtk_editable_get_chars (
		GTK_EDITABLE (xst_dialog_get_widget (tool->main_dialog, "ip")), 0, -1);

	entry[2] = fixup_text_list (xst_dialog_get_widget (tool->main_dialog, "alias"));
		
	clist = xst_dialog_get_widget (tool->main_dialog, "statichost_list");

	row = gtk_clist_append (GTK_CLIST (clist), entry);

	w = xst_dialog_get_widget (tool->main_dialog, "statichost_enabled");
	set_clist_checkmark (GTK_CLIST (clist), row, 0,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));

	w = xst_dialog_get_widget (tool->main_dialog, "alias");

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

	g_return_if_fail (xst_tool_get_access (tool));
	g_return_if_fail (statichost_row_selected != -1);

	parent = GTK_WIDGET (tool->main_dialog);
	gtk_clist_get_text (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list")),
			    statichost_row_selected, 2, &name);

	txt = g_strdup_printf (_("Are you sure you want to delete the aliases for %s?"), name);
	dialog = gnome_question_dialog_parented (txt, NULL, NULL, GTK_WINDOW (parent));
	res = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
	g_free (txt);

	if (res) return;
		
	gtk_clist_remove (GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list")), statichost_row_selected);
	xst_dialog_modify (tool->main_dialog);
	statichost_actions_set_sensitive (FALSE);
}

void
on_statichost_update_clicked (GtkWidget *b, gpointer null)
{
	GtkWidget *clist, *w;
	char *s;

	g_return_if_fail (xst_tool_get_access (tool));

	clist = xst_dialog_get_widget (tool->main_dialog, "statichost_list");

	w = xst_dialog_get_widget (tool->main_dialog, "statichost_enabled");
	set_clist_checkmark (GTK_CLIST (clist), statichost_row_selected, 0,
			     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));

	gtk_clist_set_text (GTK_CLIST (clist), statichost_row_selected, 1,
			    gtk_editable_get_chars (
				    GTK_EDITABLE (xst_dialog_get_widget (tool->main_dialog, "ip")), 0, -1));
	
	w = xst_dialog_get_widget (tool->main_dialog, "alias");
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
	N_("The name of this computer")
},{
	"description", "general_help", 
	N_("A short description of your computer")
},{
	"workgroup", "general_help",
	N_("The Windows Networking workgroup for your network")
},{
	"wins_ip", "general_help", 
	N_("The IP address of your WINS server")
},{
	"domain", "dns_help",     
	N_("The DNS domain for your computer")
},{
	"dns_list", "dns_help",
	N_("A list of your DNS servers' IP addresses"),
},{
	"search_list", "dns_help",
	N_("A list of domains where hosts will be searched") 
},{
	"ip", "hosts_help",
	N_("The IP address of this host") 
},{
	"alias", "hosts_help",
	N_("Aliases for this host, one per line") 
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
	
	label = xst_dialog_get_widget (tool->main_dialog, entry[1]);
	gtk_label_set_text (GTK_LABEL (label), _(entry[2]));

	return FALSE;
}

/* glade callbacks for the connection page */
void
on_connection_list_select_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	connection_row_selected = row;
	connection_actions_set_sensitive (TRUE);
}

void
on_connection_list_unselect_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	connection_row_selected = -1;
	connection_actions_set_sensitive (FALSE);
}

/* in my younger years i would do this function in 1 line */
void
on_connection_configure_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;
	Connection *cxn;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);

	connection_configure (cxn);
}

void
on_connection_delete_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist, *d;
	Connection *cxn;
	int res;

	d = gnome_question_dialog_parented (_("Remove this connection?"), NULL, NULL,
					    GTK_WINDOW (tool->main_dialog));

	res = gnome_dialog_run_and_close (GNOME_DIALOG (d));
	if (res)
		return;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);

	connection_free (cxn);
	gtk_clist_remove (GTK_CLIST (clist), connection_row_selected);
}

void
on_connection_add_clicked (GtkWidget *w, gpointer null)
{
	Connection *cxn;
	GtkWidget *d, *ppp, *eth, *wvlan, *plip, *clist;
	gint res, row;
	ConnectionType cxn_type;
	
	d = xst_dialog_get_widget (tool->main_dialog, "connection_type_dialog");

	ppp   = xst_dialog_get_widget (tool->main_dialog, "connection_type_ppp");
	eth   = xst_dialog_get_widget (tool->main_dialog, "connection_type_eth");
	wvlan = xst_dialog_get_widget (tool->main_dialog, "connection_type_wvlan");
	plip  = xst_dialog_get_widget (tool->main_dialog, "connection_type_plip");

	/* ppp is the default for now */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ppp), TRUE);

	res = gnome_dialog_run_and_close (GNOME_DIALOG (d));

	if (res)
		return;

	if (GTK_TOGGLE_BUTTON (ppp)->active)
		cxn_type = CONNECTION_PPP;
	else if (GTK_TOGGLE_BUTTON (eth)->active)
		cxn_type = CONNECTION_ETH;
	else if (GTK_TOGGLE_BUTTON (wvlan)->active)
		cxn_type = CONNECTION_WVLAN;
	else if (GTK_TOGGLE_BUTTON (plip)->active)
		cxn_type = CONNECTION_PLIP;
	else
		cxn_type = CONNECTION_UNKNOWN;

	cxn = connection_new_from_type (cxn_type);
	/* connection_configure (cxn); */
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	row = gtk_clist_find_row_from_data (GTK_CLIST (clist), cxn);
	gtk_clist_select_row (GTK_CLIST (clist), row, 0);
	scrolled_window_scroll_bottom (xst_dialog_get_widget (tool->main_dialog, "connection_list_sw"));
	connection_configure (cxn);
}

void
on_dns_dhcp_toggled (GtkWidget *w, gpointer null)
{
	char *ws[] = { "domain", "dns_list", "domain_label", "dns_list_label", NULL };
	int i, b;

	b = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	for (i=0; ws[i]; i++)
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, ws[i]), b);

}

void
on_samba_use_toggled (GtkWidget *w, gpointer null)
{
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "samba_frame"),
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
}
