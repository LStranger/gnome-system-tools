/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
connection_actions_set_sensitive (gboolean state)
{
	gint i;
	gchar *names[] = {
		"connection_delete",
		"connection_configure",
		"connection_activate",
		"connection_deactivate",
		NULL
	};

	for (i = 0; names[i]; i++)
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, names[i], state);
}

void 
on_network_admin_show (GtkWidget *w, gpointer user_data)
{
	GtkCList *list;

	list = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list"));
	gtk_clist_set_column_auto_resize (list, 0, TRUE);
	gtk_clist_set_column_auto_resize (list, 1, TRUE);
	
	list = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "connection_list"));
	gtk_clist_set_column_auto_resize (list, 0, TRUE);
	gtk_clist_set_column_auto_resize (list, 1, TRUE);
	gtk_clist_set_column_auto_resize (list, 2, TRUE);
}

void
on_network_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
				 gint page_num, gpointer user_data)
{
	gchar *entry[] = { "hostname", "connection_list", "dns_dhcp", "statichost_list" };
	
	if (xst_tool_get_access (tool) && entry[page_num])
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, entry[page_num]));
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
#if 0	
		else
			d(g_print ("rejecting: (%d) `%c'\n", text[i], text[i]));
#endif	

	if (l == length)
		goto text_changed_success;

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
#if 0
	if (! (rules & EF_STATIC_HOST))
		tool_modified_cb ();
#endif
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
	XstConnection *cxn;
	connection_row_selected = row;

	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);

	if (!strcmp (cxn->dev, "lo"))
		connection_actions_set_sensitive (FALSE);
	else
		connection_actions_set_sensitive (TRUE);
}

void
on_connection_list_unselect_row (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer user_data)
{
	connection_row_selected = -1;
	connection_actions_set_sensitive (FALSE);
}

void
on_connection_add_clicked (GtkWidget *w, gpointer null)
{
	XstConnection *cxn;
	GtkWidget *d, *ppp, *eth, *wvlan, *plip, *clist;
	gint res, row;
	XstConnectionType cxn_type;
	
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
		cxn_type = XST_CONNECTION_PPP;
	else if (GTK_TOGGLE_BUTTON (eth)->active)
		cxn_type = XST_CONNECTION_ETH;
	else if (GTK_TOGGLE_BUTTON (wvlan)->active)
		cxn_type = XST_CONNECTION_WVLAN;
	else if (GTK_TOGGLE_BUTTON (plip)->active)
		cxn_type = XST_CONNECTION_PLIP;
	else
		cxn_type = XST_CONNECTION_UNKNOWN;

	cxn = connection_new_from_type_add (cxn_type, xst_xml_doc_get_root (tool->config));
	cxn->creating = TRUE;
	connection_save_to_node (cxn, xst_xml_doc_get_root (tool->config));
	/* connection_configure (cxn); */
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	row = gtk_clist_find_row_from_data (GTK_CLIST (clist), cxn);
	gtk_clist_select_row (GTK_CLIST (clist), row, 0);
	scrolled_window_scroll_bottom (xst_dialog_get_widget (tool->main_dialog, "connection_list_sw"));
	connection_configure (cxn);
}

void
on_connection_delete_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist, *d;
	XstConnection *cxn;
	gint res;
	gchar *txt;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);

	if (cxn->name && *cxn->name)
		txt = g_strdup_printf (_("Remove connection %s: ``%s''?"), cxn->dev, cxn->name);
	else
		txt = g_strdup_printf (_("Remove connection %s?"), cxn->dev);
		
	d = gnome_question_dialog_parented (txt, NULL, NULL,
								 GTK_WINDOW (tool->main_dialog));
	g_free (txt);
	res = gnome_dialog_run_and_close (GNOME_DIALOG (d));
	if (res)
		return;

	connection_free (cxn);
	gtk_clist_remove (GTK_CLIST (clist), connection_row_selected);
	xst_dialog_modify (tool->main_dialog);
}

/* in my younger years i would do this function in 1 line */
void
on_connection_configure_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;
	XstConnection *cxn;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);

	connection_configure (cxn);
}

void
on_connection_activate_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;
	XstConnection *cxn;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);
	connection_update_row_enabled (cxn, TRUE);
}

void
on_connection_deactivate_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;
	XstConnection *cxn;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);
	connection_update_row_enabled (cxn, FALSE);
}

void
on_dns_dhcp_toggled (GtkWidget *w, gpointer null)
{
	char *ws[] = { "domain", "dns_list", "domain_label", "dns_list_label", NULL };
	int i, b;

	b = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	for (i=0; ws[i]; i++)
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, ws [i], b);
}

void
on_samba_use_toggled (GtkWidget *w, gpointer null)
{
	xst_dialog_widget_set_user_sensitive (tool->main_dialog, "samba_frame",
					      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
}

void
on_wins_use_toggled (GtkWidget *w, gpointer null)
{
	xst_dialog_widget_set_user_sensitive (tool->main_dialog, "wins_ip",
					      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
}

gboolean
callbacks_check_hostname_hook (XstDialog *dialog, gpointer data)
{
	gchar *hostname_old;
	gchar *hostname_new;
	xmlNode *root, *node;
	GtkWidget *entry, *message;

	root = xst_xml_doc_get_root (dialog->tool->config);
	node = xst_xml_element_find_first (root, "hostname");

	hostname_old = xst_xml_element_get_content (node);

	entry = xst_dialog_get_widget (dialog, "hostname");
	hostname_new = gtk_entry_get_text (GTK_ENTRY (entry));

	if (strcmp (hostname_new, hostname_old))
	{
		gchar *text = _("The host name has changed. This will prevent you\n"
				"from launching new applications,\n"
				"and so you will have to log in again.\n\nContinue anyway?");
		gint res;
		
		message = gnome_message_box_new (text, GNOME_MESSAGE_BOX_WARNING,
						 _("Don't change host name"),
						 GNOME_STOCK_BUTTON_OK,
						 GNOME_STOCK_BUTTON_CANCEL,
						 NULL);
		gnome_dialog_set_parent (GNOME_DIALOG (message), GTK_WINDOW (dialog));
		res = gnome_dialog_run_and_close (GNOME_DIALOG (message));

		switch (res) {
		case 0:
			gtk_entry_set_text (GTK_ENTRY (entry), hostname_old);
		case 1:
			return TRUE;
		case 2:
			return FALSE;
		}
	}

	return TRUE;
}
