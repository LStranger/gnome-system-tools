/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
 *          Arturo Espinosa <arturo@ximian.com>
 */

#include <config.h>

#include <string.h>
#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "connection.h"
#include "callbacks.h"

#include "xst.h"

typedef struct _XstNetworkInterfaceDescription XstNetworkInterfaceDescription;

struct _XstNetworkInterfaceDescription {
	const gchar * description;
	XstConnectionType type;
	const gchar * icon;
	const gchar * name;
};

static const XstNetworkInterfaceDescription xst_iface_desc [] = {
	{ N_("Other type"),                   XST_CONNECTION_OTHER,   "network.png",     "other_type" },
	{ N_("Ethernet LAN card"),            XST_CONNECTION_ETH,     "16_ethernet.xpm", "eth"        },
	{ N_("WaveLAN wireless LAN"),         XST_CONNECTION_WVLAN,   "wavelan-16.png",  "wvlan"      },
	{ N_("PPP: modem or transfer cable"), XST_CONNECTION_PPP,     "16_ppp.xpm",      "ppp"        },
	{ N_("Parallel line"),                XST_CONNECTION_PLIP,    "16_plip.xpm",     "plip"       },
	{ N_("Infrared LAN"),                 XST_CONNECTION_IRLAN,   "irda-16.png",     "irlan"      },
	{ N_("Loopback: virtual interface"),  XST_CONNECTION_LO,      "16_loopback.xpm", "lo"         },
	{ N_("Unknown type"),                 XST_CONNECTION_UNKNOWN, "network.png",     NULL         },
	{ NULL,                               XST_CONNECTION_UNKNOWN, NULL,              NULL         }
};


/* sigh more libglade callbacks */
/*static void on_status_enabled_toggled (GtkWidget *w, XstConnection *cxn);*/
static void on_connection_ok_clicked (GtkWidget *w, XstConnection *cxn);
static void on_connection_cancel_clicked (GtkWidget *w, XstConnection *cxn);
static void on_connection_config_dialog_destroy (GtkWidget *w, XstConnection *cxn);
static gint on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, XstConnection *cxn);
static void on_connection_modified (GtkWidget *w, XstConnection *cxn);
static void on_wvlan_adhoc_toggled (GtkWidget *w, XstConnection *cxn);
static void on_ppp_update_dns_toggled (GtkWidget *w, XstConnection *cxn);
static gboolean on_ip_address_focus_out (GtkWidget *widget, GdkEventFocus *event, XstConnection *cxn);

#define W(s) my_get_widget (cxn->xml, (s))

#define GET_STR(yy_prefix,xx) g_free (cxn->xx); cxn->xx = gtk_editable_get_chars (GTK_EDITABLE (W (yy_prefix#xx)), 0, -1)
#define GET_BOOL(yy_prefix,xx) cxn->xx = GTK_TOGGLE_BUTTON (W (yy_prefix#xx))->active
#define GET_BOOL_NOT(yy_prefix,xx) GET_BOOL(yy_prefix,xx); cxn->xx = !cxn->xx;
#define SET_STR(yy_prefix,xx) xst_ui_entry_set_text (GTK_ENTRY (W (yy_prefix#xx)), cxn->xx)
#define SET_BOOL(yy_prefix,xx) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W (yy_prefix#xx)), cxn->xx)
#define SET_BOOL_NOT(yy_prefix,xx) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W (yy_prefix#xx)), !cxn->xx)
	

static GdkPixmap *mini_pm[XST_CONNECTION_LAST];
static GdkBitmap *mini_mask[XST_CONNECTION_LAST];

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

static gint
my_strcmp (gchar *a, gchar *b)
{
	if (!a)
		a = "";

	if (!b)
		b = "";

	return strcmp (a, b);
}

