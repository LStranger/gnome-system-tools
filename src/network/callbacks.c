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

#include "gst.h"

#include "callbacks.h"
#include "transfer.h"
#include "connection.h"
#include "hosts.h"
#include "ppp-druid.h"

#define d(x) x

/* Don't Poll */
/* #define POLL_HACK */

extern GstTool *tool;

extern gboolean updating;

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
	GstTool *tool;

	tool = user_data;
	connection_init_gui (tool);
	hosts_init_gui (tool);
}

#ifdef POLL_HACK
static void
poll_connections_cb (GstDirectiveEntry *entry)
{
	xmlDoc     *xml;
	xmlNodePtr  root, iface;
	gboolean    active;
	gchar      *dev;
	gint        i;

	GtkWidget *clist;
	GstConnection *cxn;

	xml = gst_tool_run_get_directive (entry->tool, entry->report_sign, entry->directive, NULL);
	clist = gst_dialog_get_widget (entry->tool->main_dialog, "connection_list");

	root = gst_xml_doc_get_root (xml);
	for (iface = gst_xml_element_find_first (root, "interface"); iface;
	     iface = gst_xml_element_find_next (iface, "interface")) {

		active = gst_xml_element_get_boolean (iface, "active");
		dev = gst_xml_get_child_content (iface, "dev");

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

	gst_xml_doc_destroy (xml);
}

static gint
poll_connections_timeout (gpointer data)
{
	GstTool *tool = data;
	
	gst_tool_queue_directive (tool, poll_connections_cb, tool, NULL, NULL, "list_ifaces");
	
	return TRUE;
}
#endif

void
on_network_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
				 gint page_num, gpointer user_data)
{
	GtkWidget *w;
	gchar *entry[] = { "hostname", "connection_list_sw", "domain", "statichost_list" };

	return;

	if (gst_tool_get_access (tool) && entry[page_num]) {
		w = gst_dialog_get_widget (tool->main_dialog, entry[page_num]);
		if (w)
			gtk_widget_grab_focus (w);
	}

#ifdef POLL_HACK
	{
		static gboolean first = TRUE;
		/* The connections tab */
		if (page_num == 1 && first) {
			first = FALSE;
			gtk_timeout_add (500, poll_connections_timeout, tool);
		}
	}
#endif	
}

/* gets the value of a numeric IP address section */
static gint
get_address_section_value (const gchar *text, gint start, gint len)
{
	gchar *c = (gchar *) &text[start];
	gchar *str = g_strndup (c, len);
	gint value = g_strtod (str, NULL);
	gint i;

        for (i = 0; i < len; i++) {
		if ((str[i] < '0') || (str [i] > '9')) {
			g_free (str);
			return 256;
		}
	}

	g_free (str);
	return value;
}

/* I don't expect this function to be understood, but
 * it works with IPv4, IPv6 and IPv4 embedded in IPv6
 */
