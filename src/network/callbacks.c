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

#include "xst.h"

#include "callbacks.h"
#include "transfer.h"
#include "connection.h"

#define d(x) x

/*#define POLL_HACK*/

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

void 
on_network_admin_show (GtkWidget *w, gpointer user_data)
{
	GtkCList *list;
	XstTool *tool;

	tool = user_data;
	connection_init_gui (tool);

	list = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "statichost_list"));
	gtk_clist_set_column_auto_resize (list, 0, TRUE);
	gtk_clist_set_column_auto_resize (list, 1, TRUE);
	
	list = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "connection_list"));
	gtk_clist_set_column_auto_resize (list, 0, TRUE);
	gtk_clist_set_column_auto_resize (list, 1, TRUE);
	gtk_clist_set_column_auto_resize (list, 2, TRUE);
}

#ifdef POLL_HACK
static void
poll_connections_cb (XstDirectiveEntry *entry)
{
	xmlDoc     *xml;
	xmlNodePtr  root, iface;
	gboolean    active;
	gchar      *dev;
	gint        i;

	GtkWidget *clist;
	XstConnection *cxn;

	xml = xst_tool_run_get_directive (entry->tool, entry->report_sign, entry->directive, NULL);
	clist = xst_dialog_get_widget (entry->tool->main_dialog, "connection_list");
	
	root    = xst_xml_doc_get_root (xml);
	for (iface = xst_xml_element_find_first (root, "interface"); iface;
	     iface = xst_xml_element_find_next (iface, "interface")) {

		active = xst_xml_element_get_boolean (iface, "active");
		dev = xst_xml_get_child_content (iface, "dev");
		g_print ("%s: %s\n", dev, active? "yes": "no");

		for (i = 0; i < GTK_CLIST (clist)->rows; i++) {
			cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i);

			g_return_if_fail (cxn != NULL);
			g_return_if_fail (cxn->dev != NULL);

			if (!strcmp (cxn->dev, dev)) {
				if (cxn->activation == ACTIVATION_NONE) {
					if (active != cxn->enabled) {
						cxn->enabled = active;
						cxn->activation = (active)? ACTIVATION_UP : ACTIVATION_DOWN;
					} else
						continue;
				}
				
				if (cxn->activation == ACTIVATION_UP) {
					if (active) {
						cxn->activation = ACTIVATION_NONE;
						cxn->bulb_state = TRUE;
						connection_set_row_pixtext (clist, i, _("Active"), TRUE);
					} else {
						cxn->bulb_state = cxn->bulb_state? FALSE: TRUE;
						connection_set_row_pixtext (clist, i, _("Activating"),
									    cxn->bulb_state);
					}
					
					continue;
				}
				
				if (cxn->activation == ACTIVATION_DOWN) {
					if (!active) {
						cxn->activation = ACTIVATION_NONE;
						cxn->bulb_state = FALSE;
						connection_set_row_pixtext (clist, i, _("Inactive"), FALSE);
					} else {
						cxn->bulb_state = cxn->bulb_state? FALSE: TRUE;
						connection_set_row_pixtext (clist, i, _("Deactivating"),
									    cxn->bulb_state);
					}
					continue;
				}
			}
		}

		g_free (dev);
	}

	xst_xml_doc_destroy (xml);
}

static gint
poll_connections (gpointer data)
{
	XstTool *tool = data;
	
	g_print ("-\n");

	xst_tool_queue_directive (tool, poll_connections_cb, tool, NULL, NULL, "list_ifaces");
	
	return TRUE;
}
#endif	
	
void
on_network_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
				 gint page_num, gpointer user_data)
{
	GtkWidget *w;
	gchar *entry[] = { "hostname", "connection_list", "domain", "statichost_list" };

	if (xst_tool_get_access (tool) && entry[page_num]) {
		w = xst_dialog_get_widget (tool->main_dialog, entry[page_num]);
		if (w)
			gtk_widget_grab_focus (w);
	}

#ifdef POLL_HACK
	{
		static gboolean first = TRUE;
		/* The connections tab */
		if (page_num == 1 && first) {
			first = FALSE;
			gtk_timeout_add (500, poll_connections, tool);
		}
	}
#endif	
}

