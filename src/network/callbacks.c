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
#include "gst-hig-dialog.h"

#include "callbacks.h"
#include "transfer.h"
#include "connection.h"
#include "hosts.h"
#include "profile.h"
#include "profiles-table.h"
#include "network-druid.h"


#define d(x) x

extern GstTool *tool;

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
	dns_search_init_gui (tool);
	hosts_init_gui (tool);
	profiles_table_create (tool);
}

gboolean
callbacks_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata)
{
	GtkTreePath *path;
	GtkItemFactory *factory;

	factory = (GtkItemFactory *) gdata;

	if (event->button == 3)
	{
		gtk_widget_grab_focus (GTK_WIDGET (treeview));
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL))
		{
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), path);

			gtk_item_factory_popup (factory, event->x_root, event->y_root,
						event->button, event->time);
		}
		return TRUE;
	}
	return FALSE;
}

void
on_network_notebook_switch_page (GtkWidget *notebook, GtkNotebookPage *page,
				 gint page_num, gpointer user_data)
{
	static guint timeout_id = 0;

	/* if the tool is running remotely, we don't want this function to be run */
	if (tool->remote_config)
		return;

	if (page_num == 0) {
		timeout_id = g_timeout_add (1000, (GSourceFunc) connection_poll_stat, tool);
	} else {
		if (timeout_id) {
			g_source_remove (timeout_id);
			timeout_id = 0;
		}
	}
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
	} else {
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

	if (!success)
		gtk_signal_emit_stop_by_name (GTK_OBJECT (editable), "insert_text");
}

static GHashTable *help_hash;

void
init_editable_filters (GstDialog *dialog)
{
	gint i;
	struct tmp { char *name; EditableFilterRules rule; };
	struct tmp s[] = {
		{ "winsserver",     EF_ALLOW_NONE },
		{ "ip",          EF_ALLOW_IP }, 
		{ "domain",      EF_ALLOW_TEXT },
		{ "network_connection_other_ip_address", EF_ALLOW_IP },
		{ "network_connection_other_ip_mask", EF_ALLOW_IP },
		{ "network_connection_other_gateway", EF_ALLOW_IP },
		{ "network_connection_plip_local_ip", EF_ALLOW_IP },
		{ "network_connection_plip_remote_ip", EF_ALLOW_IP },
		{ "ip_address",          EF_ALLOW_IP },
      		{ "ip_netmask",          EF_ALLOW_IP },
		{ "ip_gateway",          EF_ALLOW_IP },
		{ "ppp_dns1",            EF_ALLOW_IP },
		{ "ppp_dns2",            EF_ALLOW_IP },
		{ "ptp_address",         EF_ALLOW_IP },
		{ "ptp_remote_address",  EF_ALLOW_IP },
		{ "dns_server_entry",    EF_ALLOW_IP },
		{ "search_domain_entry", EF_ALLOW_ENTER | EF_ALLOW_TEXT },
		{ NULL,          EF_ALLOW_NONE }
	};

	for (i = 0; s[i].name; i++)
		g_signal_connect (G_OBJECT (gst_dialog_get_widget (dialog, s[i].name)),
				  "insert_text",
				  G_CALLBACK (filter_editable),
				  GINT_TO_POINTER (s[i].rule));
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
	GtkWidget *druid_window = gst_dialog_get_widget (tool->main_dialog, "network_connection_window");
	GnomeDruid *druid = GNOME_DRUID (gst_dialog_get_widget (tool->main_dialog, "network_connection_druid"));

	network_druid_new (druid, druid_window, tool, GST_CONNECTION_UNKNOWN);
	gtk_window_set_transient_for (GTK_WINDOW (druid_window), GTK_WINDOW (tool->main_dialog));
	gtk_widget_show_all (druid_window);
}

void
on_connection_delete_clicked (GtkWidget *w, gpointer null)
{
	GtkWidget *d;
	GstConnection *cxn;
	gint res;

	cxn = connection_list_get_active ();

	d = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
				GTK_DIALOG_MODAL,
				GST_HIG_MESSAGE_QUESTION,
				NULL,
				_("This will disable any network connection with this interface as soon as you press \"apply\""),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				NULL);
	gst_hig_dialog_set_primary_text (GST_HIG_DIALOG (d),
					 _("Are you sure you want to delete connection \"%s\"?"), cxn->dev);

	res = gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);
	
	if (res == GTK_RESPONSE_ACCEPT) {
		connection_default_gw_remove (cxn->dev);
		connection_list_remove (cxn);
		connection_free (cxn);	
		gst_dialog_modify (tool->main_dialog);
	}
}

