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

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "connection.h"

#include "global.h"


/* sigh more libglade callbacks */

static void on_status_enabled_toggled (GtkWidget *w, Connection *cxn);
static void on_connection_ok_clicked (GtkWidget *w, Connection *cxn);
static void on_connection_cancel_clicked (GtkWidget *w, Connection *cxn);
static void on_connection_config_dialog_destroy (GtkWidget *w, Connection *cxn);
static gint on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, Connection *cxn);
static void on_connection_modified (GtkWidget *w, Connection *cxn);
static void on_wvlan_adhoc_toggled (GtkWidget *w, Connection *cxn);

#define W(s) my_get_widget (cxn->xml, (s))

#define GET_STR(yy_prefix,xx) g_free (cxn->xx); cxn->xx = gtk_editable_get_chars (GTK_EDITABLE (W (yy_prefix#xx)), 0, -1);
#define GET_BOOL(yy_prefix,xx) cxn->xx = GTK_TOGGLE_BUTTON (W (yy_prefix#xx))->active;
#define SET_STR(yy_prefix,xx) my_entry_set_text (GTK_ENTRY (W (yy_prefix#xx)), cxn->xx);
#define SET_BOOL(yy_prefix,xx) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W (yy_prefix#xx)), cxn->xx);
	

static GdkPixmap *mini_pm[CONNECTION_LAST];
static GdkBitmap *mini_mask[CONNECTION_LAST];

static GdkPixmap *active_pm[2];
static GdkBitmap *active_mask[2];

/*static GSList *connections;*/

XstTool *tool;

typedef struct {
	const char *hname;
	gpointer signalfunc;
} WidgetSignal;

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
	
	subnode = xst_xml_element_find_first (node, elem);
	if (subnode)
		ret = xst_xml_element_get_content (subnode);

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

	subnode = xst_xml_element_find_first (node, node_name);

	if (*str == 0) {
		if (subnode)
			xst_xml_element_destroy_children_by_name (node, node_name);
	} else {
		if (!subnode)
			subnode = xst_xml_element_add (node, node_name);
		xst_xml_element_set_content (subnode, str);
	}
}

static void
connection_xml_save_boolean_to_node (xmlNode *node, gchar *node_name, gboolean bool)
{
	connection_xml_save_str_to_node (node, node_name, bool? "1": "0");
}

static gboolean
connection_xml_wvsection_is_type (xmlNode *node, gchar *type)
{
	gchar *str;
	gint cmp;

	str = connection_xml_get_str (node, "type");
	cmp = strcmp (str, type);
	g_free (str);

	if (!cmp)
		return TRUE;

	return FALSE;
}

static gboolean
connection_xml_wvsection_has_name (xmlNode *node, gchar *name)
{
	gchar *section_found;
	gint cmp;
	
	section_found = connection_xml_get_str (node, "name");
	cmp = strcmp (section_found, name);
	g_free (section_found);

	return !cmp;
}

/* Use type == NULL if you don't care about the type. */
static xmlNode *
connection_xml_wvsection_search (xmlNode *node, gchar *section_name, gchar *type)
{
	gchar *types[] = { "dialer", "modem", NULL };
	gint i;

	/* Check that type is valid */
	if (type) {
		for (i = 0; type[i]; i++)
			if (!strcmp (types[i], type))
				break;

		g_return_val_if_fail (types[i] != NULL, NULL);
	}
	
	g_return_val_if_fail (section_name != NULL, NULL);
	
	for (node = xst_xml_element_find_first (node, "dialing");
		node; node = xst_xml_element_find_next (node, "dialing"))
	{
		if (type && !connection_xml_wvsection_is_type (node, type))
			continue;
		
		if (connection_xml_wvsection_has_name (node, section_name))
			break;
	}

	if (!node)
		g_warning ("connection_xml_wvsection_search: section %s type %s not found.",
				 section_name, type);
	
	return node;
}