static gboolean
connection_xml_get_boolean (xmlNode *node, gchar *elem)
{
	gchar *s;
	gboolean ret;

	ret = FALSE;

	s = xst_xml_get_child_content (node, elem);
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

	str = xst_xml_get_child_content (node, "type");
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

	section_found = xst_xml_get_child_content (node, "name");
	if (section_found) {
		cmp = strcmp (section_found, name);
		g_free (section_found);
		return !cmp;
	}

	return 0;
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

	inherits = xst_xml_get_child_content (node, "inherits");
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
		value = xst_xml_get_child_content (subnode, elem);
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

extern gchar *
connection_wvsection_name_generate (gchar *dev, xmlNode *root)
{
	gchar *str;
	gint i;

	i = 0;
	str = g_strdup (dev);
	while (connection_xml_wvsection_search (root, str, NULL))
	{
		g_free (str);
		str = g_strdup_printf ("%s_%d", dev, ++i);
	}

	return str;
}

/* dev_type is "eth", "wvlan", "ppp", "plip"... */
static gchar *
connection_dev_get_next (xmlNode *root, gchar *dev_type)
{
	xmlNode *node;
	gchar *dev;
	gint len, max, num;

	len = strlen (dev_type);
	max = 0;

	for (node = xst_xml_element_find_first (root, "interface");
		node; node = xst_xml_element_find_next (node, "interface"))
	{
		dev = xst_xml_get_child_content (node, "dev");

		g_return_val_if_fail (dev != NULL, NULL);
		g_return_val_if_fail (dev_type != NULL, NULL);
		
		if (strstr (dev, dev_type)) {
			num = atoi (dev + len) + 1;
			max = (num > max)? num: max;
		}
		g_free (dev);
	}

	return g_strdup_printf ("%s%d", dev_type, max);
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
connection_set_modified (XstConnection *cxn, gboolean state)
{
	if (cxn->frozen || !xst_tool_get_access (tool))
		return;

	cxn->modified = state;
}

static void
load_icon (const gchar *file, GdkPixmap **pixmap, GdkBitmap **mask)
{
	GdkPixbuf *pb, *pb2;
	char *path;

	path = g_concat_dir_and_file (PIXMAPS_DIR, file);
	pb = gdk_pixbuf_new_from_file (path);

	if (!pb) {
		g_warning ("Could not load pixmap %s\n", path);
		g_free (path);
		return;
	}
	g_free (path);
	
	pb2 = gdk_pixbuf_scale_simple (pb, 16, 16, GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (pb);
	
	gdk_pixbuf_render_pixmap_and_mask (pb2, pixmap, mask, 127);
	gdk_pixbuf_unref (pb2);
}

static gchar *
connection_get_cell_text (const GtkCListRow *row, gint col)
{
	return row->cell[col].u.text;
}

static gint
connection_clist_cmp (GtkCList *clist, gconstpointer p1, gconstpointer p2)
{
	gint res;

	/* Compare dev cols */
	res = my_strcmp (connection_get_cell_text (p1, 0),
			 connection_get_cell_text (p2, 0));
	if (res)
		return res;

	/* Compare comment cols */
	return my_strcmp (connection_get_cell_text (p1, 2),
			  connection_get_cell_text (p2, 2));
}

static void
connection_init_clist (XstTool *tool)
{
	GtkWidget *clist;
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	gtk_clist_set_compare_func (GTK_CLIST (clist), connection_clist_cmp);
}

extern void
connection_init_gui (XstTool *tool)
{
	XstConnectionType i;

	for (i = XST_CONNECTION_OTHER; i < XST_CONNECTION_LAST; i++)
		load_icon (xst_iface_desc[i].icon, &mini_pm[i], &mini_mask[i]);

	load_icon ("gnome-light-off.png" /* "connection-inactive.xpm" */,
		   &active_pm[0], &active_mask[0]);
	load_icon ("gnome-light-on.png" /*"connection-active.xpm" */,
		   &active_pm[1], &active_mask[1]);

	connection_init_clist (tool);
}

void
connection_set_row_pixtext (GtkWidget *clist, gint row, gchar *text, gboolean enabled)
{
	g_return_if_fail (GTK_IS_CLIST (clist));
	g_return_if_fail (text != NULL);
	
	gtk_clist_set_pixtext (GTK_CLIST (clist), row, 1,
			       text,
			       GNOME_PAD_SMALL,
			       active_pm[enabled ? 1 : 0], 
			       active_mask[enabled ? 1 : 0]);
}

/* NULL if false, else GtkWidget in data in found node */
static GtkWidget *
connection_default_gw_find_item (GtkWidget *omenu, gchar *dev)
{
	GList *l;
	gchar *value;

	for (l = gtk_object_get_data (GTK_OBJECT (omenu), "list");
	     l; l = l->next) {
		value = gtk_object_get_data (GTK_OBJECT (l->data), "value");
		if (!strcmp (dev, value))
			return l->data;
	}

	return NULL;
}

static void
connection_default_gw_activate (GtkMenuItem *item, gpointer data)
{
	gtk_object_set_data (GTK_OBJECT (tool), "gatewaydev", data);
}

void
connection_default_gw_add (gchar *dev)
{
	GtkWidget *omenu, *menu, *item;
	GList *l;
	gchar *cpy;

	if (!strcmp (dev, "lo"))
		return;
	
	omenu = xst_dialog_get_widget (tool->main_dialog, "connection_def_gw_omenu");
	menu  = gtk_option_menu_get_menu (GTK_OPTION_MENU (omenu));

	if (connection_default_gw_find_item (omenu, dev))
		return;
	
	cpy = g_strdup (dev);
	item = gtk_menu_item_new_with_label (dev);
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (connection_default_gw_activate),
			    (gpointer) cpy);
	gtk_object_set_data (GTK_OBJECT (item), "value", cpy);
	gtk_widget_show (item);
	gtk_menu_append (GTK_MENU (menu), item);
	
	l = gtk_object_get_data (GTK_OBJECT (omenu), "list");
	l = g_list_append (l, item);
	gtk_object_set_data (GTK_OBJECT (omenu), "list", l);
}

void
connection_default_gw_remove (gchar *dev)
{
	GtkWidget *omenu, *menu, *item;
	GList *l;
	gchar *cpy;
	
	omenu = xst_dialog_get_widget (tool->main_dialog, "connection_def_gw_omenu");
	menu  = gtk_option_menu_get_menu (GTK_OPTION_MENU (omenu));

	g_return_if_fail ((item = connection_default_gw_find_item (omenu, dev)));
	
	l = gtk_object_get_data (GTK_OBJECT (omenu), "list");
	l = g_list_remove (l, item);
	gtk_object_set_data (GTK_OBJECT (omenu), "list", l);

	cpy = gtk_object_get_data (GTK_OBJECT (item), "value");
	g_free (cpy);
	
	gtk_widget_destroy (item);
}

void
connection_default_gw_init (XstTool *tool, gchar *dev)
{
	GtkWidget *omenu, *menu, *item;

	omenu = xst_dialog_get_widget (tool->main_dialog, "connection_def_gw_omenu");
	menu  = gtk_option_menu_get_menu (GTK_OPTION_MENU (omenu));

	item = gtk_menu_get_active (GTK_MENU (menu));
	gtk_signal_connect (GTK_OBJECT (item), "activate",
			    GTK_SIGNAL_FUNC (connection_default_gw_activate),
			    (gpointer) NULL);
	/* Strange bug caused the "Auto" item to be unsensitive the first time. */
	gtk_menu_shell_select_item (GTK_MENU_SHELL (menu), item);
	
	item = connection_default_gw_find_item (omenu, dev);
	if (item) {
		GList *l;

		l = gtk_object_get_data (GTK_OBJECT (omenu), "list");
		
		gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), g_list_index (l, item) + 1);
		gtk_menu_shell_select_item (GTK_MENU_SHELL (menu), item);
	}
}

XstConnection *
connection_default_gw_get_connection (XstTool *tool)
{
	gchar *dev;

	dev = gtk_object_get_data (GTK_OBJECT (tool), "gatewaydev");
	g_return_val_if_fail (dev != NULL, NULL);

	return connection_find_by_dev (tool, dev);
}