static gboolean
is_ip_text_ok  (const gchar *text)
{
	gint i, dst, numdots;
	IpVersion ver;

	ver = IP_UNK;
	dst = numdots = 0;

	for (i = 0; text[i]; i++) {
		/* IPv6 cases */
		if (text[i] >= 'a' && text[i] <= 'f') {
			if (ver == IP_V4)
				return FALSE;
			dst ++;
			if (dst > 2)
				return FALSE;
			ver = IP_V6;
			continue;
		}

		if (text[i] == ':') {
			if (ver == IP_V4)
				return FALSE;
			dst = 0;
			ver = IP_V6;
			continue;
		}

		/* IPv4 cases */
		if (text[i] == '.') {
			if (ver == IP_V6)
				return FALSE;
			if (i == 0 || text[i - 1] == '.')
				return FALSE;
			numdots ++;
			if (numdots > 3)
				return FALSE;
			dst = 0;
			ver = IP_V4;
			continue;
		}

		if ((text[i] >= '0') && (text[i] <= '9')) {
			dst ++;
			if (dst == 3) {
				if (ver == IP_V6)
					return FALSE;
				ver = IP_V4;
				continue;
			}

			if (dst > 3)
				return FALSE;

			continue;
		}

		return FALSE;
	}

	return TRUE;
}

static gboolean
is_char_ok (char c, EditableFilterRules rules)
{
	return isdigit (c) || c == '.' ||
		((rules & EF_ALLOW_ENTER) && c == '\n') ||
		((rules & EF_ALLOW_SPACE) && c == ' ') ||
		((rules & EF_ALLOW_TEXT) && (isalpha (c) || c == '_' || c == '-'));
}

static gchar *
str_insert_text (const gchar *str, const gchar *text, gint length, gint pos)
{
	gchar *buff;
	gint i, len;

	len = strlen (str);
	
	g_assert (pos <= len);

	buff = g_new0 (char, len + length + 1);

	for (i = 0; i < pos; i++)
		buff[i] = str[i];
	for (i = 0; i < length; i++)
		buff[i + pos] = text[i];
	for (i = 0; i < len - pos; i++)
		buff[i + pos + length] = str[i + pos];
	buff[len + length] = 0;

	return buff;
}

void
filter_editable (GtkEditable *editable, const gchar *text, gint length,
		 gint *pos, gpointer data)
{
	int i, l = 0;
	char *s = NULL;
	EditableFilterRules rules = GPOINTER_TO_INT (data);

	if (rules & EF_ALLOW_IP) {
		gboolean success;

		s = str_insert_text (gtk_editable_get_chars (editable, 0, -1), text, length, *pos);
		success = is_ip_text_ok (s);
		g_free (s);
		s = NULL;
		
		if (success)
			goto text_changed_success;
		else
			goto text_changed_fail;
	}
	
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
	return;
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
init_editable_filters (XstDialog *dialog)
{
	gint i;
	struct { char *name; EditableFilterRules rule; }
	s[] = {
		{ "wins_ip",     EF_ALLOW_NONE },
		{ "dns_list",    EF_ALLOW_ENTER },
		{ "search_list", EF_ALLOW_ENTER | EF_ALLOW_TEXT },
		{ "ip",          EF_ALLOW_IP }, 
		{ "alias",       EF_ALLOW_ENTER | EF_ALLOW_TEXT },
		{ "domain",      EF_ALLOW_TEXT },
		{ NULL,          EF_ALLOW_NONE }
	};

	for (i = 0; s[i].name; i++)
		connect_editable_filter (xst_dialog_get_widget (dialog, s[i].name), s[i].rule);
}

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

	g_return_if_fail (cxn != NULL);

/*	if ((cxn->type == XST_CONNECTION_LO) &&
	    (tool->main_dialog->complexity == XST_DIALOG_BASIC))
		connection_actions_set_sensitive (FALSE);
	else
	connection_actions_set_sensitive (TRUE);*/

	if (cxn->type == XST_CONNECTION_LO)
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
	GtkWidget *d, *ppp, *eth, *wvlan, *plip, *irlan, *clist;
	gint res, row;
	XstConnectionType cxn_type;
	
	d = xst_dialog_get_widget (tool->main_dialog, "connection_type_dialog");

	ppp   = xst_dialog_get_widget (tool->main_dialog, "connection_type_ppp");
	eth   = xst_dialog_get_widget (tool->main_dialog, "connection_type_eth");
	wvlan = xst_dialog_get_widget (tool->main_dialog, "connection_type_wvlan");
	plip  = xst_dialog_get_widget (tool->main_dialog, "connection_type_plip");
	irlan = xst_dialog_get_widget (tool->main_dialog, "connection_type_irlan");

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
	else if (GTK_TOGGLE_BUTTON (irlan)->active)
		cxn_type = XST_CONNECTION_IRLAN;
	else
		cxn_type = XST_CONNECTION_UNKNOWN;

	cxn = connection_new_from_type (cxn_type, xst_xml_doc_get_root (tool->config));
	cxn->creating = TRUE;
	connection_save_to_node (cxn, xst_xml_doc_get_root (tool->config));
	connection_configure (cxn);
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	connection_add_to_list (cxn, clist);
	row = gtk_clist_find_row_from_data (GTK_CLIST (clist), cxn);
	gtk_clist_select_row (GTK_CLIST (clist), row, 0);
	scrolled_window_scroll_bottom (xst_dialog_get_widget (tool->main_dialog, "connection_list_sw"));
        /* connection_configure (cxn);*/
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

	connection_default_gw_remove (cxn->dev);
	connection_free (cxn);
	gtk_clist_remove (GTK_CLIST (clist), connection_row_selected);
	xst_dialog_modify (tool->main_dialog);
}