static xmlNode *
connection_xml_wvsection_get_inherits_node (xmlNode *root, xmlNode *node)
{
	gchar *prefix[] = { "Dialer ", "Modem", NULL };
	gchar *inherits, *c;
	gint i;
	xmlNode *inherit_node;

	g_return_val_if_fail (root != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);

	inherits = connection_xml_get_str (node, "inherits");
	if (inherits) {
		for (i = 0; prefix[i]; i++)
			if (strstr (inherits, prefix[i]) == inherits)
				break;
		
		g_return_val_if_fail (prefix[i] != NULL, NULL);
		
		c = inherits + strlen (prefix[i]);
		inherit_node = connection_xml_wvsection_search (root, c, NULL);
		g_free (inherits);
		
		g_return_val_if_fail (inherit_node != node, NULL);
		if (inherit_node)
			return inherit_node;
		
		g_warning ("connection_xml_wvsection_get_inherits: inherited section doesn't exist.");
	}
	
	inherit_node = connection_xml_wvsection_search (root, "Defaults", "dialer");
	if (inherit_node == node)
		return NULL;
	return inherit_node;
}

static gchar *
connection_xml_wvsection_node_get_str (xmlNode *node, xmlNode *subnode, gchar *elem)
{
	gchar *value;

	if (subnode) {
		/* Found the section */
		value = connection_xml_get_str (subnode, elem);
		if (value) {
			/* Got the required value */
			return value;
		} else {
			/* Value not found. Try inherited section. */
			subnode = connection_xml_wvsection_get_inherits_node (node, subnode);
			return connection_xml_wvsection_node_get_str (node, subnode, elem);
		}
	}

	return NULL;
}

static gchar *
connection_xml_wvsection_get_str (xmlNode *node, gchar *section_name, gchar *elem)
{
	xmlNode *subnode;
	
	subnode = connection_xml_wvsection_search (node, section_name, "dialer");
	return connection_xml_wvsection_node_get_str (node, subnode, elem);
}

static gboolean
connection_xml_wvsection_get_boolean (xmlNode *node, gchar *section_name, gchar *elem)
{
	gchar *str;
	gboolean ret;

	ret = FALSE;
	str = connection_xml_wvsection_get_str (node, section_name, elem);
	if (str) {
		ret = atoi (str)? TRUE: FALSE;
		g_free (str);
	}

	return ret;
}

static xmlNode *
connection_xml_wvsection_add (xmlNode *node, gchar *section_name, gchar *type)
{
	xmlNode *subnode;

	subnode = xst_xml_element_add (node, "dialing");
	connection_xml_save_str_to_node (subnode, "name", section_name);
	connection_xml_save_str_to_node (subnode, "type", type);

	return subnode;
}

static void
connection_xml_wvsection_save_str_to_node (xmlNode *node, gchar *section_name, gchar *node_name, gchar *str)
{
	xmlNode *subnode;

	subnode = connection_xml_wvsection_search (node, section_name, "dialer");
	if (!subnode)
		subnode = connection_xml_wvsection_add (node, section_name, "dialer");

	connection_xml_save_str_to_node (subnode, node_name, str);
}

static void
connection_xml_wvsection_save_boolean_to_node (xmlNode *node, gchar *section_name, 
									  gchar *node_name, gboolean bool)
{
	connection_xml_wvsection_save_str_to_node (node, section_name, node_name, bool? "1": "0");
}

static gchar *
connection_devkey_generate (gchar *dev)
{
	static gboolean flag = FALSE;
	long int num;

	if (!flag) {
		srandom (time (NULL));
		flag = TRUE;
	}
	
	num = random ();

	return g_strdup_printf ("%s_%ld", dev, num);
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

static gchar *
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

	return g_strdup (descriptions[i]);
}