XstConnectionErrorType
connection_default_gw_check_manual (XstConnection *cxn, gboolean ignore_enabled)
{
	g_return_val_if_fail (cxn != NULL, XST_CONNECTION_ERROR_OTHER);

	if (!ignore_enabled && !cxn->enabled)
		return XST_CONNECTION_ERROR_ENABLED;

	switch (cxn->type) {
	case XST_CONNECTION_ETH:
	case XST_CONNECTION_WVLAN:
	case XST_CONNECTION_IRLAN:
		if (cxn->ip_config == IP_MANUAL && (!cxn->gateway || !*cxn->gateway))
			return XST_CONNECTION_ERROR_STATIC;
		break;
	case XST_CONNECTION_PPP:
		if (!cxn->set_default_gw)
			return XST_CONNECTION_ERROR_PPP;
		break;
	case XST_CONNECTION_PLIP:
		if (!cxn->remote_address || !*cxn->remote_address)
			return XST_CONNECTION_ERROR_STATIC;
		break;
	case XST_CONNECTION_LO:
	default:
		g_warning ("connection_default_gw_check_manual: shouldn't be here.");
		return XST_CONNECTION_ERROR_OTHER;
	}

	return XST_CONNECTION_ERROR_NONE;
}

void
connection_default_gw_fix (XstConnection *cxn, XstConnectionErrorType error)
{
	switch (error) {
	case XST_CONNECTION_ERROR_ENABLED:
		cxn->enabled = TRUE;
		break;
	case XST_CONNECTION_ERROR_PPP:
		cxn->set_default_gw = TRUE;
		break;
	case XST_CONNECTION_ERROR_STATIC:
	case XST_CONNECTION_ERROR_NONE:
	case XST_CONNECTION_ERROR_OTHER:
	default:
		g_warning ("connection_default_gw_fix: shouldn't be here.");
	}
}

void
connection_default_gw_set_manual (XstTool *tool, XstConnection *cxn)
{
	gchar *gateway;

	gateway = gtk_object_get_data (GTK_OBJECT (tool), "gateway");
	if (gateway)
		g_free (gateway);
	gateway = NULL;

	if (!cxn) {
		gtk_object_set_data (GTK_OBJECT (tool), "gateway", NULL);
		gtk_object_set_data (GTK_OBJECT (tool), "gatewaydev", NULL);
		return;
	}

	switch (cxn->type) {
	case XST_CONNECTION_ETH:
	case XST_CONNECTION_WVLAN:
	case XST_CONNECTION_IRLAN:
		if (cxn->ip_config == IP_MANUAL)
			gateway = g_strdup (cxn->gateway);
		break;
	case XST_CONNECTION_PLIP:
		gateway = strdup (cxn->remote_address);
		break;
	case XST_CONNECTION_PPP:
		gtk_object_set_data (GTK_OBJECT (tool), "gatewaydev", NULL);
		break;
	case XST_CONNECTION_LO:
	default:
		gtk_object_set_data (GTK_OBJECT (tool), "gatewaydev", NULL);
		g_warning ("connection_default_gw_set_manual: shouldn't be here.");
		break;
	}

	gtk_object_set_data (GTK_OBJECT (tool), "gateway", gateway);
}

static gboolean
connection_type_is_lan (XstConnectionType type)
{
	return (type == XST_CONNECTION_ETH ||
		type == XST_CONNECTION_WVLAN ||
		type == XST_CONNECTION_IRLAN);

}

static XstConnection *
connection_default_gw_find_static (XstTool *tool)
{
	XstConnection *cxn;
	GtkWidget *clist;
	int i;
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	for (i=0; i < GTK_CLIST (clist)->rows; i++) {
		cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i);

		/* Try fo find an active ethernet or wavelan connection with a static gateway definded */
		if (cxn->enabled && connection_type_is_lan (cxn->type) &&
		    (cxn->ip_config == IP_MANUAL) &&
		    (cxn->gateway && *cxn->gateway))
			return cxn;
	}

	return NULL;
}

static XstConnection *
connection_default_gw_find_ppp (XstTool *tool)
{
	XstConnection *cxn;
	GtkWidget *clist;
	int i;
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	for (i=0; i < GTK_CLIST (clist)->rows; i++) {
		cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i);

		/* Try fo find an active PPP connection with the default_gw bit on. */
		if (cxn->enabled &&
		    cxn->type == XST_CONNECTION_PPP &&
		    cxn->set_default_gw)
			return cxn;
	}
	
	return NULL;
}

static XstConnection *
connection_default_gw_find_dynamic (XstTool *tool)
{
	XstConnection *cxn;
	GtkWidget *clist;
	int i;
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	for (i=0; i < GTK_CLIST (clist)->rows; i++) {
		cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i);
		/* Try fo find an active ethernet or wavelan connection with dynamic configuration */
		if (cxn->enabled &&
		    (cxn->type == XST_CONNECTION_ETH || cxn->type == XST_CONNECTION_WVLAN) &&
		    (cxn->ip_config != IP_MANUAL))
			return cxn;
	}
	
	return NULL;
}

static XstConnection *
connection_default_gw_find_plip (XstTool *tool)
{
	XstConnection *cxn;
	GtkWidget *clist;
	int i;
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	for (i=0; i < GTK_CLIST (clist)->rows; i++) {
		cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i);

		/* Try fo find an active plip connection with a set remote address. */
		if (cxn->enabled &&
		    (cxn->type == XST_CONNECTION_PLIP) &&
		    (cxn->remote_address && *cxn->remote_address))
			return cxn;
	}

	return NULL;
}

void
connection_default_gw_set_auto (XstTool *tool)
{
	XstConnection *cxn;

	if (!(cxn = connection_default_gw_find_static (tool))  &&
	    !(cxn = connection_default_gw_find_ppp (tool))     &&
	    !(cxn = connection_default_gw_find_dynamic (tool)) &&
	    !(cxn = connection_default_gw_find_plip (tool)))
		cxn = NULL;

	connection_default_gw_set_manual (tool, cxn);
}

