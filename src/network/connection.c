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

static void on_status_button_toggled (GtkWidget *w, Connection *cxn);
static void on_connection_ok_clicked (GtkWidget *w, Connection *cxn);
static void on_connection_cancel_clicked (GtkWidget *w, Connection *cxn);
static void on_connection_config_dialog_destroy (GtkWidget *w, Connection *cxn);
static gint on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, Connection *cxn);
static void on_connection_modified (GtkWidget *w, Connection *cxn);
static void on_wvlan_adhoc_toggled (GtkWidget *w, Connection *cxn);

#define W(s) my_get_widget (cxn->xml, (s))

static GdkPixmap *mini_pm[CONNECTION_LAST];
static GdkBitmap *mini_mask[CONNECTION_LAST];

static GdkPixmap *active_pm[2];
static GdkBitmap *active_mask[2];

/*static GSList *connections;*/

XstTool *tool;

static GtkWidget *
my_get_widget (GladeXML *glade, const gchar *name)
{
	GtkWidget *w;

	g_return_val_if_fail (glade != NULL, NULL);
	
	w = glade_xml_get_widget (glade, name);
	if (!w)
		g_warning ("my_get_widget: Unexistent widget %s", name);

	return w;
}

static void
my_entry_set_text (GtkEntry *w, gchar *txt)
{
	gtk_entry_set_text (w, (txt)? txt: "");
}

static gchar *
connection_xml_get_str (xmlNode *node, gchar *elem)
{
	xmlNode *subnode;
	gchar *ret;

	ret = NULL;
	
	subnode = xml_element_find_first (node, elem);
	if (subnode) {
		ret = xml_element_get_content (subnode);
	}

	return ret;
}

static gboolean
connection_xml_get_boolean (xmlNode *node, gchar *elem)
{
	gchar *s;
	gboolean ret;

	ret = FALSE;

	s = connection_xml_get_str (node, elem);
	if (s) {
		ret = atoi (s)? TRUE: FALSE;
		g_free (s);
	}

	return ret;
}

static void
connection_xml_save_str_to_node (xmlNode *node, gchar *node_name, gchar *str)
{
	xmlNode *subnode;

	if (!str)
		return;

	subnode = xml_element_find_first (node, node_name);

	if (*str == 0) {
		if (subnode)
			xml_element_destroy_children_by_name (node, node_name);
	} else {
		if (!subnode)
			subnode = xml_element_add (node, node_name);
		xml_element_set_content (subnode, str);
	}
}

static void
connection_xml_save_boolean_to_node (xmlNode *node, gchar *node_name, gboolean bool)
{
	connection_xml_save_str_to_node (node, node_name, bool? "1": "0");
}

static IPConfigType
connection_config_type_from_str (gchar *str)
{
	gchar *protos[] = { "dhcp", "bootp", NULL };
	IPConfigType types[] = { IP_DHCP, IP_BOOTP, IP_MANUAL };
	gint i;
	
	for (i = 0; protos[i]; i++)
		if (!strcmp (str, protos[i]))
			break;
	
	return types[i];
}

static gchar *
connection_config_type_to_str (IPConfigType type)
{
	gchar *protos[] = { "dhcp", "bootp", "none" };
	IPConfigType types[] = { IP_DHCP, IP_BOOTP, IP_MANUAL };
	gint i;
	
	for (i = 0; types[i] != IP_MANUAL; i++)
		if (type == types[i])
			break;
	
	return g_strdup (protos[i]);
}

