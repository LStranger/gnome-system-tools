/* 
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
 * Authors: Jacob Berkman <jacob@ximian.com>
 */

#include <config.h>

#include "connection.h"

#include "global.h"

#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* sigh more libglade callbacks */

void on_status_button_clicked (GtkWidget *w, Connection *cxn);
void on_connection_help_clicked (GtkWidget *w, Connection *cxn);
void on_connection_apply_clicked (GtkWidget *w, Connection *cxn);
void on_connection_close_clicked (GtkWidget *w, Connection *cxn);
gint on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, Connection *cxn);
void connection_modified (GtkWidget *w, Connection *cxn);
void on_wvlan_adhoc_toggled (GtkWidget *w, Connection *cxn);

#define W(s) glade_xml_get_widget (cxn->xml, (s))

static GdkPixmap *mini_pm[CONNECTION_LAST];
static GdkBitmap *mini_mask[CONNECTION_LAST];

static GdkPixmap *active_pm[2];
static GdkBitmap *active_mask[2];

static GSList *connections;

XstTool *tool;

static void
connection_set_modified (Connection *cxn, gboolean state)
{
	if (cxn->frozen || !xst_tool_get_access (tool))
		return;

	gtk_widget_set_sensitive (W ("connection_apply"), state);
	cxn->modified = state;
}