/* in my younger years i would do this function in 1 line */
/* Yeah, now we know the compiler takes care of it. */
void
on_connection_configure_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;
	XstConnection *cxn;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);

	connection_configure (cxn);
}

static void
activate_directive_cb (XstDirectiveEntry *entry)
{
	gchar *file = entry->data;
	
	xst_tool_run_set_directive (entry->tool, entry->in_xml, entry->report_sign, entry->directive,
				    file, "1", NULL);
	g_free (entry->report_sign);
}

void
on_connection_activate_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;
	XstConnection *cxn;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);
	connection_update_row_enabled (cxn, TRUE);

	if (xst_dialog_get_modified (tool->main_dialog)) {
		xst_tool_save (tool);
		xst_dialog_set_modified (tool->main_dialog, FALSE);
	} else {
		gchar *sign, *file;
		
		file = (cxn->file)? cxn->file: cxn->dev;
		sign = g_strdup_printf (_("Activating connection ``%s.''"), cxn->name);
		xst_tool_run_set_directive (tool, NULL, sign, "enable_iface", file, "1", NULL);
		g_free (sign);
/*		xst_tool_queue_directive (tool, activate_directive_cb, file, NULL, sign, "enable_iface");*/
	}

	cxn->activation = ACTIVATION_UP;
}

static void
deactivate_directive_cb (XstDirectiveEntry *entry)
{
	gchar *file = entry->data;
	
	xst_tool_run_set_directive (entry->tool, entry->in_xml, entry->report_sign, entry->directive,
				    file, "0", NULL);
	g_free (entry->report_sign);
}

void
on_connection_deactivate_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *clist;
	XstConnection *cxn;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	cxn = gtk_clist_get_row_data (GTK_CLIST (clist), connection_row_selected);
	connection_update_row_enabled (cxn, FALSE);

	if (xst_dialog_get_modified (tool->main_dialog)) {
		xst_tool_save (tool);
		xst_dialog_set_modified (tool->main_dialog, FALSE);
	} else {
		gchar *sign, *file;
		
		file = (cxn->file)? cxn->file: cxn->dev;
		sign = g_strdup_printf (_("Deactivating connection ``%s.''"), cxn->name);
		xst_tool_run_set_directive (tool, NULL, sign, "enable_iface", file, "0", NULL);
		g_free (sign);
/*		xst_tool_queue_directive (tool, deactivate_directive_cb, file, NULL, sign, "enable_iface");*/
	}

	cxn->activation = ACTIVATION_DOWN;
}