void
connection_update_clist_enabled_apply (GtkWidget *clist)
{
	gint i;
	XstConnection *cxn;

	g_return_if_fail (GTK_IS_CLIST (clist));

	for (i = 0; i < GTK_CLIST (clist)->rows; i++) {
		cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i);
		g_return_if_fail (cxn != NULL);
		connection_set_row_pixtext (clist, i, cxn->enabled ? _("Active") :
					    _("Inactive"), cxn->enabled);
	}
}

void
connection_update_row_enabled (XstConnection *cxn, gboolean enabled)
{
	XstConnection *cxn2;
	GtkWidget *clist;
	gint row, i;

	g_return_if_fail (cxn != NULL);
	g_return_if_fail (cxn->dev != NULL);
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");

	if (enabled)
		for (i = 0; i < GTK_CLIST (clist)->rows; i++) {
			cxn2 = gtk_clist_get_row_data (GTK_CLIST (clist), i);

			g_return_if_fail (cxn2 != NULL);
			g_return_if_fail (cxn2->dev != NULL);
			
			if (!strcmp (cxn2->dev, cxn->dev))
				connection_update_row_enabled (cxn2, FALSE);
		}

	cxn->enabled = enabled;
	row = gtk_clist_find_row_from_data (GTK_CLIST (clist), cxn);
	g_return_if_fail (row > -1);
	connection_set_row_pixtext (clist, row, enabled ? _("Active") :
				    _("Inactive"), enabled);
	/*xst_dialog_modify (tool->main_dialog);*/
}

void
connection_update_row (XstConnection *cxn)
{
	GtkWidget *clist;
	gint row;

	g_return_if_fail (cxn != NULL);
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");

	row = gtk_clist_find_row_from_data (GTK_CLIST (clist), cxn);

	g_return_if_fail (row > -1);
	g_return_if_fail (GTK_IS_CLIST (clist));
	g_return_if_fail (cxn->dev != NULL);

	gtk_clist_set_pixtext (GTK_CLIST (clist), row, 0, cxn->dev, GNOME_PAD_SMALL,
			       mini_pm[cxn->type], mini_mask[cxn->type]);

	gtk_clist_set_text (GTK_CLIST (clist), row, 2, cxn->name ? cxn->name : "");
	gtk_clist_sort (GTK_CLIST (clist));

	xst_dialog_modify (tool->main_dialog);
}

void
connection_add_to_list (XstConnection *cxn, GtkWidget *clist)
{
	int row;
	char *text[3] = { "Error", NULL };

	g_return_if_fail (GTK_IS_CLIST (clist));
	g_return_if_fail (cxn != NULL);

	row = gtk_clist_append (GTK_CLIST (clist), text);
	gtk_clist_set_row_data (GTK_CLIST (clist), row, cxn);

	connection_default_gw_add (cxn->dev);
	connection_update_row (cxn);
	connection_update_row_enabled (cxn, cxn->enabled);

	return;
}

XstConnection *
connection_find_by_dev (XstTool *tool, gchar *dev)
{
	XstConnection *cxn;
	GtkWidget *clist;
	int i;
	
	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	for (i=0; i < GTK_CLIST (clist)->rows; i++) {
		cxn = gtk_clist_get_row_data (GTK_CLIST (clist), i);
		if (!strcmp (cxn->dev, dev))
			return cxn;
	}

	return NULL;
}


/*static void
add_connections_to_list (void)
{
	g_slist_foreach (connections, (GFunc) add_connection_to_list, NULL);
}*/

static gchar *
connection_description_from_type (XstConnectionType type)
{
	gint i;

	for (i = 0; xst_iface_desc[i].type != XST_CONNECTION_UNKNOWN; i++)
		if (type == xst_iface_desc[i].type)
			break;

	return g_strdup (_(xst_iface_desc[i].description));
}

extern gchar *
connection_get_serial_port_from_node (xmlNode *node, gchar *wvsection)
{
	return connection_xml_wvsection_get_str (node, wvsection, "device");
}

static void
connection_get_ppp_from_node (xmlNode *node, XstConnection *cxn)
{
	cxn->wvsection = xst_xml_get_child_content (node, "wvsection");
	if (cxn->wvsection) {
		cxn->serial_port = connection_get_serial_port_from_node (node->parent, cxn->wvsection);
		cxn->phone_number = connection_xml_wvsection_get_str (node->parent, cxn->wvsection, "phone");
		cxn->login = connection_xml_wvsection_get_str (node->parent, cxn->wvsection, "login");
		cxn->password = connection_xml_wvsection_get_str (node->parent, cxn->wvsection, "password");
		cxn->stupid = connection_xml_wvsection_get_boolean (node->parent, cxn->wvsection, "stupid");
	} else {
		cxn->wvsection = connection_wvsection_name_generate (cxn->dev, node->parent);
		connection_xml_save_str_to_node (cxn->node, "wvsection", cxn->wvsection);

		cxn->phone_number = xst_xml_get_child_content (node, "phone_number");
		cxn->login = xst_xml_get_child_content (node, "login");
		cxn->password = xst_xml_get_child_content (node, "password");
		cxn->stupid = FALSE;
	}

	/* PPP advanced */
	cxn->persist = connection_xml_get_boolean (node, "persist");
	cxn->serial_port = xst_xml_get_child_content (node, "serial_port");
	cxn->set_default_gw = connection_xml_get_boolean (node, "set_default_gw");
	cxn->dns1 = xst_xml_get_child_content (node, "dns1");
	cxn->dns2 = xst_xml_get_child_content (node, "dns2");
	cxn->ppp_options = xst_xml_get_child_content (node, "ppp_options");
}

static void
connection_get_ptp_from_node (xmlNode *node, XstConnection *cxn)
{
	cxn->remote_address = xst_xml_get_child_content (node, "remote_address");
}
	