void
on_connection_configure_clicked (GtkWidget *w, gpointer null)
{
	GstConnection *cxn = connection_list_get_active ();
	
	connection_configure (cxn);
}

void
on_connection_popup_add_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_connection_add_clicked (callback_data, NULL);
}

void
on_connection_popup_configure_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_connection_configure_clicked (callback_data, NULL);
}

void
on_connection_popup_delete_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_connection_delete_clicked (callback_data, NULL);
}

static void
on_connection_activate_clicked (GtkWidget *w, GtkTreePath *path, gpointer null)
{
	GstConnection *cxn;

	cxn = connection_list_get_by_path (path);
	connection_enable (cxn, TRUE);
}

void
on_connection_activate_button_clicked (GtkWidget *widget, gpointer data)
{
	GstConnection *cxn = connection_list_get_active ();
	connection_enable (cxn, TRUE);
}

static void
on_connection_deactivate_clicked (GtkWidget *w, GtkTreePath *path, gpointer null)
{
	GstConnection *cxn;

	cxn = connection_list_get_by_path (path);
	connection_enable (cxn, FALSE);
}

void
on_connection_deactivate_button_clicked (GtkWidget *widget, gpointer data)
{
	GstConnection *cxn = connection_list_get_active ();
	connection_enable (cxn, FALSE);
}

void
on_samba_use_toggled (GtkWidget *w, gpointer null)
{
	gboolean active, wins_active, configured, smb_installed;
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	wins_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gst_dialog_get_widget (tool->main_dialog, "wins_use")));
	configured = (gboolean) g_object_get_data (G_OBJECT (tool), "tool_configured");
	smb_installed = (gboolean) g_object_get_data (G_OBJECT (tool), "smbinstalled");
	
	if (configured && !smb_installed && active) {
		GtkWidget *dialog;
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);

		dialog = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					     GTK_DIALOG_MODAL,
					     GST_HIG_MESSAGE_INFO,
					     _("SMB Support is not running"),
					     _("You don't have SMB support installed. Please install "
					       "SMB support in the system to enable file sharing in Windows networks"),
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					     NULL);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "description_label"), active);
	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "smbdesc"), active);
	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "workgroup_label"), active);
	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "workgroup"), active);
	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "wins_use"), active);

	if ((active) && (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gst_dialog_get_widget (tool->main_dialog, "wins_use")))))
		gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "winsserver"), active);
	else
		gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "winsserver"), FALSE);

	if (smb_installed) {
		gst_xml_element_set_boolean (root, "smbuse", active);
		gst_dialog_modify (tool->main_dialog);
	}
}

void
on_wins_use_toggled (GtkWidget *w, gpointer null)
{
	GtkWidget *smb = gst_dialog_get_widget (tool->main_dialog, "samba_use");
	gboolean wins_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));
	gboolean smb_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (smb));
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);

	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "winsserver"), wins_active && smb_active);

	gst_xml_element_set_boolean (root, "winsuse", wins_active & smb_active);
	gst_dialog_modify (tool->main_dialog);
}