void
on_samba_use_toggled (GtkWidget *w, gpointer null)
{
	gboolean active, configured, smb_installed;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	configured = (gboolean) gtk_object_get_data (GTK_OBJECT (tool), "tool_configured");
	smb_installed = (gboolean) gtk_object_get_data (GTK_OBJECT (tool), "smbinstalled");
	
	if (configured && !smb_installed && active) {
		GtkWidget *dialog;
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
		dialog = gnome_ok_dialog (_("You don't have SMB support installed. Please install SMB support\nin the system to enable windows networking."));
		gtk_window_set_title (GTK_WINDOW (dialog), _("SMB support missing."));
		gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
		return;
	}

	if (smb_installed) {
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "samba_frame", active);
		xst_dialog_modify_cb (w, null);
	} else
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "samba_frame", active);
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
	GtkWidget *entry;
	gint res;

	root = xst_xml_doc_get_root (dialog->tool->config);
	node = xst_xml_element_find_first (root, "hostname");

	hostname_old = xst_xml_element_get_content (node);

	entry = xst_dialog_get_widget (dialog, "hostname");
	hostname_new = gtk_entry_get_text (GTK_ENTRY (entry));

	if (strcmp (hostname_new, hostname_old))
	{
		gchar *text = _("The host name has changed. This will prevent you\n"
				"from launching new applications, and so you will\n"
				"have to log in again.\n\nContinue anyway?");
		GtkWidget *message;
		
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
			g_free (hostname_old);
			return TRUE;
		case 2:
			g_free (hostname_old);
			return FALSE;
		}
	}

	g_free (hostname_old);
	return TRUE;
}

gboolean
callbacks_update_connections_hook (XstDialog *dialog, gpointer data)
{
	GtkWidget *clist;

	clist = xst_dialog_get_widget (dialog, "connection_list");
	connection_update_clist_enabled_apply (clist);

	return TRUE;
}

static gboolean
callbacks_has_dialer (XstTool *tool)
{
	gboolean   has_dialer = FALSE;
	gboolean   need_dialer = FALSE;
	xmlNodePtr root;
	GtkWidget *clist;
	int i;

	root = xst_xml_doc_get_root (tool->config);
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	for (i=0; i < GTK_CLIST (clist)->rows; i++) {
		XstConnection *cxn;

		cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i), root;
		if (cxn && cxn->type == XST_CONNECTION_PPP) {
			need_dialer = TRUE;
			break;
		}
	}

	if (!need_dialer)
		return TRUE;

	has_dialer = (gboolean) gtk_object_get_data (GTK_OBJECT (tool),
						     "dialinstalled");

	return has_dialer;
}

void
callbacks_check_dialer (GtkWindow *window, XstTool *tool)
{
	gboolean has_dialer;
	
	has_dialer = callbacks_has_dialer (tool);
	if (!has_dialer)
	{
		gchar *text = _("wvdial could not be found on your system.\n"
				"You need to install wvdial, or the PPP (modem)\n"
				"connections will not activate.");
		GtkWidget *message;
		
		message = gnome_warning_dialog_parented (text, window);
		gnome_dialog_run_and_close (GNOME_DIALOG (message));
	}
}

gboolean
callbacks_check_dialer_hook (XstDialog *dialog, gpointer data)
{
	XstTool *tool;
	gboolean has_dialer;

	tool = XST_TOOL (data);
	has_dialer = callbacks_has_dialer (tool);
	if (!has_dialer)
	{
		gchar *text = _("wvdial could not be found on your system.\n"
				"You need to install wvdial, or the PPP (modem)\n"
				"connections will not activate.\n\nContinue anyway?");
		gint res;
		GtkWidget *message;
		
		message = gnome_message_box_new (text, GNOME_MESSAGE_BOX_WARNING,
						 GNOME_STOCK_BUTTON_OK,
						 GNOME_STOCK_BUTTON_CANCEL,
						 NULL);
		gnome_dialog_set_parent (GNOME_DIALOG (message), GTK_WINDOW (dialog));
		res = gnome_dialog_run_and_close (GNOME_DIALOG (message));

		switch (res) {
		case 0:
			return TRUE;
		case 1:
			return FALSE;
		}
	}

	return TRUE;
}