static gboolean
is_ip_text_ok (const gchar *text)
{
	gint i, len, numsep, section_val;
	IpVersion ver;
	gchar c;
	gboolean has_double_colon, segment_has_alpha;

	ver = IP_UNK;
	len = 0;
	numsep = 0;
	has_double_colon = FALSE;
	segment_has_alpha = FALSE;

	for (i = 0; text[i]; i++) {
		c = text[i];
		
		if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
			len++;

			if ((ver == IP_V4) ||
			    ((ver == IP_V6) && (i == 1) && (text[0] == ':')) ||
			    ((ver == IP_V6) && (len > 4)))
				return FALSE;

			if (ver == IP_UNK)
				ver = IP_V6;
			segment_has_alpha = TRUE;
		}
		else if (c >='0' && c <='9') {
			len++;
			section_val = get_address_section_value (text, i - len + 1, len);

			if (((ver == IP_V4) && ((len > 3) || (section_val > 255))) ||
			    ((ver == IP_V6) && (i == 1) && (text[0] == ':')) ||
			    ((ver == IP_V6) && (len > 4)) ||
			    ((ver == IP_UNK) && (len > 4)))
				return FALSE;

			if ((ver == IP_UNK) && ((len == 4) || (section_val > 255)))
				ver = IP_V6;
		}
		else if (c == '.') {
			section_val = get_address_section_value (text, i - len, len);
			numsep++;

			if ((len == 0) ||
			    ((ver == IP_V4) && (numsep > 3)) ||
			    ((ver == IP_V6) && ((numsep > 6) || (len > 3) || (len == 0) || (section_val > 255))))
				return FALSE;

			if ((ver == IP_V6) && (len >= 1) && (len <= 3) && (!segment_has_alpha) && (section_val <= 255)) {
				ver = IP_V4;
				numsep = 1;
			}
			if ((ver == IP_UNK) && (section_val <= 255))
				ver = IP_V4;
			len = 0;
		}
		else if (c == ':') {
			numsep++;			

			if ((ver == IP_V4) ||
			    (numsep >= 8) ||
			    ((len == 0) && (has_double_colon)))
				return FALSE;

			if ((numsep > 1) && (len == 0) && (!has_double_colon))
				has_double_colon = TRUE;
			if (ver == IP_UNK)
				ver = IP_V6;
			len = 0;
			segment_has_alpha = FALSE;
		}
		else 
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
	int i = 0;
	gchar *s = NULL;
	gboolean success;
	gboolean string_ok = TRUE;
	EditableFilterRules rules = GPOINTER_TO_INT (data);

	if (rules & EF_ALLOW_IP) {
		s = g_strconcat (gtk_editable_get_chars (editable, 0, -1), text, NULL);
		success = is_ip_text_ok (s);
		g_free (s);
	}
	else {
		while ((i < length) && (string_ok)) {
			if (is_char_ok (text[i], rules))
				i++;
			else
				string_ok = FALSE;
		}

		if (string_ok)
			success = TRUE;
		else
			success = FALSE;
	}

	if (!success) {
		gdk_beep ();
	
		gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");
/*		if (s) {
			gtk_editable_insert_text (editable, s, l, pos);
			}*/
	}
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
init_editable_filters (GstDialog *dialog)
{
	gint i;
	struct tmp { char *name; EditableFilterRules rule; };
	struct tmp s[] = {
		{ "wins_ip",     EF_ALLOW_NONE },
		{ "ip",          EF_ALLOW_IP }, 
		{ "domain",      EF_ALLOW_TEXT },
		{ NULL,          EF_ALLOW_NONE }
	};

	struct tmp s1[] = {
		{ "dns_list",    EF_ALLOW_ENTER },
		{ "search_list", EF_ALLOW_ENTER | EF_ALLOW_TEXT },
		{ "alias",       EF_ALLOW_ENTER | EF_ALLOW_TEXT },
		{ NULL,          EF_ALLOW_NONE }
	};

	for (i = 0; s[i].name; i++)
		connect_editable_filter (gst_dialog_get_widget (dialog, s[i].name), s[i].rule);

#warning FIXME
	return;

	for (i = 0; s1[i].name; i++) {
		GtkTextBuffer *buffer;
		GtkWidget *w = gst_dialog_get_widget (dialog, s1[i].name);

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
		g_signal_connect (G_OBJECT (buffer), "insert_text",
				  G_CALLBACK (filter_editable),
				  GINT_TO_POINTER (s1[i].rule));
	}
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
	
	label = gst_dialog_get_widget (tool->main_dialog, entry[1]);
	gtk_label_set_text (GTK_LABEL (label), _(entry[2]));

	return FALSE;
}

void
on_connection_add_clicked (GtkWidget *w, gpointer null)
{
	GstConnection *cxn;
	GtkWidget *d, *table, *ppp, *eth, *wvlan, *plip, *irlan, *clist;
	gint res, row;
	GstConnectionType cxn_type;

	table = gst_dialog_get_widget (tool->main_dialog, "connection_type_table");

	ppp   = gst_dialog_get_widget (tool->main_dialog, "connection_type_ppp");
	eth   = gst_dialog_get_widget (tool->main_dialog, "connection_type_eth");
	wvlan = gst_dialog_get_widget (tool->main_dialog, "connection_type_wvlan");
	plip  = gst_dialog_get_widget (tool->main_dialog, "connection_type_plip");
	irlan = gst_dialog_get_widget (tool->main_dialog, "connection_type_irlan");

	/* ppp is the default for now */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ppp), TRUE);

	d = gtk_dialog_new_with_buttons (_("New Connection Type"),
					 GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					 NULL);

	gtk_widget_reparent (table, GTK_DIALOG (d)->vbox);

	res = gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_hide (d);

	if (res != GTK_RESPONSE_ACCEPT)
		return;

	if (GTK_TOGGLE_BUTTON (ppp)->active)
		cxn_type = GST_CONNECTION_PPP;
	else if (GTK_TOGGLE_BUTTON (eth)->active)
		cxn_type = GST_CONNECTION_ETH;
	else if (GTK_TOGGLE_BUTTON (wvlan)->active)
		cxn_type = GST_CONNECTION_WVLAN;
	else if (GTK_TOGGLE_BUTTON (plip)->active)
		cxn_type = GST_CONNECTION_PLIP;
	else if (GTK_TOGGLE_BUTTON (irlan)->active)
		cxn_type = GST_CONNECTION_IRLAN;
	else
		cxn_type = GST_CONNECTION_UNKNOWN;

	cxn = connection_new_from_type (cxn_type, gst_xml_doc_get_root (tool->config));
	cxn->creating = TRUE;
	connection_save_to_node (cxn, gst_xml_doc_get_root (tool->config));
	connection_configure (cxn);

	connection_add_to_list (cxn);
	connection_list_select_connection (cxn);

	scrolled_window_scroll_bottom (gst_dialog_get_widget (tool->main_dialog, "connection_list_sw"));
}