static void
connection_get_ppp_from_node (xmlNode *node, Connection *cxn)
{
	cxn->wvsection = connection_xml_get_str (node, "wvsection");
	if (cxn->wvsection) {
		cxn->phone_number = connection_xml_wvsection_get_str (node->parent, cxn->wvsection, "phone");
		cxn->login = connection_xml_wvsection_get_str (node->parent, cxn->wvsection, "login");
		cxn->password = connection_xml_wvsection_get_str (node->parent, cxn->wvsection, "password");
		cxn->stupid = connection_xml_wvsection_get_boolean (node->parent, cxn->wvsection, "stupid");
	} else {
		cxn->wvsection = connection_devkey_generate (cxn->dev);
		connection_xml_save_str_to_node (cxn->node, "wvsection", cxn->wvsection);

		cxn->phone_number = connection_xml_get_str (node, "phone_number");
		cxn->login = connection_xml_get_str (node, "login");
		cxn->password = connection_xml_get_str (node, "password");
		cxn->stupid = FALSE;
	}

	/* PPP advanced */
	cxn->persist = connection_xml_get_boolean (node, "persist");
	cxn->serial_port = connection_xml_get_str (node, "serial_port");
	cxn->set_default_gw = connection_xml_get_boolean (node, "set_default_gw");
	cxn->dns1 = connection_xml_get_str (node, "dns1");
	cxn->dns2 = connection_xml_get_str (node, "dns2");
	cxn->ppp_options = connection_xml_get_str (node, "ppp_options");
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
		cxn->name = connection_description_from_type (cxn->type);
	
	s = connection_xml_get_str (node, "file");
	if (s) {
		g_free (cxn->file);
		cxn->file = s;
	}

	/* Activation */
	cxn->user = connection_xml_get_boolean (node, "user");
	cxn->autoboot = connection_xml_get_boolean (node, "auto");
	cxn->enabled = connection_xml_get_boolean (node, "enabled");

	/* TCP/IP general paramaters */
	cxn->address = connection_xml_get_str (node, "address");
	cxn->netmask = connection_xml_get_str (node, "netmask");
	cxn->gateway = connection_xml_get_str (node, "gateway");

	/* PPP stuff */
	if (cxn->type == CONNECTION_PPP)
		connection_get_ppp_from_node (cxn->node, cxn);

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
		cxn->autoboot = TRUE;
		cxn->dev = g_strdup ("eth0");
		break;
	case CONNECTION_WVLAN:
		cxn->autoboot = TRUE;
		cxn->dev = g_strdup ("wvlan0");
		break;
	case CONNECTION_PPP:
		cxn->autoboot = FALSE;
		cxn->dev = g_strdup ("ppp0");
		break;
	case CONNECTION_LO:
		cxn->autoboot = TRUE;
		cxn->dev = g_strdup ("lo");
		break;
	case CONNECTION_PLIP:
		cxn->autoboot = FALSE;
		cxn->dev = g_strdup ("plip0");
		break;
	default:
		cxn->dev = g_strdup ("NIL");
		break;
	}	

	/* set up some defaults */
	cxn->file = connection_devkey_generate (cxn->dev);
	cxn->user = FALSE;
	cxn->dhcp_dns = TRUE;
	cxn->ip_config = cxn->tmp_ip_config = IP_MANUAL;

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
				GTK_TOGGLE_BUTTON (W ("status_enabled"))->active
				? PIXMAPS_DIR "/gnome-light-on.png"
				: PIXMAPS_DIR "/gnome-light-off.png");
}

static void
on_status_enabled_toggled (GtkWidget *w, Connection *cxn)
{
	connection_set_modified (cxn, TRUE);
	update_status (cxn);
}

static void
empty_general (Connection *cxn)
{
	GET_STR ("connection_", name);
	GET_BOOL ("status_", autoboot);
	GET_BOOL ("status_", user);
	GET_BOOL ("status_", enabled);
}

static void
empty_ip (Connection *cxn)
{
	cxn->ip_config = cxn->tmp_ip_config;
	GET_BOOL ("status_", dhcp_dns);
	GET_STR ("ip_", address);
	GET_STR ("ip_", netmask);
	GET_STR ("ip_", gateway);

#warning FIXME: calculate broadcast and stuff in empty_id
}

static void
empty_wvlan (Connection *cxn)
{
}

static void
empty_ppp (Connection *cxn)
{
	GET_STR ("ppp_", phone_number);
	GET_STR ("ppp_", login);
	GET_STR ("ppp_", password);
	GET_BOOL ("ppp_", persist);
}

static void
empty_ppp_adv (Connection *cxn)
{
	GET_STR ("ppp_", serial_port);
	GET_BOOL ("ppp_", stupid);
	GET_BOOL ("ppp_", set_default_gw);
	GET_STR ("ppp_", dns1);
	GET_STR ("ppp_", dns2);
	GET_STR ("ppp_", ppp_options);
}