XstConnection *
connection_new_from_type (XstConnectionType type, xmlNode *root)
{
	XstConnection *cxn;

	cxn = g_new0 (XstConnection, 1);
	cxn->type = type;
	cxn->file = NULL;
	
	/* set up some defaults */
	cxn->user = FALSE;
	cxn->autoboot = TRUE;
	cxn->update_dns = TRUE;
	cxn->ip_config = cxn->tmp_ip_config = IP_MANUAL;
	
        /* FIXME: figure out a new device correctly */
	switch (cxn->type) {
	case XST_CONNECTION_ETH:
		cxn->dev = g_strdup ("eth0");
		break;
	case XST_CONNECTION_WVLAN:
		cxn->dev = g_strdup ("wvlan0");
		break;
	case XST_CONNECTION_IRLAN:
		cxn->dev = g_strdup ("irlan0");
		break;
	case XST_CONNECTION_PPP:
		cxn->user = TRUE;
		cxn->autoboot = FALSE;
		cxn->dev = connection_dev_get_next (root, "ppp");
		break;
	case XST_CONNECTION_LO:
		cxn->dev = g_strdup ("lo");
		break;
	case XST_CONNECTION_PLIP:
		cxn->autoboot = FALSE;
		cxn->dev = g_strdup ("plip0");
		break;
	default:
		cxn->dev = g_strdup ("NIL");
		break;
	}	

	cxn->node = NULL;

	return cxn;
}

XstConnection *
connection_new_from_node (xmlNode *node)
{
	XstConnection *cxn;
	char *s = NULL;

	s = xst_xml_get_child_content (node, "dev");

	if (s) {
		cxn = connection_new_from_dev_name (s, node->parent);
		g_free (cxn->dev);
		cxn->dev = s;
	} else {
		cxn = connection_new_from_type (XST_CONNECTION_OTHER, node->parent);
	}

	cxn->node = node;

	s = xst_xml_get_child_content (node, "file");
	if (s)
		cxn->file = s;
	
	s = xst_xml_get_child_content (node, "bootproto");
	if (s) {
		cxn->ip_config = cxn->tmp_ip_config = connection_config_type_from_str (s);
		g_free (s);
	}

	s = xst_xml_get_child_content (node, "name");
	if (s)
		cxn->name = s;
	else
		cxn->name = connection_description_from_type (cxn->type);
	
	/* Activation */
	cxn->user = connection_xml_get_boolean (node, "user");
	cxn->autoboot = connection_xml_get_boolean (node, "auto");
	cxn->enabled = connection_xml_get_boolean (node, "enabled");
	cxn->update_dns = connection_xml_get_boolean (node, "update_dns");

	/* TCP/IP general paramaters */
	cxn->address = xst_xml_get_child_content (node, "address");
	cxn->netmask = xst_xml_get_child_content (node, "netmask");
	
	cxn->gateway = xst_xml_get_child_content (node, "gateway");
	if (!cxn->gateway || !*cxn->gateway) {
		g_free (cxn->gateway);
		cxn->gateway = xst_xml_get_child_content (node->parent, "gateway");
	}

	switch (cxn->type) {
	case XST_CONNECTION_PPP:
		connection_get_ppp_from_node (cxn->node, cxn);
		break;
	case XST_CONNECTION_PLIP:
		connection_get_ptp_from_node (cxn->node, cxn);
		break;
	default:
		break;
	}

	connection_add_to_list (cxn, xst_dialog_get_widget (tool->main_dialog, "connection_list"));
	connection_update_row (cxn);
	connection_update_row_enabled (cxn, cxn->enabled);

	return cxn;
}

XstConnection *
connection_new_from_dev_name (char *dev_name, xmlNode *root)
{
	int i;

	for (i = 0; xst_iface_desc[i].name; i++)
		if (strstr (dev_name, xst_iface_desc[i].name) == dev_name)
			break;

	return connection_new_from_type (xst_iface_desc[i].type, root);
}

void
connection_free (XstConnection *cxn)
{
	if (cxn->node)
		xst_xml_element_destroy (cxn->node);

	g_free (cxn->dev);
	g_free (cxn->name);

	g_free (cxn->address);
	g_free (cxn->netmask);
	g_free (cxn->broadcast);
	g_free (cxn->network);
	g_free (cxn->gateway);

	g_free (cxn->session_id);

	g_free (cxn->phone_number);
	g_free (cxn->login);
	g_free (cxn->password);
	g_free (cxn->serial_port);
	g_free (cxn->wvsection);
	g_free (cxn->dns1);
	g_free (cxn->dns2);
	g_free (cxn->ppp_options);

	g_free (cxn->remote_address);
}

/* This function may come handy again later. */
#if 0
static void
update_status (XstConnection *cxn)
{
	gnome_pixmap_load_file (GNOME_PIXMAP (W ("status_icon")),
				GTK_TOGGLE_BUTTON (W ("status_enabled"))->active
				? PIXMAPS_DIR "/gnome-light-on.png"
				: PIXMAPS_DIR "/gnome-light-off.png");
}

static void
on_status_enabled_toggled (GtkWidget *w, XstConnection *cxn)
{
	connection_set_modified (cxn, TRUE);
/*	update_status (cxn);*/
}
#endif

static void
connection_check_netmask_gui (XstConnection *cxn)
{
        GtkWidget *netmask_widget, *address_widget;
        gchar *address, *netmask;
        guint32 ip1;

	netmask_widget = W ("ip_netmask");
	address_widget = W ("ip_address");
        address = gtk_editable_get_chars (GTK_EDITABLE (address_widget), 0, -1);
        netmask = gtk_editable_get_chars (GTK_EDITABLE (netmask_widget), 0, -1);

        g_print ("ip: %s\n", address);
        g_print ("netmask: %s\n", netmask);

        if ((sscanf (address, "%d.", &ip1) == 1) && (!strlen (netmask))) {
		if (ip1 < 127) {
			gtk_entry_set_text (GTK_ENTRY (netmask_widget), "255.0.0.0");
		} else if (ip1 < 192) {
			gtk_entry_set_text (GTK_ENTRY (netmask_widget), "255.255.0.0");
		} else {
			gtk_entry_set_text (GTK_ENTRY (netmask_widget), "255.255.255.0");
		}
	}
}