void
on_connection_delete_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *d;
	GstConnection *cxn;
	gint res;
	gchar *txt;

	cxn = connection_list_get_active ();

	if (cxn->name && *cxn->name)
		txt = g_strdup_printf (_("Remove connection %s: \"%s\"?"), cxn->dev, cxn->name);
	else
		txt = g_strdup_printf (_("Remove connection %s?"), cxn->dev);

	d = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
				    GTK_DIALOG_MODAL,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_YES_NO,
				    txt);
	g_free (txt);
	res = gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);
	if (res != GTK_RESPONSE_YES)
		return;

	connection_default_gw_remove (cxn->dev);
	connection_list_remove (cxn);
	connection_free (cxn);	
	gst_dialog_modify (tool->main_dialog);
}

void
on_connection_configure_clicked (GtkWidget *w, gpointer null)
{
	connection_configure (connection_list_get_active ());
}

static void
activate_directive_cb (GstDirectiveEntry *entry)
{
	gchar *file = entry->data;

	gst_tool_run_set_directive (entry->tool, entry->in_xml, entry->report_sign, entry->directive,
				    file, "1", NULL);
	g_free (entry->report_sign);
}

void
on_connection_activate_clicked (GtkWidget *w, gpointer null)
{
	GstConnection *cxn;
	gchar *sign, *file;
	gboolean modified = gst_dialog_get_modified (tool->main_dialog);

	cxn = connection_list_get_active ();
	connection_activate (cxn, TRUE);

	file = (cxn->file)? cxn->file: cxn->dev;
	sign = g_strdup_printf (_("Activating connection ``%s.''"), cxn->name);
	gst_tool_queue_directive (tool, activate_directive_cb, file, NULL, NULL, "enable_iface");
	
	gst_dialog_set_modified (tool->main_dialog, modified);
	
/*	if (gst_dialog_get_modified (tool->main_dialog)) {
		gst_tool_save (tool);
		gst_dialog_set_modified (tool->main_dialog, FALSE);
	} else {
		file = (cxn->file)? cxn->file: cxn->dev;
		sign = g_strdup_printf (_("Activating connection ``%s.''"), cxn->name);
		gst_tool_queue_directive (tool, activate_directive_cb, file, NULL, NULL, "enable_iface");
	}*/

	cxn->activation = ACTIVATION_UP;
}