static void
load_icon (char *file, GdkPixmap **pixmap, GdkBitmap **mask)
{
	GdkPixbuf *pb, *pb2;
	char *path;

	path = g_concat_dir_and_file (PIXMAPS_DIR, file);
	
	pb = gdk_pixbuf_new_from_file (path);
	g_free (path);

	if (!pb)
		return;
	
	pb2 = gdk_pixbuf_scale_simple (pb, 24, 24, GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (pb);
	
	gdk_pixbuf_render_pixmap_and_mask (pb2, pixmap, mask, 127);
	gdk_pixbuf_unref (pb2);
}

void
init_icons (void)
{
	ConnectionType i;
	char *icons[CONNECTION_LAST] = {
		"networking.png",
		"connection-ethernet.png",
		"gnome-laptop.png",
		"connection-modem.png"
	};

	for (i = CONNECTION_OTHER; i < CONNECTION_LAST; i++)
		load_icon (icons[i], &mini_pm[i], &mini_mask[i]);

	load_icon ("gnome-light-off.png" /* "connection-inactive.xpm" */,
                   
		   &active_pm[0], &active_mask[0]);
	load_icon ("gnome-light-on.png" /*"connection-active.xpm" */,
		   
		   &active_pm[1], &active_mask[1]);
}

static void
update_row (Connection *cxn)
{
	GtkWidget *clist;
	int row;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");

	row = gtk_clist_find_row_from_data (GTK_CLIST (clist), cxn);

	gtk_clist_set_pixtext (GTK_CLIST (clist), row, 0, cxn->device, GNOME_PAD_SMALL,
			       mini_pm[cxn->type], mini_mask[cxn->type]);
	
	gtk_clist_set_pixtext (GTK_CLIST (clist), row, 1,
			       cxn->active ? _("Active") : _("Inactive"),
			       GNOME_PAD_SMALL,
			       active_pm[cxn->active ? 1 : 0], 
			       active_mask[cxn->active ? 1 : 0]);

	gtk_clist_set_text (GTK_CLIST (clist), row, 2, cxn->description);

	xst_dialog_modify (tool->main_dialog);
}

void
add_connection_to_list (Connection *cxn, gpointer null)
{
	GtkWidget *clist;
	int row;
	char *text[3] = { NULL };

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");

	row = gtk_clist_append (GTK_CLIST (clist), text);
	gtk_clist_set_row_data (GTK_CLIST (clist), row, cxn);

	update_row (cxn);
}

void
add_connections_to_list (void)
{
	g_slist_foreach (connections, add_connection_to_list, NULL);
}

Connection *
connection_new_from_node (xmlNode *node)
{
	Connection *cxn;
	xmlNode *subnode;
	char *s = NULL;

	subnode = xml_element_find_first (node, "dev");
	if (subnode) {
		s = xml_element_get_content (subnode);
	}

	if (s) {
		cxn = connection_new_from_dev_name (s);
		g_free (cxn->device);
		cxn->device = s;
	} else
		cxn = connection_new_from_type (CONNECTION_OTHER);
	
	
	subnode = xml_element_find_first (node, "bootproto");
	if (subnode) {
		s = xml_element_get_content (subnode);
		if (s) {
			if (!strcmp (s, "dhcp"))
				cxn->ip_config = cxn->tmp_ip_config = IP_DHCP;
			else if (!strcmp (s, "bootp"))
				cxn->ip_config = cxn->tmp_ip_config = IP_BOOTP;
			else
				cxn->ip_config = cxn->tmp_ip_config = IP_MANUAL;

			g_free (s);
		}
	}

	subnode = xml_element_find_first (node, "auto");
	if (subnode) {
		s = xml_element_get_content (subnode);
		if (s) {
			cxn->autoboot = atoi (s);
			g_free (s);
		}
	}

	subnode = xml_element_find_first (node, "enabled");
	if (subnode) {
		s = xml_element_get_content (subnode);
		if (s) {
			cxn->active = atoi (s)? TRUE: FALSE;
			g_free (s);
		}
	}

	update_row (cxn);

	return cxn;
}

Connection *
connection_new_from_dev_name (char *dev_name)
{
	typedef struct {
		gchar *name;
		ConnectionType type;
	} NameType;

	NameType table[] = {
		{ "eth", CONNECTION_ETH },
		{ "wvlan", CONNECTION_WVLAN },
		{ "ppp", CONNECTION_PPP },
		{ NULL, CONNECTION_OTHER }
	};

	int i;

	for (i = 0; table[i].name; i++)
		if (strstr (dev_name, table[i].name) == dev_name)
			break;

	return connection_new_from_type (table[i].type);
}

Connection *
connection_new_from_type (ConnectionType type)
{
	Connection *cxn;

	cxn = g_new0 (Connection, 1);
	cxn->type = type;
	
#warning FIXME: figure out a new device correctly
	switch (cxn->type) {
	case CONNECTION_ETH:
		cxn->device = g_strdup ("eth0");
		cxn->description = g_strdup (_("Ethernet connection"));
		break;
	case CONNECTION_WVLAN:
		cxn->device = g_strdup ("wvlan0");
		cxn->description = g_strdup (_("WaveLAN connection"));
		break;
	case CONNECTION_PPP:
		cxn->device = g_strdup ("ppp0");
		cxn->description = g_strdup (_("PPP connection"));
		break;
	default:
		cxn->device = g_strdup ("???");
		cxn->description = g_strdup ( _("Unknown type connection"));
		break;
	}	

	/* set up some defaults */
	cxn->autoboot = TRUE;
	cxn->dhcp_dns = TRUE;
	cxn->ip_config = cxn->tmp_ip_config = IP_DHCP;

	cxn->ip      = g_strdup ("10.0.1.10");
	cxn->subnet  = g_strdup ("255.255.0.0");
	cxn->gateway = g_strdup ("10.0.1.1");

	add_connection_to_list (cxn, NULL);
	
	return cxn;
}

void
connection_free (Connection *cxn)
{
#warning FIXME: implement
}

static void
update_status (Connection *cxn)
{
	gnome_pixmap_load_file (GNOME_PIXMAP (W ("status_icon")),
				GTK_TOGGLE_BUTTON (W ("status_button"))->active
				? PIXMAPS_DIR "/gnome-light-on.png"
				: PIXMAPS_DIR "/gnome-light-off.png");
}

void
on_status_button_toggled (GtkWidget *w, Connection *cxn)
{
	connection_set_modified (cxn, TRUE);
	update_status (cxn);
}

void
on_connection_help_clicked (GtkWidget *w, Connection *cxn)
{

}

static void
empty_general (Connection *cxn)
{
	g_free (cxn->description);
	cxn->description = gtk_editable_get_chars (GTK_EDITABLE (W ("connection_desc")), 0, -1);
		
	cxn->autoboot = GTK_TOGGLE_BUTTON (W ("status_boot"))->active;
	cxn->active = GTK_TOGGLE_BUTTON (W ("status_button"))->active;
}

static void
empty_ip (Connection *cxn)
{
	cxn->ip_config = cxn->tmp_ip_config;

	cxn->dhcp_dns = GTK_TOGGLE_BUTTON (W ("status_dhcp"))->active;

	g_free (cxn->ip);
	cxn->ip = gtk_editable_get_chars (GTK_EDITABLE (W ("ip_address")), 0, -1);

	g_free (cxn->subnet);
	cxn->subnet = gtk_editable_get_chars (GTK_EDITABLE (W ("ip_subnet")), 0, -1);

	g_free (cxn->gateway);
	cxn->gateway = gtk_editable_get_chars (GTK_EDITABLE (W ("ip_gateway")), 0, -1);		
}

static void
empty_wvlan (Connection *cxn)
{

}

static void
empty_ppp (Connection *cxn)
{

}

static void
connection_config_save (Connection *cxn)
{
	empty_general (cxn);
	empty_ip (cxn);

	switch (cxn->type) {
	case CONNECTION_WVLAN:
		empty_wvlan (cxn);
		break;
	case CONNECTION_PPP:
		empty_ppp (cxn);
		break;
	default:
		break;
	}

	connection_set_modified (cxn, FALSE);
	update_row (cxn);	
}

void
on_connection_apply_clicked (GtkWidget *w, Connection *cxn)
{
	connection_config_save (cxn);
}

static void
connection_close (Connection *cxn)
{
	GtkWidget *w;
	int reply;

	if (cxn->modified) {
		
		w = gnome_question_dialog_parented (
			_("There are changes which haven't been applyed.\nApply now?"),
			NULL, NULL, GTK_WINDOW (cxn->window));
		
		reply = gnome_dialog_run (GNOME_DIALOG (w));
		
		if (!reply)
			connection_config_save(cxn);
	}
}

void
connection_destroy (GtkWidget *w, Connection *cxn)
{
	gtk_object_unref (GTK_OBJECT (cxn->xml));
	cxn->xml = NULL;

	cxn->window = NULL;
}

void
on_connection_close_clicked (GtkWidget *wi, Connection *cxn)
{
	connection_close (cxn);
	gtk_widget_destroy (cxn->window);
}

gint
on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, Connection *cxn)
{
	connection_close (cxn);
	return FALSE;
}