gboolean
callbacks_check_hostname_hook (GstDialog *dialog, gpointer data)
{
	gchar *hostname_old = NULL;
	const gchar *hostname_new;
	xmlNode *root, *node;
	GtkWidget *entry;
	gint res;

	root = gst_xml_doc_get_root (dialog->tool->config);
	node = gst_xml_element_find_first (root, "hostname");

	hostname_old = gst_xml_element_get_content (node);

	entry = gst_dialog_get_widget (dialog, "hostname");
	hostname_new = gtk_entry_get_text (GTK_ENTRY (entry));

	if (hostname_old && hostname_new && strcmp (hostname_new, hostname_old))
	{
		GtkWidget *message;

		message = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					      GTK_DIALOG_MODAL,
					      GST_HIG_MESSAGE_WARNING,
					      _("The host name has changed"),
					      _("This will prevent you "
						"from launching new applications, and so you will "
						"have to log in again. Continue anyway?"),
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      _("Change _Host name"), GTK_RESPONSE_ACCEPT,
					      NULL);

		res = gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);

		switch (res) {
		case GTK_RESPONSE_ACCEPT:
			g_free (hostname_old);
			return TRUE;
		case GTK_RESPONSE_CANCEL:
		default:
			gtk_entry_set_text (GTK_ENTRY (entry), hostname_old);
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
	
	has_dialer = (gboolean) g_object_get_data (G_OBJECT (tool), "dialinstalled");
	
	if (!has_dialer)
	{
		GtkWidget *message;

		message = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					      GTK_DIALOG_MODAL,
					      GST_HIG_MESSAGE_WARNING,
					      _("Wvdial command not found"),
					      _("wvdial could not be found on your system. "
						"You need to install wvdial, or the PPP (modem) "
						"connections will not activate"),
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
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
	if (!has_dialer) {
		GtkWidget *message;
		gint res;

		message = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					      GTK_DIALOG_MODAL,
					      GST_HIG_MESSAGE_WARNING,
					      _("Wvdial command not found"),
					      _("wvdial could not be found on your system. "
						"You need to install wvdial, or the PPP (modem) "
						"connections will not activate. Continue anyway?"),
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      _("_Continue"), GTK_RESPONSE_ACCEPT,
					      NULL);
		res = gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);

		if (res == GTK_RESPONSE_ACCEPT)
			return TRUE;
		else
			return FALSE;
	}

	return TRUE;
}

static gboolean
callbacks_disabled_gatewaydev_warn (GstTool *tool, GstConnection *cxn)
{
	GtkWidget *message;
	gint res;

	message = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
				      GTK_DIALOG_MODAL,
				      GST_HIG_MESSAGE_WARNING,
				      _("The default gateway device is not activated"),
				      _("This will prevent you from connecting to the Internet. "
					"Continue anyway?"),
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      _("_Continue"), GTK_RESPONSE_ACCEPT,
				      NULL);
	res = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);
	
	switch (res) {
	case GTK_RESPONSE_ACCEPT:
		connection_default_gw_fix (cxn, GST_CONNECTION_ERROR_ENABLED);
		return TRUE;
	case GTK_RESPONSE_CANCEL:
	default:
		return FALSE;
		break;
	}
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
			if (callbacks_disabled_gatewaydev_warn (tool, cxn))
				continue;
			else
				return FALSE;
		case GST_CONNECTION_ERROR_PPP:
			connection_default_gw_fix (cxn, error);
			continue;
		case GST_CONNECTION_ERROR_STATIC:
		{
			GtkWidget *dialog;

			dialog = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
						     GTK_DIALOG_MODAL,
						     GST_HIG_MESSAGE_ERROR,
						     _("The default gateway device is missing gateway information"),
						     _("Please provide this information to "
						       "proceed, or choose another default gateway device"),
						     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						     NULL);
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
		GtkWidget *message;

		message = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					      GTK_DIALOG_MODAL,
					      GST_HIG_MESSAGE_WARNING,
					      _("The program redhat-config-network-cmd could not be found"),
					      _("This could render missing connections "
						"under the connections tab. Please install the "
						"redhat-config-network rpm package to avoid this"),
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
	}

	return TRUE;
}

/* Connection dialog callbacks */

void
on_connection_toggled (GtkWidget *w, gchar *path_str, gpointer data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW (data);
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	GstConnection *cxn;
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	gchar *button_text    = NULL;
	GtkWidget *dialog;

	gtk_tree_model_get_iter (model, &iter, path);
	cxn = connection_list_get_by_path (path);

	if (!cxn->enabled) {
		primary_text   = g_strdup_printf (_("Do you want to enable interface \"%s\"?"), cxn->dev);
		secondary_text = g_strdup (_("This will enable network access through this interface"));
		button_text    = g_strdup (_("_Enable"));
	} else {
		primary_text   = g_strdup_printf (_("Do you want to disable interface \"%s\"?"), cxn->dev);
		secondary_text = g_strdup (_("This will disable network access through this interface"));
		button_text    = g_strdup (_("_Disable"));
	}

	dialog = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
				     GTK_DIALOG_MODAL,
				     GST_HIG_MESSAGE_QUESTION,
				     primary_text,
				     secondary_text,
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     button_text, GTK_RESPONSE_ACCEPT,
				     NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		if (!cxn->enabled)
			on_connection_activate_clicked (GTK_WIDGET (treeview), path, NULL);
		else
			on_connection_deactivate_clicked (GTK_WIDGET (treeview), path,  NULL);
	}

	gtk_widget_destroy (dialog);
	g_free (primary_text);
	g_free (secondary_text);
	g_free (button_text);
	gtk_tree_path_free (path);
}