static void
deactivate_directive_cb (GstDirectiveEntry *entry)
{
	gchar *file = entry->data;
	
	gst_tool_run_set_directive (entry->tool, entry->in_xml, entry->report_sign, entry->directive,
				    file, "0", NULL);
	g_free (entry->report_sign);
}

void
on_connection_deactivate_clicked (GtkWidget *w, gpointer null)
{
	GstConnection *cxn;
	gchar *sign, *file;
	gboolean modified = gst_dialog_get_modified (tool->main_dialog);

	cxn = connection_list_get_active ();
	connection_activate (cxn, FALSE);
	
	file = (cxn->file)? cxn->file: cxn->dev;
	sign = g_strdup_printf (_("Deactivating connection ``%s.''"), cxn->name);
	gst_tool_queue_directive (tool, deactivate_directive_cb, file, NULL, NULL, "enable_iface");
	
	gst_dialog_set_modified (tool->main_dialog, modified);
	
/*	cxn = connection_list_get_active ();
	connection_activate (cxn, FALSE);

	if (gst_dialog_get_modified (tool->main_dialog)) {
		gst_tool_save (tool);
		gst_dialog_set_modified (tool->main_dialog, FALSE);
	} else {
		file = (cxn->file)? cxn->file: cxn->dev;
		sign = g_strdup_printf (_("Deactivating connection ``%s.''"), cxn->name);
		gst_tool_queue_directive (tool, deactivate_directive_cb, file, NULL, NULL, "enable_iface");
	}*/

	cxn->activation = ACTIVATION_DOWN;
}

void
on_samba_use_toggled (GtkWidget *w, gpointer null)
{
	gboolean active, configured, smb_installed;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	configured = (gboolean) g_object_get_data (G_OBJECT (tool), "tool_configured");
	smb_installed = (gboolean) g_object_get_data (G_OBJECT (tool), "smbinstalled");
	
	if (configured && !smb_installed && active) {
		GtkWidget *dialog;
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
		dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_OK,
						 _("You don't have SMB support installed. Please install "
						   "SMB support\nin the system to enable windows networking."));

		gtk_window_set_title (GTK_WINDOW (dialog), _("SMB support missing."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	gst_dialog_widget_set_user_sensitive (tool->main_dialog, "description_label", active);
	gst_dialog_widget_set_user_sensitive (tool->main_dialog, "description", active);
	gst_dialog_widget_set_user_sensitive (tool->main_dialog, "workgroup_label", active);
	gst_dialog_widget_set_user_sensitive (tool->main_dialog, "workgroup", active);
	gst_dialog_widget_set_user_sensitive (tool->main_dialog, "wins_use", active);
	gst_dialog_widget_set_user_sensitive (tool->main_dialog, "wins_ip", active);
	if (smb_installed)
		gst_dialog_modify_cb (w, null);
}

void
on_wins_use_toggled (GtkWidget *w, gpointer null)
{
	gst_dialog_widget_set_user_sensitive (tool->main_dialog, "wins_ip",
					      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
}

gboolean
callbacks_check_hostname_hook (GstDialog *dialog, gpointer data)
{
	gchar *hostname_old;
	const gchar *hostname_new;
	xmlNode *root, *node;
	GtkWidget *entry;
	gint res;

	root = gst_xml_doc_get_root (dialog->tool->config);
	node = gst_xml_element_find_first (root, "hostname");

	hostname_old = gst_xml_element_get_content (node);

	entry = gst_dialog_get_widget (dialog, "hostname");
	hostname_new = gtk_entry_get_text (GTK_ENTRY (entry));

	if (strcmp (hostname_new, hostname_old))
	{
		gchar *text = _("The host name has changed. This will prevent you\n"
				"from launching new applications, and so you will\n"
				"have to log in again.\n\nContinue anyway?");
		GtkWidget *message;

		message = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
						  GTK_DIALOG_MODAL,
						  GTK_MESSAGE_WARNING,
						  GTK_BUTTONS_OK_CANCEL,
						  text);

		res = gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);

		switch (res) {
		case GTK_RESPONSE_OK:
			gtk_entry_set_text (GTK_ENTRY (entry), hostname_old);
		case GTK_RESPONSE_CANCEL:
			g_free (hostname_old);
			return TRUE;
		default:
			g_free (hostname_old);
			return FALSE;
		}
	}

	g_free (hostname_old);
	return TRUE;
}

gboolean
callbacks_update_connections_hook (GstDialog *dialog, gpointer data)
{
	connection_list_update ();

	return TRUE;
}

void
callbacks_check_dialer (GtkWindow *window, GstTool *tool)
{
	gboolean has_dialer;
	
	has_dialer = connection_list_has_dialer (tool);
	if (!has_dialer)
	{
		gchar *text = _("wvdial could not be found on your system.\n"
				"You need to install wvdial, or the PPP (modem)\n"
				"connections will not activate.");
		GtkWidget *message;

		gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_OK,
					text);

		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
	}
}