static gboolean
callbacks_disabled_gatewaydev_warn (XstTool *tool, XstConnection *cxn, gboolean *ignore_enabled)
{
	gchar *text = _("The default gateway device is not activated. This\n"
			"will prevent you from connecting to the Internet.\n"
			"\nContinue anyway?");
	gint res;
	GtkWidget *message;
	
	message = gnome_message_box_new (text, GNOME_MESSAGE_BOX_WARNING,
					 _("Activate connection"),
					 GNOME_STOCK_BUTTON_OK,
					 GNOME_STOCK_BUTTON_CANCEL,
					 NULL);
	gnome_dialog_set_parent (GNOME_DIALOG (message), GTK_WINDOW (tool->main_dialog));
	res = gnome_dialog_run_and_close (GNOME_DIALOG (message));
	
	switch (res) {
	case 0:
		connection_default_gw_fix (cxn, XST_CONNECTION_ERROR_ENABLED);
		return TRUE;
	case 1:
		*ignore_enabled = TRUE;
		return TRUE;
	case 2:
		return FALSE;
	}

	return TRUE;
}

static gboolean
callbacks_check_manual_gatewaydev (XstTool *tool)
{
	XstConnection *cxn;
	XstConnectionErrorType error;
	gboolean ignore_enabled;

	ignore_enabled = FALSE;
	cxn = connection_default_gw_get_connection (tool);

	while ((error = connection_default_gw_check_manual (cxn, ignore_enabled))
	       != XST_CONNECTION_ERROR_NONE)
	{
		switch (error) {
		case XST_CONNECTION_ERROR_ENABLED:
			if (callbacks_disabled_gatewaydev_warn (tool, cxn, &ignore_enabled))
				continue;
			else
				return FALSE;
		case XST_CONNECTION_ERROR_PPP:
			connection_default_gw_fix (cxn, error);
			continue;
		case XST_CONNECTION_ERROR_STATIC:
		{
			GtkWidget *dialog;
			gchar *txt = _("The default gateway device is missing gateway\n"
				       "information. Please provide this information to\n"
				       "proceed, or choose another default gateway device.\n");
			
			dialog = gnome_error_dialog_parented (txt, GTK_WINDOW (tool->main_dialog));
			gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
			return FALSE;
		}
		break;
		case XST_CONNECTION_ERROR_NONE:
		case XST_CONNECTION_ERROR_OTHER:
		default:
			g_warning ("callbacks_check_manual_gatewaydev: shouldn't be here.");
			return TRUE;
		}
	}

	connection_default_gw_set_manual (tool, cxn);
	
	return TRUE;
}

gboolean
callbacks_check_gateway_hook (XstDialog *dialog, gpointer data)
{
	XstTool *tool;

	tool = XST_TOOL (data);
	if (!gtk_object_get_data (GTK_OBJECT (tool), "gwdevunsup") &&
	    gtk_object_get_data (GTK_OBJECT (tool), "gatewaydev"))
		return callbacks_check_manual_gatewaydev (tool);

	connection_default_gw_set_auto (tool);
	return TRUE;
}

gboolean
callbacks_tool_not_found_hook (XstTool *tool, XstReportLine *rline, gpointer data)
{
	if (! strcmp (rline->argv[0], "redhat-config-network-cmd")) {
		gchar *text = _("The program redhat-config-network-cmd could not\n"
				"be found. This could render missing connections\n"
				"under the connections tab. Please install the\n"
				"redhat-config-network rpm package to avoid this.");
		GtkWidget *message;
		
		message = gnome_message_box_new (text, GNOME_MESSAGE_BOX_WARNING,
						 GNOME_STOCK_BUTTON_OK,
						 NULL);
		gnome_dialog_set_parent (GNOME_DIALOG (message), GTK_WINDOW (tool->main_dialog));
		gnome_dialog_run_and_close (GNOME_DIALOG (message));
	}

	return TRUE;
}