void
connection_modified (GtkWidget *w, Connection *cxn)
{
	connection_set_modified (cxn, TRUE);
}

void
on_wvlan_adhoc_toggled (GtkWidget *w, Connection *cxn)
{

}

static void
fill_general (Connection *cxn)
{	
	gtk_label_set_text (GTK_LABEL (W ("connection_device")),
			    cxn->device);

	gtk_entry_set_text (GTK_ENTRY (W ("connection_desc")),
			    cxn->description);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_button")), cxn->dhcp_dns);

	update_status (cxn);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_boot")), cxn->autoboot);
}

static void
update_ip_config (Connection *cxn)
{
	IPConfigType ip;

	ip = cxn->tmp_ip_config;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_dhcp")), cxn->dhcp_dns);
	gtk_widget_set_sensitive (W ("status_dhcp"), ip != IP_MANUAL);
	gtk_widget_set_sensitive (W ("ip_table"), ip == IP_MANUAL);
}

static void
ip_config_menu_cb (GtkWidget *w, gpointer data)
{
	Connection *cxn;
	IPConfigType ip;

	cxn = gtk_object_get_user_data (GTK_OBJECT (w));

	if (cxn->frozen)
		return;

	ip = GPOINTER_TO_INT (data);

	if (cxn->tmp_ip_config == ip)
		return;

	cxn->tmp_ip_config = GPOINTER_TO_INT (data);

	connection_set_modified (cxn, TRUE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_dhcp")),
				      ip != IP_MANUAL);

	update_ip_config (cxn);
}