gboolean
callbacks_check_dialer_hook (GstDialog *dialog, gpointer data)
{
	GstTool *tool;
	gboolean has_dialer;

	tool = GST_TOOL (data);
	has_dialer = connection_list_has_dialer (tool);
	if (!has_dialer)
	{
		gchar *text = _("wvdial could not be found on your system.\n"
				"You need to install wvdial, or the PPP (modem)\n"
				"connections will not activate.\n\nContinue anyway?");
		gint res;
		GtkWidget *message;

		message = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
						  GTK_DIALOG_MODAL,
						  GTK_MESSAGE_WARNING,
						  GTK_BUTTONS_OK_CANCEL,
						  text);

		res = gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);

		if (res == GTK_RESPONSE_OK)
			return TRUE;
		else
			return FALSE;
	}

	return TRUE;
}

static gboolean
callbacks_disabled_gatewaydev_warn (GstTool *tool, GstConnection *cxn, gboolean *ignore_enabled)
{
	gchar *text = _("The default gateway device is not activated. This\n"
			"will prevent you from connecting to the Internet.\n"
			"\nContinue anyway?");
	gint res;
	GtkWidget *message;

	message = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_WARNING,
					  GTK_BUTTONS_OK_CANCEL,
					  text);

	res = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);
	
	switch (res) {
	case GTK_RESPONSE_OK:
		connection_default_gw_fix (cxn, GST_CONNECTION_ERROR_ENABLED);
		return TRUE;
	case GTK_RESPONSE_CANCEL:
		*ignore_enabled = TRUE;
		return TRUE;
	default:
		return FALSE;
		break;
	}

	return TRUE;
}

static gboolean
callbacks_check_manual_gatewaydev (GstTool *tool)
{
	GstConnection *cxn;
	GstConnectionErrorType error;
	gboolean ignore_enabled;

	ignore_enabled = FALSE;
	cxn = connection_default_gw_get_connection (tool);

	while ((error = connection_default_gw_check_manual (cxn, ignore_enabled))
	       != GST_CONNECTION_ERROR_NONE)
	{
		switch (error) {
		case GST_CONNECTION_ERROR_ENABLED:
			if (callbacks_disabled_gatewaydev_warn (tool, cxn, &ignore_enabled))
				continue;
			else
				return FALSE;
		case GST_CONNECTION_ERROR_PPP:
			connection_default_gw_fix (cxn, error);
			continue;
		case GST_CONNECTION_ERROR_STATIC:
		{
			GtkWidget *dialog;
			gchar *txt = _("The default gateway device is missing gateway\n"
				       "information. Please provide this information to\n"
				       "proceed, or choose another default gateway device.\n");

			dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
							 GTK_DIALOG_MODAL,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 txt);

			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			return FALSE;
		}
		break;
		case GST_CONNECTION_ERROR_NONE:
		case GST_CONNECTION_ERROR_OTHER:
		default:
			g_warning ("callbacks_check_manual_gatewaydev: shouldn't be here.");
			return TRUE;
		}
	}

	connection_default_gw_set_manual (tool, cxn);
	
	return TRUE;
}