static gchar *
connection_str_to_addr (gchar *str)
{
	gchar **strv;
	gchar *addr;
	gint i;

	strv = g_strsplit (str, ".", 4);
	addr = g_new0 (gchar, 4);
	
	for (i = 0; strv[i]; i++)
		addr[i] = (gchar) atoi (strv[i]);

	g_strfreev (strv);
	
	return addr;
}

static gchar *
connection_addr_to_str (gchar *addr)
{
	gchar *str;

	str = g_strdup_printf ("%u.%u.%u.%u",
			       (guchar) addr[0],
			       (guchar) addr[1],
			       (guchar) addr[2],
			       (guchar) addr[3]);
	return str;
}

static void
connection_set_bcast_and_network (XstConnection *cxn)
{
	gchar *address, *netmask;
	gchar *broadcast;
	gchar *network;
	gint i;

	if (!cxn->address || !*cxn->address ||
	    !cxn->netmask || !*cxn->netmask)
		return;
	
	address = connection_str_to_addr (cxn->address);
	netmask = connection_str_to_addr (cxn->netmask);
	broadcast = g_new0 (gchar, 4);
	network   = g_new0 (gchar, 4);

	for (i = 0; i < 4; i++) {
		broadcast[i] = (gchar) (address[i] | (~netmask[i]));
		network[i]   = (gchar) (address[i] & netmask[i]);
	}

	if (cxn->broadcast)
		g_free (cxn->broadcast);
	if (cxn->network)
		g_free (cxn->network);

	cxn->broadcast = connection_addr_to_str (broadcast);
	cxn->network   = connection_addr_to_str (network);

	g_free (address);
	g_free (netmask);
	g_free (broadcast);
	g_free (network);
}

static void
empty_general (XstConnection *cxn)
{
	GET_STR ("connection_", name);
	GET_BOOL ("status_", autoboot);
	GET_BOOL ("status_", user);
/*	GET_BOOL ("status_", enabled);*/
}

static void
empty_ip (XstConnection *cxn)
{
	connection_check_netmask_gui (cxn);
	
	cxn->ip_config = cxn->tmp_ip_config;
	GET_BOOL ("ip_", update_dns);
	GET_STR ("ip_", address);
	GET_STR ("ip_", netmask);
	GET_STR ("ip_", gateway);
}

static void
empty_wvlan (XstConnection *cxn)
{
}

static void
empty_ppp (XstConnection *cxn)
{
	GET_STR ("ppp_", phone_number);
	GET_STR ("ppp_", login);
	GET_STR ("ppp_", password);
	GET_BOOL ("ppp_", persist);
}

static void
empty_ppp_adv (XstConnection *cxn)
{
	GET_STR ("ppp_", serial_port);
	GET_BOOL ("ppp_", stupid);
	GET_BOOL ("ppp_", set_default_gw);
	GET_BOOL_NOT ("ppp_", update_dns);
	GET_STR ("ppp_", dns1);
	GET_STR ("ppp_", dns2);
	GET_STR ("ppp_", ppp_options);
}

static void
empty_ptp (XstConnection *cxn)
{
	GET_STR ("ptp_", address);
	GET_STR ("ptp_", remote_address);

	g_free (cxn->gateway);
	if (GTK_TOGGLE_BUTTON (W ("ptp_remote_is_gateway"))->active) {
		cxn->gateway = g_strdup (cxn->remote_address);
	} else {
		cxn->gateway = NULL;
	}
}

static gboolean
strempty (gchar *str)
{
	return (!str || !*str);
}

static gboolean
connection_validate (XstConnection *cxn)
{
	gchar *error = NULL;
	
	switch (cxn->type) {
	case XST_CONNECTION_ETH:
	case XST_CONNECTION_WVLAN:
	case XST_CONNECTION_IRLAN:
	case XST_CONNECTION_UNKNOWN:
		if (cxn->ip_config == IP_MANUAL &&
		    (strempty (cxn->address) ||
		     strempty (cxn->netmask)))
			error = _("The IP address or netmask for the interface\n"
				  "was left empty. Please enter valid IP\n"
				  "addresses in those fields to continue.");
		break;
	case XST_CONNECTION_PLIP:
		if (strempty (cxn->address) ||
		    strempty (cxn->remote_address))
			error = _("The IP address or remote address for the\n"
				  "interface was left empty. Please enter valid\n"
				  "IP addresses in those fields to continue.");
		break;
	case XST_CONNECTION_PPP:
		if (!cxn->update_dns && strempty (cxn->dns1))
			error = _("You chose to set the DNS servers for this\n"
				  "connection manually, but left the IP\n"
				  "address for the primary DNS empty. Please\n"
				  "enter the IP for the primary DNS or uncheck\n"
				  "the manual DNS option.");
		break;
	default:
		break;
	}

	if (error) {
		GtkWidget *message;
		
		message = gnome_message_box_new (error, GNOME_MESSAGE_BOX_WARNING,
						 GNOME_STOCK_BUTTON_OK,
						 NULL);
		if (cxn->window)
			gnome_dialog_set_parent (GNOME_DIALOG (message), GTK_WINDOW (cxn->window));
		gnome_dialog_run_and_close (GNOME_DIALOG (message));
		return FALSE;
	}

	return TRUE;
}

static void
connection_empty_gui (XstConnection *cxn)
{
	empty_general (cxn);

	switch (cxn->type) {
	case XST_CONNECTION_WVLAN:
		empty_wvlan (cxn);
		empty_ip (cxn);
		break;
	case XST_CONNECTION_PPP:
		empty_ppp (cxn);
		empty_ppp_adv (cxn);
		break;
	case XST_CONNECTION_PLIP:
		empty_ptp (cxn);
		break;
	case XST_CONNECTION_ETH:
	case XST_CONNECTION_IRLAN:
	default:
		empty_ip (cxn);
		break;
	}
}	