void
on_connection_ok_clicked (GtkWidget *w, gpointer data)
{
	GtkWidget *window = gst_dialog_get_widget (tool->main_dialog, "connection_config_dialog");
	GstConnection *cxn = g_object_get_data (G_OBJECT (window), "connection_data");
	gboolean standalone = (g_object_get_data (G_OBJECT (window), "standalone") != NULL);
	gboolean add_to_list = !standalone;

	if ((cxn->modified) && (connection_config_save (cxn, window, FALSE))) {
		cxn->creating = FALSE;
		gst_dialog_modify (tool->main_dialog);

		if (cxn->gateway && *cxn->gateway)
			connection_default_gw_add (cxn);
		else
			connection_default_gw_remove (cxn->dev);

		if (standalone) {
			gtk_widget_set_sensitive (window, FALSE);
			connection_save_to_node (cxn, gst_xml_doc_get_root (tool->config));
			gtk_signal_emit_by_name (GTK_OBJECT (tool->main_dialog), "apply", tool);
			gtk_main_quit ();
		}

		gtk_widget_hide (window);
	} else if (!cxn->modified) {
		gtk_widget_hide (window);
	}
}

gboolean
on_connection_delete_event (GtkWidget *window, GdkEvent *event, gpointer data)
{
	on_connection_cancel_clicked (window, NULL);
	return TRUE;
}

void
on_connection_cancel_clicked (GtkWidget *w, gpointer data)
{
	GtkWidget *window = gst_dialog_get_widget (tool->main_dialog, "connection_config_dialog");

	if (g_object_get_data (G_OBJECT (window), "standalone") != NULL)
		gtk_main_quit ();

	gtk_widget_hide (window);
}

void
on_connection_modified (GtkWidget *w, gpointer data)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "connection_config_dialog");
	GstConnection *cxn = g_object_get_data (G_OBJECT (dialog), "connection_data");

	connection_set_modified (cxn, TRUE);
}

void
on_ppp_autodetect_modem_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget *autodetect_button = gst_dialog_get_widget (tool->main_dialog, "ppp_autodetect_modem");
	GtkWidget *devices_combo = gst_dialog_get_widget (tool->main_dialog, "ppp_serial_port_g");
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "connection_config_dialog");
	GtkWidget *w;
	gchar *dev;
	GdkCursor *cursor;

	/* give some feedback to let know the user that the tool is busy */
	gtk_widget_set_sensitive (autodetect_button, FALSE);
	gtk_widget_set_sensitive (devices_combo, FALSE);

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (GTK_WIDGET (dialog)->window, cursor);
	gdk_cursor_unref (cursor);

	dev = connection_autodetect_modem ();

	if ((!dev) || (strcmp (dev, "") == 0)) {
		w = gst_hig_dialog_new (GTK_WINDOW (dialog),
					GTK_DIALOG_MODAL,
					GST_HIG_MESSAGE_WARNING,
					_("Could not autodetect modem device"),
					_("Check that the device is not busy and "
					  "that is correctly attached to the computer"),
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
		gtk_dialog_run (GTK_DIALOG (w));
		gtk_widget_destroy (w);
	} else {
		w = gst_dialog_get_widget (tool->main_dialog, "ppp_serial_port");
		gtk_entry_set_text (GTK_ENTRY (w), dev);
	}

	if (dev)
		g_free (dev);

	/* remove the user feedback */
	gtk_widget_set_sensitive (autodetect_button, TRUE);
	gtk_widget_set_sensitive (devices_combo, TRUE);
	
	gdk_window_set_cursor (GTK_WIDGET (dialog)->window, NULL);
}