gboolean
callbacks_check_gateway_hook (GstDialog *dialog, gpointer data)
{
	GstTool *tool;

	tool = GST_TOOL (data);
	if (!g_object_get_data (G_OBJECT (tool), "gwdevunsup") &&
	    g_object_get_data (G_OBJECT (tool), "gatewaydev"))
		return callbacks_check_manual_gatewaydev (tool);

	connection_default_gw_set_auto (tool);
	return TRUE;
}

gboolean
callbacks_tool_not_found_hook (GstTool *tool, GstReportLine *rline, gpointer data)
{
	if (! strcmp (rline->argv[0], "redhat-config-network-cmd")) {
		gchar *text = _("The program redhat-config-network-cmd could not\n"
				"be found. This could render missing connections\n"
				"under the connections tab. Please install the\n"
				"redhat-config-network rpm package to avoid this.");
		GtkWidget *message;

		message = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
						  GTK_DIALOG_MODAL,
						  GTK_MESSAGE_WARNING,
						  GTK_BUTTONS_OK,
						  text);

		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
	}

	return TRUE;
}


/* Connection dialog callbacks */

void
on_connection_list_clicked (GtkWidget *w, gpointer data)
{
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *column_list;
	gint ncol;
	GdkPixbuf *stat_icon;
	GstConnection *cxn;
	GtkWidget *dialog;
	gchar *txt;

	cxn = connection_list_get_active ();
	column_list = gtk_tree_view_get_columns (GTK_TREE_VIEW (w));
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (w), &path, &column);
							  
	ncol = g_list_index (column_list, column) + 1;

	if (ncol == CONNECTION_LIST_COL_STAT_PIX)
	{
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (w));
		gtk_tree_model_get_iter (model, &iter, path);
		
		if (!cxn->enabled)
		{
			if (cxn->type != GST_CONNECTION_LO) {
				txt = g_strdup_printf (_("Do you want to enable interface %s?"), cxn->dev);
				dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, txt);
				if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
					on_connection_activate_clicked (w, NULL);

				gtk_widget_destroy (dialog);
				g_free (txt);
			}
		}
		else
		{
			if (cxn->type != GST_CONNECTION_LO) {
				txt = g_strdup_printf (_("Do you want to disable interface %s?"), cxn->dev);
				dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, txt);
				if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
					on_connection_deactivate_clicked (w, NULL);

				gtk_widget_destroy (dialog);
				g_free (txt);
			}
		}
	}
	g_list_free (column_list);
	gtk_tree_path_free (path);
}

void
on_connection_ok_clicked (GtkWidget *w, GstConnection *cxn)
{

	if (cxn->modified) {
		if (connection_config_save (cxn))
			gtk_widget_destroy (cxn->window);
		cxn->creating = FALSE;
		gst_dialog_modify (tool->main_dialog);
	} else
		gtk_widget_destroy (cxn->window);
}

void
on_connection_cancel_clicked (GtkWidget *w, GstConnection *cxn)
{
	gtk_widget_destroy (cxn->window);

	if (cxn->creating) {
		connection_list_remove (cxn);
		connection_free (cxn);
		gst_dialog_modify (tool->main_dialog);
	}
}

void
on_connection_config_dialog_destroy (GtkWidget *w, GstConnection *cxn)
{
	g_object_unref (G_OBJECT (cxn->xml));
	cxn->xml = NULL;

	cxn->window = NULL;
}

gint
on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, GstConnection *cxn)
{
	return FALSE;
}

void
on_connection_modified (GtkWidget *w, GstConnection *cxn)
{
	connection_set_modified (cxn, TRUE);
}

void
on_wvlan_adhoc_toggled (GtkWidget *w, GstConnection *cxn)
{
/* FIXME: implement on_wvlan_adhoc_toggled*/
}