static gboolean
connection_config_save (XstConnection *cxn)
{
	XstConnection *tmp = g_new0 (XstConnection, 1);

	tmp->window = cxn->window;
	tmp->xml = cxn->xml;
	tmp->type = cxn->type;
	tmp->modified = cxn->modified;
	tmp->creating = cxn->creating;
	tmp->frozen = cxn->frozen;
	tmp->ip_config = cxn->ip_config;
	tmp->tmp_ip_config = cxn->tmp_ip_config;
	
	connection_empty_gui (tmp);
	if (!connection_validate (tmp)) {
		connection_free (tmp);
		return FALSE;
	}

	connection_empty_gui (cxn);
	connection_set_modified (cxn, FALSE);
	connection_update_row (cxn);

	return TRUE;
}

static void
on_connection_ok_clicked (GtkWidget *w, XstConnection *cxn)
{
	if (cxn->modified) {
		if (connection_config_save (cxn))
			gtk_widget_destroy (cxn->window);
		cxn->creating = FALSE;
	} else
		gtk_widget_destroy (cxn->window);
}

static void
on_connection_cancel_clicked (GtkWidget *w, XstConnection *cxn)
{
	GtkCList *list;
	gint row;

	gtk_widget_destroy (cxn->window);
	
	if (cxn->creating) {
		list = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "connection_list"));
		row = gtk_clist_find_row_from_data (list, cxn);
		g_return_if_fail (row > -1);
		gtk_clist_remove (list, row);
		connection_free (cxn);
		xst_dialog_modify (tool->main_dialog);
	}
}

static void
on_connection_config_dialog_destroy (GtkWidget *w, XstConnection *cxn)
{
	gtk_object_unref (GTK_OBJECT (cxn->xml));
	cxn->xml = NULL;

	cxn->window = NULL;
}

static gint
on_connection_config_dialog_delete_event (GtkWidget *w, GdkEvent *evt, XstConnection *cxn)
{
	return FALSE;
}

static void
on_connection_modified (GtkWidget *w, XstConnection *cxn)
{
	connection_set_modified (cxn, TRUE);
}

static void
on_wvlan_adhoc_toggled (GtkWidget *w, XstConnection *cxn)
{
/* FIXME: implement on_wvlan_adhoc_toggled*/
}

static void
on_ppp_update_dns_toggled (GtkWidget *w, XstConnection *cxn)
{
	gboolean active;

	active = GTK_TOGGLE_BUTTON (W ("ppp_update_dns"))->active;

	gtk_widget_set_sensitive (W ("ppp_dns1_label"), active);
	gtk_widget_set_sensitive (W ("ppp_dns2_label"), active);	
	gtk_widget_set_sensitive (W ("ppp_dns1"), active);
	gtk_widget_set_sensitive (W ("ppp_dns2"), active);
}

static gboolean
on_ip_address_focus_out (GtkWidget *widget, GdkEventFocus *event, XstConnection *cxn)
{
	connection_check_netmask_gui (cxn);

        return TRUE;
}

static void
fill_general (XstConnection *cxn)
{
	gtk_label_set_text (GTK_LABEL (W ("connection_dev")), cxn->dev);
	SET_STR ("connection_", name);
/*	SET_BOOL ("status_", enabled);
	update_status (cxn);*/
	SET_BOOL ("status_", autoboot);
	SET_BOOL ("status_", user);
}

static void
update_ip_config (XstConnection *cxn)
{
	IPConfigType ip;

	ip = cxn->tmp_ip_config;

	SET_BOOL ("ip_", update_dns);
	gtk_widget_set_sensitive (W ("ip_update_dns"), ip != IP_MANUAL);
	gtk_widget_set_sensitive (W ("ip_table"), ip == IP_MANUAL);
}

static void
ip_config_menu_cb (GtkWidget *w, gpointer data)
{
	XstConnection *cxn;
	IPConfigType ip;

	cxn = gtk_object_get_user_data (GTK_OBJECT (w));

	if (cxn->frozen)
		return;

	ip = GPOINTER_TO_INT (data);

	if (cxn->tmp_ip_config == ip)
		return;

	cxn->tmp_ip_config = GPOINTER_TO_INT (data);

	connection_set_modified (cxn, TRUE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("ip_update_dns")),
				      ip != IP_MANUAL);

	update_ip_config (cxn);
}

static void
fill_ip (XstConnection *cxn)
{
	GtkWidget *menu, *menuitem, *omenu;
	IPConfigType i;

	char *bootproto_labels[] = {
		N_("Manual"),
		N_("DHCP"),
		N_("BOOTP")
	};

	omenu = W ("connection_config");

	menu = gtk_menu_new ();
	for (i = 0; i < 3; i++) {
		menuitem = gtk_menu_item_new_with_label (_(bootproto_labels[i]));
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

	SET_BOOL ("ip_", update_dns);
	SET_STR ("ip_", address);
	SET_STR ("ip_", netmask);
	SET_STR ("ip_", gateway);
}

static void
fill_wvlan (XstConnection *cxn)
{

}

static void
fill_ppp (XstConnection *cxn)
{
	SET_STR ("ppp_", phone_number);
	SET_STR ("ppp_", login);
	SET_STR ("ppp_", password);
	SET_BOOL ("ppp_", persist);
}

static void
fill_ppp_adv (XstConnection *cxn)
{
	gnome_entry_load_history (GNOME_ENTRY (W ("ppp_serial_port_g")));
	SET_STR ("ppp_", serial_port);
	SET_BOOL ("ppp_", stupid);
	SET_BOOL ("ppp_", set_default_gw);
	SET_BOOL_NOT ("ppp_", update_dns);
	gtk_signal_emit_by_name (GTK_OBJECT (W ("ppp_update_dns")), "toggled");
	SET_STR ("ppp_", dns1);
	SET_STR ("ppp_", dns2);
	SET_STR ("ppp_", ppp_options);
}

static void
fill_ptp (XstConnection *cxn)
{
	gboolean state;
	
	SET_STR ("ptp_", address);
	SET_STR ("ptp_", remote_address);

	if (cxn->gateway && cxn->remote_address && !strcmp (cxn->gateway, cxn->remote_address))
		state = TRUE;
	else
		state = FALSE;
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("ptp_remote_is_gateway")), state);
}