void
on_ppp_update_dns_toggled (GtkWidget *w, gpointer data)
{
	gboolean active;
	GtkWidget *ppp_update_dns = gst_dialog_get_widget (tool->main_dialog, "ppp_update_dns");
	GtkWidget *ppp_dns1_label = gst_dialog_get_widget (tool->main_dialog, "ppp_dns1_label");
	GtkWidget *ppp_dns2_label = gst_dialog_get_widget (tool->main_dialog, "ppp_dns2_label");
	GtkWidget *ppp_dns1 = gst_dialog_get_widget (tool->main_dialog, "ppp_dns1");
	GtkWidget *ppp_dns2 = gst_dialog_get_widget (tool->main_dialog, "ppp_dns2");
	
	active = GTK_TOGGLE_BUTTON (ppp_update_dns)->active;

	gtk_widget_set_sensitive (ppp_dns1_label, active);
	gtk_widget_set_sensitive (ppp_dns2_label, active);	
	gtk_widget_set_sensitive (ppp_dns1, active);
	gtk_widget_set_sensitive (ppp_dns2, active);
}

gboolean
on_ip_address_focus_out (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	GtkWidget *netmask_widget = gst_dialog_get_widget (tool->main_dialog, "ip_netmask");

	connection_check_netmask_gui (widget, netmask_widget);

        return FALSE;
}

gchar*
on_volume_format_value (GtkWidget *widget, gdouble value, gpointer data)
{
	gchar *string;
	
	if (value == 0)
		string = g_strdup_printf (_("Silent"));
	else if (value == 1)
		string = g_strdup_printf (_("Low"));
	else if (value == 2)
		string = g_strdup_printf (_("Medium"));
	else if (value == 3)
		string = g_strdup_printf (_("Loud"));

	return string;
}

/* Dns tab callbacks */
void 
on_dns_search_add_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget *treeview;
	GtkWidget *entry;

	if (strcmp (gtk_widget_get_name (GTK_WIDGET (button)),
		    "dns_server_add_button") == 0) {
		treeview = gst_dialog_get_widget (tool->main_dialog,
					       "dns_list");
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "dns_server_entry");
	}
	else if (strcmp (gtk_widget_get_name (GTK_WIDGET (button)),
			 "search_domain_add_button") == 0) {
		treeview = gst_dialog_get_widget (tool->main_dialog,
					       "search_list");
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "search_domain_entry");
	}

	dns_search_list_append (treeview, gtk_entry_get_text (
					GTK_ENTRY (entry)));
}

void 
on_dns_search_del_button_clicked (GtkWidget *widget, gpointer gdata)
{
	GtkWidget *treeview;
	GtkWidget *entry;

	if (strcmp (gtk_widget_get_name (GTK_WIDGET (widget)),
		    "dns_server_del_button") == 0) {
		treeview = gst_dialog_get_widget (tool->main_dialog,
						  "dns_list");
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "dns_server_entry");
	}
	else if (strcmp (gtk_widget_get_name (GTK_WIDGET (widget)),
			 "search_domain_del_button") == 0) {
		treeview = gst_dialog_get_widget (tool->main_dialog,
						  "search_list");
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "search_domain_entry");
	}
	else if (strcmp (gtk_widget_get_name (GTK_WIDGET (widget)),
			 "dns_list") == 0) {
		treeview = widget;
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "dns_server_entry");
	}
	else if (strcmp (gtk_widget_get_name (GTK_WIDGET (widget)),
			 "search_list") == 0) {
		treeview = widget;
		entry = gst_dialog_get_widget (tool->main_dialog,
					       "search_domain_entry");
	}

	dns_search_list_remove (treeview, entry);
}

void
on_dns_search_entry_changed (GtkWidget *widget, gpointer gdata)
{
	GtkWidget *list;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gboolean valid;
	gchar const *text;
	gchar *buf;

	list = (GtkWidget *) gdata;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	text = gtk_entry_get_text (GTK_ENTRY (widget));
	
	if ((!gst_dns_search_is_in_list (list, text)) &&
	    (strlen (text) > 0))
		gtk_tree_selection_unselect_all (selection);
	else {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
		valid = gtk_tree_model_get_iter_first (model, &iter);

		while (valid) {
			gtk_tree_model_get (model, &iter, 0, &buf, -1);

			if (strcmp (buf, text) == 0) {
				g_free (buf);
				gtk_tree_selection_select_iter (selection, &iter);
				break;
			}

			g_free (buf);
			valid = gtk_tree_model_iter_next (model, &iter);
		}
	}

	gst_dns_search_update_sensitivity (list);
	gtk_widget_show_all (list);
}