void
on_ppp_autodetect_modem_clicked (GtkWidget *widget, GstConnection *cxn)
{
	xmlNodePtr root;
	gchar *dev;
	xmlDoc *doc = gst_tool_run_get_directive (tool, _("Autodetecting modem device"), "detect_modem", NULL);
	GtkWidget *w;

	g_return_if_fail (doc != NULL);

	root = gst_xml_doc_get_root (doc);
	dev = gst_xml_get_child_content (root, "device");

	if (strcmp (dev, "") == 0) {
		w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
					    _("Could not autodetect modem device, check that this is not busy and that it's correctly attached"));
		gtk_dialog_run (GTK_DIALOG (w));
		gtk_widget_destroy (w);
	} else {
		w = glade_xml_get_widget (cxn->xml, "ppp_serial_port");
		gtk_entry_set_text (GTK_ENTRY (w), dev);
	}
	
	g_free (dev);
}

void
on_ppp_update_dns_toggled (GtkWidget *w, GstConnection *cxn)
{
	gboolean active;
	GtkWidget *ppp_update_dns = glade_xml_get_widget (cxn->xml, "ppp_update_dns");
	GtkWidget *ppp_dns1_label = glade_xml_get_widget (cxn->xml, "ppp_dns1_label");
	GtkWidget *ppp_dns2_label = glade_xml_get_widget (cxn->xml, "ppp_dns2_label");
	GtkWidget *ppp_dns1 = glade_xml_get_widget (cxn->xml, "ppp_dns1");
	GtkWidget *ppp_dns2 = glade_xml_get_widget (cxn->xml, "ppp_dns2");
	
	active = GTK_TOGGLE_BUTTON (ppp_update_dns)->active;

	gtk_widget_set_sensitive (ppp_dns1_label, active);
	gtk_widget_set_sensitive (ppp_dns2_label, active);	
	gtk_widget_set_sensitive (ppp_dns1, active);
	gtk_widget_set_sensitive (ppp_dns2, active);
}

gboolean
on_ip_address_focus_out (GtkWidget *widget, GdkEventFocus *event, GstConnection *cxn)
{
	connection_check_netmask_gui (cxn);

        return FALSE;
}


/* Hosts tab callbacks */

static void
gst_hosts_unselect_all (void)
{
	GtkTreeSelection *select;
	GtkTreeModel     *model;
	GstStatichostUI  *ui;
	gboolean          valid;

	updating = TRUE;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));

	gtk_tree_selection_unselect_all (select);
	gst_ui_text_view_clear (GTK_TEXT_VIEW (ui->alias));

	updating = FALSE;
}

static void
gst_hosts_select_row (const gchar *ip_str)
{
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gboolean          valid;
	gchar            *buf;
	GtkTreeSelection *select;
	GstStatichostUI  *ui;

	if (!ip_str || strlen (ip_str) == 0)
		return;

	updating = TRUE;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP, &buf, -1);

		if (!strcmp (buf, ip_str)) {
			gtk_tree_selection_select_iter (select, &iter);

			updating = FALSE;
			g_free (buf);

			return;
		}

		g_free (buf);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	updating = FALSE;
}	

static void
gst_hosts_clear_entries (void)
{
	GstStatichostUI *ui;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);

	updating = TRUE;
	gtk_editable_delete_text (GTK_EDITABLE (ui->ip), 0, -1);
	gst_ui_text_view_clear (GTK_TEXT_VIEW (ui->alias));
	updating = FALSE;
}

static char *
fixup_text_list (GtkWidget *text)
{
	gchar *s2, *s;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (GTK_IS_TEXT_VIEW (text), NULL);

	s = gst_ui_text_view_get_text (GTK_TEXT_VIEW (text));

	for (s2 = (gchar *) strchr (s, '\n'); s2; s2 = (gchar *) strchr (s2, '\n'))
		*s2 = ' ';

	return s;
}