static void
connection_set_modified (Connection *cxn, gboolean state)
{
	if (cxn->frozen || !xst_tool_get_access (tool))
		return;

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

extern void
connection_init_icons (void)
{
	ConnectionType i;
	char *icons[CONNECTION_LAST] = {
		"networking.png",
		"connection-ethernet.png",
		"gnome-laptop.png",
		"connection-modem.png",
		"networking.png",
		"networking.png"
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

	gtk_clist_set_pixtext (GTK_CLIST (clist), row, 0, cxn->dev, GNOME_PAD_SMALL,
			       mini_pm[cxn->type], mini_mask[cxn->type]);
	
	gtk_clist_set_pixtext (GTK_CLIST (clist), row, 1,
			       cxn->enabled ? _("Active") : _("Inactive"),
			       GNOME_PAD_SMALL,
			       active_pm[cxn->enabled ? 1 : 0], 
			       active_mask[cxn->enabled ? 1 : 0]);

	gtk_clist_set_text (GTK_CLIST (clist), row, 2, cxn->name);

	xst_dialog_modify (tool->main_dialog);
}

static void
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

/*static void
add_connections_to_list (void)
{
	g_slist_foreach (connections, (GFunc) add_connection_to_list, NULL);
}*/

static const gchar *
connection_description_from_type (ConnectionType type)
{
	gchar *descriptions[] = {
		N_("Ethernet LAN card"),
		N_("WaveLAN wireless LAN"),
		N_("PPP: modem or transfer cable"),
		N_("Loopback: virtual interface"),
		N_("Parallel line"),
		N_("Other type"),
		N_("Unknown type")
	};

	ConnectionType types[] = {
		CONNECTION_ETH,
		CONNECTION_WVLAN,
		CONNECTION_PPP,
		CONNECTION_LO,
		CONNECTION_PLIP,
		CONNECTION_OTHER,
		CONNECTION_UNKNOWN
	};

	gint i;

	for (i = 0; types[i] != CONNECTION_UNKNOWN; i++)
		if (type == types[i])
			break;

	return descriptions[i];
}

Connection *
connection_new_from_node (xmlNode *node)
{
	Connection *cxn;
	char *s = NULL;

	s = connection_xml_get_str (node, "dev");

	if (s) {
		cxn = connection_new_from_dev_name (s);
		g_free (cxn->dev);
		cxn->dev = s;
	} else
		cxn = connection_new_from_type (CONNECTION_OTHER);

	cxn->node = node;
	
	s = connection_xml_get_str (node, "bootproto");
	if (s) {
		cxn->ip_config = cxn->tmp_ip_config = connection_config_type_from_str (s);
		g_free (s);
	}

	s = connection_xml_get_str (node, "name");
	if (s)
		cxn->name = s;
	else
		cxn->name = g_strdup (connection_description_from_type (cxn->type));
		
	cxn->user = connection_xml_get_boolean (node, "user");
	cxn->autoboot = connection_xml_get_boolean (node, "auto");
	cxn->enabled = connection_xml_get_boolean (node, "enabled");

	cxn->address = connection_xml_get_str (node, "address");
	cxn->netmask = connection_xml_get_str (node, "netmask");
	cxn->gateway = connection_xml_get_str (node, "gateway");

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
		{ "lo", CONNECTION_LO },
		{ "plip", CONNECTION_PLIP },
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
		cxn->dev = g_strdup ("eth0");
		break;
	case CONNECTION_WVLAN:
		cxn->dev = g_strdup ("wvlan0");
		break;
	case CONNECTION_PPP:
		cxn->dev = g_strdup ("ppp0");
		break;
	case CONNECTION_LO:
		cxn->dev = g_strdup ("lo");
		break;
	case CONNECTION_PLIP:
		cxn->dev = g_strdup ("plip0");
		break;
	default:
		cxn->dev = g_strdup ("???");
		break;
	}	

	/* set up some defaults */
	cxn->autoboot = TRUE;
	cxn->user = FALSE;
	cxn->dhcp_dns = TRUE;
	cxn->ip_config = cxn->tmp_ip_config = IP_DHCP;

	add_connection_to_list (cxn, NULL);
	
	return cxn;
}

void
connection_free (Connection *cxn)
{
#warning FIXME: implement connection_free
}

static void
update_status (Connection *cxn)
{
	gnome_pixmap_load_file (GNOME_PIXMAP (W ("status_icon")),
				GTK_TOGGLE_BUTTON (W ("status_button"))->active
				? PIXMAPS_DIR "/gnome-light-on.png"
				: PIXMAPS_DIR "/gnome-light-off.png");
}

static void
on_status_button_toggled (GtkWidget *w, Connection *cxn)
{
	connection_set_modified (cxn, TRUE);
	update_status (cxn);
}

static void
empty_general (Connection *cxn)
{
	g_print ("About to free *%s*\n", cxn->name);
	g_free (cxn->name);
	cxn->name = gtk_editable_get_chars (GTK_EDITABLE (W ("connection_desc")), 0, -1);
		
	cxn->autoboot = GTK_TOGGLE_BUTTON (W ("status_boot"))->active;
	cxn->user = GTK_TOGGLE_BUTTON (W ("status_user"))->active;
	cxn->enabled = GTK_TOGGLE_BUTTON (W ("status_button"))->active;
}

static void
empty_ip (Connection *cxn)
{
	cxn->ip_config = cxn->tmp_ip_config;

	cxn->dhcp_dns = GTK_TOGGLE_BUTTON (W ("status_dhcp"))->active;

	g_free (cxn->address);
	cxn->address = gtk_editable_get_chars (GTK_EDITABLE (W ("ip_address")), 0, -1);

	g_free (cxn->netmask);
	cxn->netmask = gtk_editable_get_chars (GTK_EDITABLE (W ("ip_netmask")), 0, -1);

	g_free (cxn->gateway);
	cxn->gateway = gtk_editable_get_chars (GTK_EDITABLE (W ("ip_gateway")), 0, -1);

	/* FIXME: calculate broadcast */
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

static void
on_connection_ok_clicked (GtkWidget *w, Connection *cxn)
{
	connection_config_save (cxn);
	gtk_widget_destroy (cxn->window);
}

static void
on_connection_cancel_clicked (GtkWidget *wi, Connection *cxn)
{
	gtk_widget_destroy (cxn->window);
}

static void
on_connection_config_dialog_destroy (GtkWidget *w, Connection *cxn)
{
	gtk_object_unref (GTK_OBJECT (cxn->xml));
	cxn->xml = NULL;

	cxn->window = NULL;
}

static gint
on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, Connection *cxn)
{
	return FALSE;
}

static void
on_connection_modified (GtkWidget *w, Connection *cxn)
{
	connection_set_modified (cxn, TRUE);
}

static void
on_wvlan_adhoc_toggled (GtkWidget *w, Connection *cxn)
{
#warning FIXME: implement on_wvlan_adhoc_toggled
}

static void
fill_general (Connection *cxn)
{	
	gtk_label_set_text (GTK_LABEL (W ("connection_device")), cxn->dev);
	my_entry_set_text (GTK_ENTRY (W ("connection_desc")), cxn->name);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_button")), cxn->dhcp_dns);

	update_status (cxn);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_boot")), cxn->autoboot);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_user")), cxn->user);
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

	my_entry_set_text (GTK_ENTRY (W ("ip_address")), cxn->address);
	my_entry_set_text (GTK_ENTRY (W ("ip_netmask")), cxn->netmask);
	my_entry_set_text (GTK_ENTRY (W ("ip_gateway")), cxn->gateway);
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
		{ "on_connection_ok_clicked", on_connection_ok_clicked },
		{ "on_connection_cancel_clicked", on_connection_cancel_clicked },
		{ "on_connection_config_dialog_delete_event", on_connection_config_dialog_delete_event },
		{ "on_status_button_toggled", on_status_button_toggled },
		{ "on_connection_modified", on_connection_modified },
		{ "on_connection_config_dialog_destroy", on_connection_config_dialog_destroy },
		{ "on_wvlan_adhoc_toggled", on_wvlan_adhoc_toggled },
		{ NULL } };

	for (i = 0; signals[i].hname; i++)
		glade_xml_signal_connect_data (cxn->xml, signals[i].hname,
					       signals[i].signalfunc, cxn);
}