void
on_dns_search_popup_del_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_dns_search_del_button_clicked (callback_data, NULL);
}

/* Hosts tab callbacks */

static void
gst_hosts_unselect_all (void)
{
	GtkTreeSelection *select;
	GtkTreeModel     *model;
	GstStatichostUI  *ui;
	gboolean          valid;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));

	gtk_tree_selection_unselect_all (select);
	gst_ui_text_view_clear (GTK_TEXT_VIEW (ui->alias));
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

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, STATICHOST_LIST_COL_IP, &buf, -1);

		if (!strcmp (buf, ip_str)) {
			gtk_tree_selection_select_iter (select, &iter);

			g_free (buf);

			return;
		}

		g_free (buf);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}	

static void
gst_hosts_clear_entries (void)
{
	GstStatichostUI *ui;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);

	gtk_editable_delete_text (GTK_EDITABLE (ui->ip), 0, -1);
	gst_ui_text_view_clear (GTK_TEXT_VIEW (ui->alias));
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

	gst_hosts_update_sensitivity ();

	/* Get the texts */
	ip_str = gtk_editable_get_chars (ip,  0, -1);
	if (gst_hosts_ip_is_in_list (ip_str))
		gst_hosts_select_row (ip_str);
}

void
on_hosts_alias_changed (GtkTextBuffer *w, gpointer not_used)
{
	const gchar *ip_str;
	char *s;
	GstStatichostUI *ui;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GtkTreeSelection *select;

	if (!gst_tool_get_access (tool))
		return;

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));
	ip_str = gtk_editable_get_chars (GTK_EDITABLE (ui->ip), 0, -1);

	if (gst_hosts_ip_is_in_list (ip_str)) {
		select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));
		if (gtk_tree_selection_get_selected (select, &model, &iter)) {
			s = fixup_text_list (ui->alias);
			
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    STATICHOST_LIST_COL_IP, gtk_editable_get_chars (GTK_EDITABLE (ui->ip), 0, -1),
					    STATICHOST_LIST_COL_ALIAS, s,
					    -1);

			gst_dialog_modify (tool->main_dialog);
			g_free (s);


		}
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
	gchar *ip, *alias;
	GstStatichostUI *ui;

	g_return_if_fail (gst_tool_get_access (tool));

	ui = (GstStatichostUI *)g_object_get_data (G_OBJECT (tool), STATICHOST_UI_STRING);

	if (!hosts_list_get_selected (&ip, &alias))
		return;

	hosts_list_remove (tool, ip);
	gst_dialog_modify (tool->main_dialog);

	gst_hosts_clear_entries ();
	gst_hosts_unselect_all ();
}

void
on_hosts_popup_del_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_hosts_delete_clicked (callback_data, NULL);
}

/* Network druid callbacks */
gboolean
on_network_druid_hide (GtkWidget *w, gpointer data)
{
	GtkWidget *window = gst_dialog_get_widget (tool->main_dialog, "network_connection_window");
	GnomeDruid *druid = GNOME_DRUID (gst_dialog_get_widget (tool->main_dialog, "network_connection_druid"));

	if (g_object_get_data (G_OBJECT (druid), "standalone") == NULL) {
		gtk_widget_hide (window);
		network_druid_clear (druid, TRUE);
	} else {
		gtk_main_quit ();
	}

	return TRUE;
}