static void
connection_config_save (Connection *cxn)
{
	empty_general (cxn);

	switch (cxn->type) {
	case CONNECTION_WVLAN:
		empty_wvlan (cxn);
		empty_ip (cxn);
		break;
	case CONNECTION_PPP:
		empty_ppp (cxn);
		empty_ppp_adv (cxn);
		break;
	default:
		empty_ip (cxn);
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
	gtk_label_set_text (GTK_LABEL (W ("connection_dev")), cxn->dev);
	SET_STR ("connection_", name);
	SET_BOOL ("status_", enabled);
	update_status (cxn);
	SET_BOOL ("status_", autoboot);
	SET_BOOL ("status_", user);
}

static void
update_ip_config (Connection *cxn)
{
	IPConfigType ip;

	ip = cxn->tmp_ip_config;

	SET_BOOL ("status_", dhcp_dns);
	gtk_widget_set_sensitive (W ("status_dhcp_dns"), ip != IP_MANUAL);
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

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_dhcp_dns")),
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

	SET_STR ("ip_", address);
	SET_STR ("ip_", netmask);
	SET_STR ("ip_", gateway);
}

static void
fill_wvlan (Connection *cxn)
{

}

static void
fill_ppp (Connection *cxn)
{
	SET_STR ("ppp_", phone_number);
	SET_STR ("ppp_", login);
	SET_STR ("ppp_", password);
	SET_BOOL ("ppp_", persist);
}

static void
fill_ppp_adv (Connection *cxn)
{
	gnome_entry_load_history (GNOME_ENTRY (W ("ppp_serial_port_g")));
	SET_STR ("ppp_", serial_port);
	SET_BOOL ("ppp_", stupid);
	SET_BOOL ("ppp_", set_default_gw);
	SET_STR ("ppp_", dns1);
	SET_STR ("ppp_", dns2);
	SET_STR ("ppp_", ppp_options);
}

static void
hookup_callbacks (Connection *cxn)
{
	int i;
	WidgetSignal signals[] = {
		{ "on_connection_ok_clicked", on_connection_ok_clicked },
		{ "on_connection_cancel_clicked", on_connection_cancel_clicked },
		{ "on_connection_config_dialog_delete_event", on_connection_config_dialog_delete_event },
		{ "on_status_enabled_toggled", on_status_enabled_toggled },
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
	if (cxn->type == CONNECTION_PPP) {
		fill_ppp (cxn);
		fill_ppp_adv (cxn);
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
							 gtk_notebook_page_num (GTK_NOTEBOOK (nb),
											    W ("ip_vbox")));
	} else {
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
							 gtk_notebook_page_num (GTK_NOTEBOOK (nb),
											    W ("ppp_vbox")));
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
							 gtk_notebook_page_num (GTK_NOTEBOOK (nb),
											    W ("ppp_adv_vbox")));
	}
       
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
	xmlNode *node;
	
	if (!cxn->node)
		cxn->node = xst_xml_element_add (root, "interface");
	
	node = cxn->node;
		
	connection_xml_save_str_to_node (node, "dev", cxn->dev);
	connection_xml_save_str_to_node (node, "name", cxn->name);

	/* Activation */
	connection_xml_save_boolean_to_node (node, "user", cxn->user);
	connection_xml_save_boolean_to_node (node, "auto", cxn->autoboot);
	connection_xml_save_boolean_to_node (node, "enabled", cxn->enabled);

	/* TCP/IP general paramaters */
	connection_xml_save_str_to_node (node, "address", cxn->address);
	connection_xml_save_str_to_node (node, "netmask", cxn->netmask);
	connection_xml_save_str_to_node (node, "broadcast", cxn->broadcast);
	connection_xml_save_str_to_node (node, "network", cxn->network);
	connection_xml_save_str_to_node (node, "gateway", cxn->gateway);

	s = connection_config_type_to_str (cxn->ip_config);
	connection_xml_save_str_to_node (node, "bootproto", s);
	g_free (s);

	/* PPP stuff */
	if (cxn->type == CONNECTION_PPP) {
		connection_xml_wvsection_save_str_to_node (root, cxn->wvsection, "phone", cxn->phone_number);
		connection_xml_wvsection_save_str_to_node (root, cxn->wvsection, "login", cxn->login);
		connection_xml_wvsection_save_str_to_node (root, cxn->wvsection, "password", cxn->password);
		connection_xml_wvsection_save_boolean_to_node (root, cxn->wvsection, "stupid", cxn->stupid);
		
		/* PPP advanced */
		connection_xml_save_boolean_to_node (node, "persist", cxn->persist);
		connection_xml_save_str_to_node (node, "serial_port", cxn->serial_port);
		connection_xml_save_boolean_to_node (node, "set_default_gw", cxn->set_default_gw);
		connection_xml_save_str_to_node (node, "dns1", cxn->dns1);
		connection_xml_save_str_to_node (node, "dns2", cxn->dns2);
		connection_xml_save_str_to_node (node, "ppp_options", cxn->ppp_options);
	}
}