static void
fill_ip (Connection *cxn)
{
	GtkWidget *menu, *menuitem, *omenu;
	IPConfigType i;

	char *blah[] = {
		N_("Manual"),
		N_("DHCP"),
		N_("BOOTP")
	};

	omenu = W ("connection_config");

	menu = gtk_menu_new ();
	for (i = 0; i < 3; i++) {
		menuitem = gtk_menu_item_new_with_label (_(blah[i]));
		gtk_object_set_user_data (GTK_OBJECT (menuitem), cxn);
		gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				    GTK_SIGNAL_FUNC (ip_config_menu_cb),
				    GINT_TO_POINTER (i));
		gtk_menu_append (GTK_MENU (menu), menuitem);
	}
	gtk_widget_show_all (menu);

	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), cxn->ip_config);	

	update_ip_config (cxn);

	gtk_entry_set_text (GTK_ENTRY (W ("ip_address")), cxn->ip);
	gtk_entry_set_text (GTK_ENTRY (W ("ip_subnet")), cxn->subnet);
	gtk_entry_set_text (GTK_ENTRY (W ("ip_gateway")), cxn->gateway);
}

static void
fill_wvlan (Connection *cxn)
{

}

static void
fill_ppp (Connection *cxn)
{

}

typedef struct {
	const char *hname;
	gpointer signalfunc;
} WidgetSignal;

static void
hookup_callbacks (Connection *cxn)
{
	int i;
	WidgetSignal signals[] = {
		{ "on_connection_help_clicked", on_connection_help_clicked },
	     /* { "on_connection_complexity_clicked", on_connection_complexity_clicked }, */
		{ "on_connection_apply_clicked", on_connection_apply_clicked },
		{ "on_connection_close_clicked", on_connection_close_clicked },
		{ "on_connection_config_dialog_delete_event", on_connection_config_dialog_delete_event },
		{ "on_status_button_toggled", on_status_button_toggled },
		{ "connection_modified", connection_modified },
		{ "connection_destroy", connection_destroy },
		{ "on_wvlan_adhoc_toggled", on_wvlan_adhoc_toggled },
		{ NULL } };

	for (i = 0; signals[i].hname; i++)
		glade_xml_signal_connect_data (cxn->xml, signals[i].hname,
					       signals[i].signalfunc, cxn);
}

void
connection_configure (Connection *cxn)
{
	GtkWidget *nb, *hb, *qm;
	char *s;

	if (cxn->window) {
		gdk_window_show (cxn->window->window);
		gdk_window_raise (cxn->window->window);
		return;
	}

	g_assert (!cxn->xml);

	cxn->frozen = TRUE;

	s = g_concat_dir_and_file (INTERFACES_DIR, "network.glade");
	cxn->xml = glade_xml_new (s, "connection_config_dialog");

	g_assert (cxn->xml);

	hookup_callbacks (cxn);

	cxn->window = W("connection_config_dialog");

	hb = W ("connection_help");
	qm = gnome_stock_pixmap_widget_new (hb, GNOME_STOCK_PIXMAP_HELP);
	gtk_widget_show (qm);
	gtk_container_add (GTK_CONTAINER (hb), qm);
	gtk_widget_set_sensitive (hb, FALSE);
			   
	fill_general (cxn);
	fill_ip      (cxn);

	/* would like to do this as a switch */
	nb = W ("connection_nb");
	if (cxn->type == CONNECTION_PPP)
		fill_ppp (cxn);
	else
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
					  gtk_notebook_page_num (GTK_NOTEBOOK (nb),
								 W ("ppp_vbox")));
       
	if (cxn->type == CONNECTION_WVLAN)
		fill_wvlan (cxn);
	else
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
					  gtk_notebook_page_num (GTK_NOTEBOOK (nb),
								 W ("wvlan_vbox")));

	cxn->frozen = FALSE;

	connection_set_modified (cxn, FALSE);

	gtk_widget_show (cxn->window);
}

void
connection_save_to_node (Connection *cxn, xmlNode *node)
{
#warning FIXME: implement
}