void
on_hosts_ip_changed (GtkEditable *ip, gpointer not_used)
{
	const gchar *ip_str;

	if (updating)
		return;

	gst_hosts_update_sensitivity ();

	/* Get the texts */
	ip_str = gtk_editable_get_chars (ip,  0, -1);
	if (gst_hosts_ip_is_in_list (ip_str))
		gst_hosts_select_row (ip_str);
}

void
on_hosts_alias_changed (GtkTextBuffer *w, gpointer not_used)
{
	char *s;
	GstStatichostUI *ui;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GtkTreeSelection *select;

	if (!gst_tool_get_access (tool))
		return;

	if (updating)
		return;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));
	if (gtk_tree_selection_get_selected (select, &model, &iter)) {
		s = fixup_text_list (ui->alias);

		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    STATICHOST_LIST_COL_IP, gtk_editable_get_chars (GTK_EDITABLE (ui->ip), 0, -1),
				    STATICHOST_LIST_COL_ALIAS, s,
				    -1);

		g_free (s);
	}
}

void
on_hosts_add_clicked (GtkWidget * button, gpointer user_data)
{
	gchar *entry[STATICHOST_LIST_COL_LAST];
	int row;
	GstStatichostUI *ui;

	g_return_if_fail (gst_tool_get_access (tool));

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);

	entry[STATICHOST_LIST_COL_IP] = gtk_editable_get_chars (GTK_EDITABLE (ui->ip), 0, -1);
	entry[STATICHOST_LIST_COL_ALIAS] = fixup_text_list (ui->alias);

	hosts_list_append (tool, (const gchar**) entry);

	gst_hosts_select_row (entry[STATICHOST_LIST_COL_IP]);
}

void
on_hosts_delete_clicked (GtkWidget * button, gpointer user_data)
{
	gchar *txt, *ip, *alias;
	GtkWidget *dialog;
	gint res;
	GstStatichostUI *ui;

	g_return_if_fail (gst_tool_get_access (tool));

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);

	if (!hosts_list_get_selected (&ip, &alias))
		return;

	txt = g_strdup_printf (_("Are you sure you want to delete the aliases for %s?"), ip);
	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 txt);

	res = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (txt);

	if (res != GTK_RESPONSE_YES)
		return;

	hosts_list_remove (tool, ip);
	gst_dialog_modify (tool->main_dialog);

	gst_hosts_clear_entries ();
	gst_hosts_unselect_all ();
}


/* PPP dialog callbacks */

void
ppp_druid_on_window_delete_event (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_exit (ppp);
}

void
ppp_druid_on_druid_cancel (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_exit (ppp);
}

gboolean
ppp_druid_on_page_next (GtkWidget *w, gpointer arg1, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp->current_page++;

	return FALSE;
}

gboolean
ppp_druid_on_page_back (GtkWidget *w, gpointer arg1, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp->current_page--;

	return FALSE;
}

void
ppp_druid_on_page_prepare (GtkWidget *w, gpointer arg1, gpointer data)
{
	gchar *next_focus[] = {
		NULL, "phone", "login", "profile", NULL
	};
	PppDruid *ppp = (PppDruid *) data;
	gchar *s = g_strdup_printf ("ppp_druid_%s", next_focus [ppp->current_page]);

	ppp_druid_check_page (ppp);

	if (next_focus[ppp->current_page])
		gtk_widget_grab_focus (glade_xml_get_widget (ppp->glade, s));
	else
		gtk_widget_grab_focus (ppp_druid_get_button_next (ppp));
}

void
ppp_druid_on_page_last_finish (GtkWidget *w, gpointer arg1, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_save (ppp);
}

void
ppp_druid_on_entry_changed (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	ppp_druid_check_page (ppp);
}

void
ppp_druid_on_entry_activate (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	if (!ppp->error_state)
		gtk_widget_grab_focus (ppp_druid_get_button_next (ppp));
}

void
ppp_druid_on_login_activate (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	gtk_widget_grab_focus (ppp->passwd);
}

void
ppp_druid_on_passwd_activate (GtkWidget *w, gpointer data)
{
	PppDruid *ppp = (PppDruid *) data;

	gtk_widget_grab_focus (ppp->passwd2);
}