static void
hookup_callbacks (XstConnection *cxn)
{
	gint i;
	WidgetSignal signals[] =
	{
		{ "on_connection_ok_clicked", on_connection_ok_clicked },
		{ "on_connection_cancel_clicked", on_connection_cancel_clicked },
		{ "on_connection_config_dialog_delete_event", on_connection_config_dialog_delete_event },
/*		{ "on_status_enabled_toggled", on_status_enabled_toggled },*/
		{ "on_connection_modified", on_connection_modified },
		{ "on_connection_config_dialog_destroy", on_connection_config_dialog_destroy },
		{ "on_wvlan_adhoc_toggled", on_wvlan_adhoc_toggled },
		{ "on_ppp_update_dns_toggled", on_ppp_update_dns_toggled },
		{ "on_ip_address_focus_out", on_ip_address_focus_out },
		{ NULL }
	};

	struct
	{
		char *name;
		EditableFilterRules rule;
	}
	s [] =
	{
		{ "ip_address",          EF_ALLOW_IP },
      		{ "ip_netmask",          EF_ALLOW_IP },
		{ "ip_gateway",          EF_ALLOW_IP },
		{ "ppp_dns1",            EF_ALLOW_IP },
		{ "ppp_dns2",            EF_ALLOW_IP },
		{ "ptp_address",         EF_ALLOW_IP },
		{ "ptp_remote_address",  EF_ALLOW_IP },
		{ NULL,                  EF_ALLOW_NONE }
	};

	for (i = 0; signals[i].hname; i++)
		glade_xml_signal_connect_data (cxn->xml, signals[i].hname,
					       signals[i].signalfunc, cxn);
	for (i = 0; s[i].name; i++)
		connect_editable_filter (W (s[i].name), s[i].rule);
}

void
connection_configure (XstConnection *cxn)
{
	GtkWidget *nb;
	gchar *s;

	if (cxn->window) {
		gtk_widget_show (cxn->window);
		return;
	}

	g_assert (!cxn->xml);

	cxn->frozen = TRUE;

	s = g_concat_dir_and_file (INTERFACES_DIR, "network.glade");
	cxn->xml = glade_xml_new (s, "connection_config_dialog");

	g_assert (cxn->xml);

	hookup_callbacks (cxn);

	cxn->window = W("connection_config_dialog");

	fill_general (cxn);
	fill_ip      (cxn);

	if (!xst_xml_element_get_boolean (cxn->node->parent, "smartdhcpcd"))
		gtk_widget_hide (W("ip_update_dns"));
	if (!xst_xml_element_get_boolean (cxn->node->parent, "userifacfectl"))
		gtk_widget_hide (W("status_user"));

	/* would like to do this as a switch */
	nb = W ("connection_nb");
	if (cxn->type == XST_CONNECTION_PPP) {
		xst_ui_image_set_pix (W ("connection_pixmap"), PIXMAPS_DIR "/ppp.png");
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
       
	if (cxn->type == XST_CONNECTION_WVLAN) {
		xst_ui_image_set_pix (W ("connection_pixmap"), PIXMAPS_DIR "/wavelan-48.png");
		fill_wvlan (cxn);
	} else
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
					  gtk_notebook_page_num (GTK_NOTEBOOK (nb),
								 W ("wvlan_vbox")));

	if (cxn->type == XST_CONNECTION_PLIP) {
		xst_ui_image_set_pix (W ("connection_pixmap"), PIXMAPS_DIR "/plip-48.png");
		fill_ptp (cxn);
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
					  gtk_notebook_page_num (GTK_NOTEBOOK (nb),
								 W ("ip_vbox")));
	} else 
		gtk_notebook_remove_page (GTK_NOTEBOOK (nb),
					  gtk_notebook_page_num (GTK_NOTEBOOK (nb),
								 W ("ptp_vbox")));

	if (cxn->type == XST_CONNECTION_ETH) {
		xst_ui_image_set_pix (W ("connection_pixmap"), PIXMAPS_DIR "/connection-ethernet.png");
	}

	if (cxn->type == XST_CONNECTION_IRLAN) {
		xst_ui_image_set_pix (W ("connection_pixmap"), PIXMAPS_DIR "/irda-48.png");
	}

	cxn->frozen = FALSE;

	connection_set_modified (cxn, FALSE);

	gtk_widget_show (cxn->window);
}

static gboolean
connection_updatedns_supported (XstConnection *cxn)
{
	switch (cxn->type) {
	case XST_CONNECTION_ETH:
	case XST_CONNECTION_WVLAN:
	case XST_CONNECTION_PPP:
	case XST_CONNECTION_PLIP:
	case XST_CONNECTION_IRLAN:
		return TRUE;
	case XST_CONNECTION_OTHER:
	case XST_CONNECTION_LO:
	case XST_CONNECTION_UNKNOWN:
	case XST_CONNECTION_LAST:
	default:
		return FALSE;
	}

	return FALSE;
}

void
connection_save_to_node (XstConnection *cxn, xmlNode *root)
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
	if (connection_updatedns_supported (cxn))
		connection_xml_save_boolean_to_node (node, "update_dns", cxn->update_dns);

	/* TCP/IP general paramaters */
	connection_set_bcast_and_network (cxn);
	connection_xml_save_str_to_node (node, "address", cxn->address);
	connection_xml_save_str_to_node (node, "netmask", cxn->netmask);
	connection_xml_save_str_to_node (node, "broadcast", cxn->broadcast);
	connection_xml_save_str_to_node (node, "network", cxn->network);
	connection_xml_save_str_to_node (node, "gateway", cxn->gateway);

	s = connection_config_type_to_str (cxn->ip_config);
	connection_xml_save_str_to_node (node, "bootproto", s);
	g_free (s);

	/* PPP stuff */
	if (cxn->type == XST_CONNECTION_PPP) {
		if (!cxn->wvsection)
			cxn->wvsection = connection_wvsection_name_generate (cxn->dev, root);
		connection_xml_save_str_to_node (node, "wvsection", cxn->wvsection);
		
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

	/* PtP */
	if (cxn->type == XST_CONNECTION_PLIP) {
		connection_xml_save_str_to_node (node, "remote_address", cxn->remote_address);
	}
}