void
connection_configure (Connection *cxn)
{
	GtkWidget *nb;
/*	GtkWidget *hb, *qm;*/
	char *s;

	if (cxn->window) {
		gtk_widget_show (cxn->window);
/*		gdk_window_show (cxn->window->window);
		gdk_window_raise (cxn->window->window);*/
		return;
	}

	g_assert (!cxn->xml);

	cxn->frozen = TRUE;

	s = g_concat_dir_and_file (INTERFACES_DIR, "network.glade");
	cxn->xml = glade_xml_new (s, "connection_config_dialog");

	g_assert (cxn->xml);

	hookup_callbacks (cxn);

	cxn->window = W("connection_config_dialog");

/*	hb = W ("connection_help");
	qm = gnome_stock_pixmap_widget_new (hb, GNOME_STOCK_PIXMAP_HELP);
	gtk_widget_show (qm);
	gtk_container_add (GTK_CONTAINER (hb), qm);
	gtk_widget_set_sensitive (hb, FALSE);*/
			   
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
connection_save_to_node (Connection *cxn, xmlNode *root)
{
	gchar *s;
	
	if (!cxn->node)
		cxn->node = xml_element_add (root, "interface");
		
	connection_xml_save_str_to_node (cxn->node, "dev", cxn->dev);
	connection_xml_save_str_to_node (cxn->node, "name", cxn->name);
	connection_xml_save_boolean_to_node (cxn->node, "enabled", cxn->enabled);
	connection_xml_save_boolean_to_node (cxn->node, "user", cxn->user);
	connection_xml_save_boolean_to_node (cxn->node, "auto", cxn->autoboot);
	connection_xml_save_str_to_node (cxn->node, "address", cxn->address);
	connection_xml_save_str_to_node (cxn->node, "netmask", cxn->netmask);
	connection_xml_save_str_to_node (cxn->node, "broadcast", cxn->broadcast);
	connection_xml_save_str_to_node (cxn->node, "network", cxn->network);
	connection_xml_save_str_to_node (cxn->node, "gateway", cxn->gateway);

	s = connection_config_type_to_str (cxn->ip_config);
	connection_xml_save_str_to_node (cxn->node, "bootproto", s);
	g_free (s);
}