gboolean
on_network_druid_page_back (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{
	return network_druid_set_page_back (druid);
}

gboolean
on_network_druid_page_next (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{
	return network_druid_set_page_next (druid);
}

void
on_network_druid_page_prepare (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	gchar *next_default_focus[] = {
		NULL,
		"connection_type_modem_option",
		"network_connection_wireless_device_entry",
		"network_connection_other_config_type",
		"network_connection_plip_local_ip",
		"network_connection_ppp_phone",
		"network_connection_ppp_login",
		"network_connection_activate",
		NULL
	};

	if (druid_data == NULL)
		return;

	g_signal_stop_emission_by_name (druid_page, "prepare");
	network_druid_check_page (druid, druid_data->current_page);

	if (next_default_focus[druid_data->current_page]) {
		gtk_widget_grab_focus (gst_dialog_get_widget (druid_data->tool->main_dialog,
							      next_default_focus [druid_data->current_page]));
	}

	network_druid_set_window_title (druid);
}

void
on_network_druid_entry_changed (GtkWidget *widget, gpointer data)
{
	GnomeDruid *druid = GNOME_DRUID (gst_dialog_get_widget (tool->main_dialog,
								"network_connection_druid"));
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");

	if (druid_data)
		network_druid_check_page (druid, druid_data->current_page);
}

void
on_network_druid_finish (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{
	GstConnection *cxn = network_druid_get_connection_data (druid);
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	GtkWidget *window = gst_dialog_get_widget (tool->main_dialog, "network_connection_window");

	g_return_if_fail (cxn != NULL);

	if (g_object_get_data (G_OBJECT (druid), "standalone") == NULL) {
		connection_save_to_node (cxn, root);
		connection_add_to_list (cxn);
		connection_list_select_connection (cxn);

		if (cxn->enabled)
			connection_apply_and_activate (tool, cxn);
		else
			gst_dialog_modify (tool->main_dialog);

		network_druid_clear (druid, FALSE);
		gtk_widget_hide (window);
	} else {
		connection_save_to_node (cxn, root);
		gst_dialog_modify (tool->main_dialog);

		connection_apply_and_activate (tool, cxn);
		gtk_main_quit ();
	}
}

void
on_network_druid_config_type_changed (GtkWidget *option_menu, gpointer data)
{
	GnomeDruid *druid = GNOME_DRUID (gst_dialog_get_widget (tool->main_dialog,
								"network_connection_druid"));
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	gboolean enable;
	GtkWidget *widget;
	gint i;
	gchar *widgets [] = {
		"network_connection_other_ip_address",
		"network_connection_other_ip_mask",
		"network_connection_other_gateway",
		"network_connection_other_ip_address_label",
		"network_connection_other_ip_mask_label",
		"network_connection_other_gateway_label",
		NULL
	};

	if (gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu)) == IP_MANUAL)
		enable = TRUE;
	else
		enable = FALSE;

	for (i = 0; widgets[i] != NULL; i++) {
		widget = gst_dialog_get_widget (tool->main_dialog, widgets[i]);
		gtk_widget_set_sensitive (widget, enable);
	}

	if (druid_data)
		network_druid_check_page (druid, druid_data->current_page);
}

gboolean
on_network_druid_ip_address_focus_out (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	GnomeDruid *druid = GNOME_DRUID (gst_dialog_get_widget (tool->main_dialog,
								"network_connection_druid"));
	GtkWidget *mask_widget = gst_dialog_get_widget (tool->main_dialog,
							"network_connection_other_ip_mask");
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");

	connection_check_netmask_gui (widget, mask_widget);

        return FALSE;
}


/* Network profiles callbacks */
void
on_network_profiles_button_clicked (GtkWidget *widget, gpointer data)
{
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	GtkWidget *profiles_dialog = gst_dialog_get_widget (tool->main_dialog,
							    "network_profiles_dialog");
	
	gtk_dialog_run (GTK_DIALOG (profiles_dialog));
	gtk_widget_hide (profiles_dialog);

	profile_populate_option_menu (tool, root);
}

void
on_network_profile_new_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget *profile_new = gst_dialog_get_widget (tool->main_dialog,
							"network_profile_new_dialog");
	GtkWidget *profile_name = gst_dialog_get_widget (tool->main_dialog,
							 "profile_name");
	GtkWidget *profile_description = gst_dialog_get_widget (tool->main_dialog,
								"profile_description");
	const gchar *name, *description;
	gint response;
	
	response = gtk_dialog_run (GTK_DIALOG (profile_new));
	gtk_widget_hide (profile_new);

	if (response == GTK_RESPONSE_OK) {
		name = gtk_entry_get_text (GTK_ENTRY (profile_name));
		description = gtk_entry_get_text (GTK_ENTRY (profile_description));

		if (strcmp (name, "") != 0) {
			profile_save_current (name, description, tool);
			profiles_table_update_content (tool);

			gst_dialog_modify (tool->main_dialog);
		}
	}

	gtk_entry_set_text (GTK_ENTRY (profile_name), "");
	gtk_entry_set_text (GTK_ENTRY (profile_description), "");
}

void
on_network_profile_table_selection_changed (GtkWidget *widget, gpointer data)
{
	GtkWidget *delete_button = gst_dialog_get_widget (tool->main_dialog,
							  "network_profile_delete");

	gtk_widget_set_sensitive (delete_button, TRUE);
}

static void
on_network_profile_delete_clicked_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	xmlNodePtr node;
	
	gtk_tree_model_get (model, iter, PROFILES_TABLE_COL_POINTER, &node, -1);

	if (profile_delete (node, tool) == TRUE)
		* (gboolean *) data = TRUE;
}

void
on_network_profile_delete_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget *profile_table = gst_dialog_get_widget (tool->main_dialog,
							  "network_profiles_table");
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (profile_table));
	gboolean deleted;

	gtk_tree_selection_selected_foreach (selection,
					     on_network_profile_delete_clicked_foreach,
					     &deleted);

	if (deleted) {
		gst_dialog_modify (tool->main_dialog);
		profiles_table_update_content (tool);
	}

	gtk_tree_selection_unselect_all (selection);
}

void
on_network_profile_option_selected (GtkWidget *widget, gpointer data)
{
	GtkWidget *connection_table = gst_dialog_get_widget (tool->main_dialog, "connection_list");
	GtkTreeModel *connection_model = gtk_tree_view_get_model (GTK_TREE_VIEW (connection_table));
	GtkWidget *statichost_table = gst_dialog_get_widget (tool->main_dialog, "statichost_list");
	GtkTreeModel *statichost_model = gtk_tree_view_get_model (GTK_TREE_VIEW (statichost_table));
	GtkWidget *dns_table = gst_dialog_get_widget (tool->main_dialog, "dns_list");
	GtkTreeModel *dns_model = gtk_tree_view_get_model (GTK_TREE_VIEW (dns_table));
	GtkWidget *search_table = gst_dialog_get_widget (tool->main_dialog, "search_list");
	GtkTreeModel *search_model = gtk_tree_view_get_model (GTK_TREE_VIEW (search_table));
	xmlNodePtr profile = (xmlNodePtr) data;
	
	profile_set_active (profile, tool);

	gtk_list_store_clear (GTK_LIST_STORE (connection_model));
	gtk_list_store_clear (GTK_LIST_STORE (statichost_model));
	gtk_list_store_clear (GTK_LIST_STORE (dns_model));
	gtk_list_store_clear (GTK_LIST_STORE (search_model));
	
	transfer_profile_to_gui (tool, NULL);
}

void
on_drag_data_get (GtkTreeView *treeview, GdkDragContext *context, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *ip_address;

	gtk_tree_view_get_cursor (treeview, &path, NULL);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &ip_address, -1);
	
	gtk_selection_data_set (data,
				gdk_atom_intern ("x/dns-data", FALSE),
				8,
				ip_address,
				strlen (ip_address) + 1);
}

void
on_drag_data_received (GtkTreeView *treeview,
		       GdkDragContext *context,
		       gint x,
		       gint y,
		       GtkSelectionData *data,
		       guint info,
		       guint time)
{
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreePath *dest_path;
	GtkTreeViewDropPosition pos;
	GtkTreeIter dest_iter, iter;

	if (data->data == NULL || data->length == -1) {
		gtk_drag_finish (context, FALSE, FALSE, GDK_CURRENT_TIME);
		return;
	}

	if (gtk_tree_view_get_dest_row_at_pos (treeview, x, y, &dest_path, &pos)) {
		if (!gtk_tree_model_get_iter (model, &dest_iter, dest_path)) {
			gtk_drag_finish (context, FALSE, FALSE, GDK_CURRENT_TIME);
			return;
		}

		if (pos == GTK_TREE_VIEW_DROP_BEFORE)
			gtk_list_store_insert_before (GTK_LIST_STORE (model), &iter, &dest_iter);
		else
			gtk_list_store_insert_after (GTK_LIST_STORE (model), &iter, &dest_iter);
	} else {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	}

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, data->data, -1);
	gtk_drag_finish (context, TRUE, TRUE, GDK_CURRENT_TIME);

	gst_dialog_modify (tool->main_dialog);
}
