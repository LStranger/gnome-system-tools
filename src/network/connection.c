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
	const gchar       *description;
	XstConnectionType  type;
	const gchar       *icon;
	const gchar       *name;
	GdkPixbuf         *pixbuf;
};


enum {
	CONNECTION_LIST_COL_DEV_PIX,
	CONNECTION_LIST_COL_DEVICE,
	CONNECTION_LIST_COL_STAT_PIX,
	CONNECTION_LIST_COL_STATUS,
	CONNECTION_LIST_COL_DESCR,

	CONNECTION_LIST_COL_DATA,
	CONNECTION_LIST_COL_LAST
};

static XstNetworkInterfaceDescription xst_iface_desc [] = {
	{ N_("Other type"),                   XST_CONNECTION_OTHER,   "network.png",     "other_type", NULL },
	{ N_("Ethernet LAN card"),            XST_CONNECTION_ETH,     "16_ethernet.xpm", "eth",        NULL },
	{ N_("WaveLAN wireless LAN"),         XST_CONNECTION_WVLAN,   "wavelan-16.png",  "wvlan",      NULL },
	{ N_("PPP: modem or transfer cable"), XST_CONNECTION_PPP,     "16_ppp.xpm",      "ppp",        NULL },
	{ N_("Parallel line"),                XST_CONNECTION_PLIP,    "16_plip.xpm",     "plip",       NULL },
	{ N_("Infrared LAN"),                 XST_CONNECTION_IRLAN,   "irda-16.png",     "irlan",      NULL },
	{ N_("Loopback: virtual interface"),  XST_CONNECTION_LO,      "16_loopback.xpm", "lo",         NULL },
	{ N_("Unknown type"),                 XST_CONNECTION_UNKNOWN, "network.png",     NULL,         NULL },
	{ NULL,                               XST_CONNECTION_UNKNOWN, NULL,              NULL,         NULL  }
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

typedef struct {
	GtkWidget *list;
	GtkWidget *def_gw_omenu;
} XstConnectionUI;

#define CONNECTION_UI_STRING "connection_ui"

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
	     node; node = xst_xml_element_find_next (node, "dialing")) {
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

static GdkPixbuf *
load_pixbuf (const gchar *file)
{
	GdkPixbuf *pb, *pb2;
	gchar     *path;

	path = g_strconcat (PIXMAPS_DIR, "/", file, NULL);
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);

	pb2 = gdk_pixbuf_scale_simple (pb, 16, 16, GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (pb);

	return pb2;
}

static GdkPixbuf *
connection_get_dev_pixbuf (XstConnection *cxn)
{
	gint i;

	g_return_val_if_fail (cxn != NULL, NULL);

	for (i = 0; xst_iface_desc[i].description != NULL; i++)
		if (xst_iface_desc[i].type == cxn->type)
			return xst_iface_desc[i].pixbuf;

	return NULL;
}

static GdkPixbuf *
connection_get_stat_pixbuf (XstConnection *cxn)
{
	static GdkPixbuf *active;
	static GdkPixbuf *inactive;

	g_return_val_if_fail (cxn != NULL, NULL);

	if (active == NULL)
		active = load_pixbuf ("gnome-light-on.png");
	if (inactive == NULL)
		inactive = load_pixbuf ("gnome-light-off.png");

	return cxn->enabled ? active : inactive;
}

static const gchar *
connection_get_stat_string (XstConnection *cxn)
{
	g_return_val_if_fail (cxn != NULL, NULL);

	return cxn->enabled ? _("Active") : _("Inactive");
}

static GtkTreeModel *
connection_list_model_new (void)
{
	GtkListStore *store;

	store = gtk_list_store_new (CONNECTION_LIST_COL_LAST,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_POINTER);
	return GTK_TREE_MODEL (store);
}

static void
connection_list_add_columns (GtkTreeView *treeview)
{
	GtkCellRenderer   *text_renderer, *pixbuf_renderer;
	GtkTreeViewColumn *col;
	GtkTreeModel      *model = gtk_tree_view_get_model (treeview);

	/* Device */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title  (col, _("Device"));

	pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();

	gtk_tree_view_column_pack_start (col, pixbuf_renderer, FALSE);
	gtk_tree_view_column_add_attribute (col, pixbuf_renderer, "pixbuf", CONNECTION_LIST_COL_DEV_PIX);

	text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, text_renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, text_renderer, "text", CONNECTION_LIST_COL_DEVICE);

	gtk_tree_view_append_column (treeview, col);

	/* Status */
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title  (col, _("Status"));

	pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();

	gtk_tree_view_column_pack_start (col, pixbuf_renderer, FALSE);
	gtk_tree_view_column_add_attribute (col, pixbuf_renderer, "pixbuf", CONNECTION_LIST_COL_STAT_PIX);

	text_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, text_renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, text_renderer, "text", CONNECTION_LIST_COL_STATUS);

	gtk_tree_view_append_column (treeview, col);

	/* Description */
	text_renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Description"),
							text_renderer,
							"text",
							CONNECTION_LIST_COL_DESCR,
							NULL);
	gtk_tree_view_append_column (treeview, col);
}

static void
connection_list_select_row (GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter    iter;
	GtkTreeModel  *model;
	XstConnection *cxn;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, CONNECTION_LIST_COL_DATA, &cxn, -1);

		if (cxn->type == XST_CONNECTION_LO)
			connection_actions_set_sensitive (FALSE);
		else
			connection_actions_set_sensitive (TRUE);
	} else
		connection_actions_set_sensitive (FALSE);
}

static void
list_get_active_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, data, -1);
}

XstConnection *
connection_list_get_active (void)
{	
	XstConnectionUI  *ui;
	GtkTreeSelection *select;
	XstConnection    *cxn = NULL;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));
	gtk_tree_selection_selected_foreach (select, list_get_active_cb, &cxn);

	return cxn;
}

static gboolean
connection_iter (XstConnection *cxn, GtkTreeIter *iter)
{
	XstConnectionUI *ui;
	XstConnection   *c;
	GtkTreeModel    *model;
	gboolean         valid;

	g_return_val_if_fail (cxn != NULL, FALSE);

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, iter);
	while (valid) {
		gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, &c, -1);

		if (!strcmp (cxn->dev, c->dev))
			return TRUE;

		valid = gtk_tree_model_iter_next (model, iter);
	}

	return FALSE;
}

GtkWidget *
connection_list_new (void)
{
	GtkWidget        *treeview;
	GtkTreeSelection *select;
	GtkTreeModel     *model;

	model = connection_list_model_new ();

	treeview = gtk_tree_view_new_with_model (model);
	g_object_unref (G_OBJECT (model));

	connection_list_add_columns (GTK_TREE_VIEW (treeview));

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
			  G_CALLBACK (connection_list_select_row), NULL);

	gtk_widget_show_all (treeview);

	return treeview;
}

void
connection_list_append (XstConnection *cxn)
{
	XstConnectionUI *ui;
	GtkTreeModel    *model;
	GdkPixbuf       *pb;
	GtkTreeIter      iter;
	gboolean         exists;

	g_return_if_fail (cxn != NULL);

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	exists = connection_iter (cxn, &iter);
	if (!exists)
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    CONNECTION_LIST_COL_DEV_PIX,  connection_get_dev_pixbuf (cxn),
			    CONNECTION_LIST_COL_DEVICE,   cxn->dev,
			    CONNECTION_LIST_COL_STAT_PIX, connection_get_stat_pixbuf (cxn),
			    CONNECTION_LIST_COL_STATUS,   connection_get_stat_string (cxn),
			    CONNECTION_LIST_COL_DESCR,    cxn->name,
			    CONNECTION_LIST_COL_DATA,     cxn,
			    -1);
}

void
connection_list_remove (XstConnection *cxn)
{
	XstConnectionUI *ui;
	XstConnection   *c;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gboolean         valid;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, CONNECTION_LIST_COL_DATA, &c, -1);
		if (!strcmp (c->dev, cxn->dev)) {
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			break;
		}

		valid = gtk_tree_model_iter_next (model, &iter);
	}
}

void
connection_list_update (void)
{
	XstConnectionUI *ui;
	XstConnection   *cxn;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gboolean         valid;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, CONNECTION_LIST_COL_DATA, &cxn, -1);

		connection_list_append (cxn);

		valid = gtk_tree_model_iter_next (model, &iter);
	}
}

XstConnection *
connection_find_by_dev (GtkWidget *list, gchar *dev)
{
	XstConnection   *cxn;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gboolean         valid;

	g_return_val_if_fail (list != NULL, NULL);
	g_return_val_if_fail (GTK_IS_TREE_VIEW (list), NULL);
	g_return_val_if_fail (dev != NULL, NULL);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, CONNECTION_LIST_COL_DATA, &cxn, -1);

		if (!strcmp (cxn->dev, dev))
			return cxn;

		valid = gtk_tree_model_iter_next (model, &iter);
	}

	return NULL;
}

static gboolean
connection_type_is_lan (XstConnectionType type)
{
	return (type == XST_CONNECTION_ETH ||
		type == XST_CONNECTION_WVLAN ||
		type == XST_CONNECTION_IRLAN);

}

static gboolean
default_gw_find_static_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	XstConnection *cxn;

	gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, &cxn, -1);
	if (cxn->enabled && connection_type_is_lan (cxn->type) &&
	    (cxn->ip_config == IP_MANUAL) &&
	    (cxn->gateway && *cxn->gateway)) {
		* (XstConnection *)data = *cxn;
		return TRUE;
	}

	return FALSE;
}

static XstConnection *
connection_default_gw_find_static (XstTool *tool)
{
	XstConnection   *cxn;
	XstConnectionUI *ui;
	GtkTreeModel    *model;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	cxn = NULL;
	gtk_tree_model_foreach (model, default_gw_find_static_cb, &cxn);

	return cxn;
}

static gboolean
default_gw_find_ppp_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	XstConnection *cxn;

	gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, &cxn, -1);
	if (cxn->enabled && cxn->type == XST_CONNECTION_PPP && cxn->set_default_gw) {
		* (XstConnection *)data = *cxn;
		return TRUE;
	}

	return FALSE;
}

static XstConnection *
connection_default_gw_find_ppp (XstTool *tool)
{
	XstConnection   *cxn;
	XstConnectionUI *ui;
	GtkTreeModel    *model;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	cxn = NULL;
	gtk_tree_model_foreach (model, default_gw_find_ppp_cb, &cxn);

	return cxn;
}

static gboolean
default_gw_find_dynamic_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	XstConnection *cxn;

	gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, &cxn, -1);
	if (cxn->enabled &&
	    (cxn->type == XST_CONNECTION_ETH || cxn->type == XST_CONNECTION_WVLAN) &&
	    (cxn->ip_config != IP_MANUAL)) {
		* (XstConnection *)data = *cxn;
		return TRUE;
	}

	return FALSE;
}

static XstConnection *
connection_default_gw_find_dynamic (XstTool *tool)
{
	XstConnection   *cxn;
	XstConnectionUI *ui;
	GtkTreeModel    *model;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	cxn = NULL;
	gtk_tree_model_foreach (model, default_gw_find_dynamic_cb, &cxn);

	return cxn;
}

static gboolean
default_gw_find_plip_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	XstConnection *cxn;

	gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, &cxn, -1);
	if (cxn->enabled && (cxn->type == XST_CONNECTION_PLIP) &&
	    (cxn->remote_address && *cxn->remote_address)) {
		* (XstConnection *)data = *cxn;
		return TRUE;
	}

	return FALSE;
}

static XstConnection *
connection_default_gw_find_plip (XstTool *tool)
{
	XstConnection   *cxn;
	XstConnectionUI *ui;
	GtkTreeModel    *model;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	cxn = NULL;
	gtk_tree_model_foreach (model, default_gw_find_plip_cb, &cxn);

	return cxn;
}

extern void
connection_init_gui (XstTool *tool)
{
	XstConnectionUI *ui;
	GtkWidget *container;
	XstConnectionType i;

	ui = g_new0 (XstConnectionUI, 1);

	container = xst_dialog_get_widget (tool->main_dialog, "connection_list_sw");
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (container),
					     GTK_SHADOW_ETCHED_IN);
	ui->list = connection_list_new ();
	gtk_container_add (GTK_CONTAINER (container), ui->list);

	ui->def_gw_omenu = xst_dialog_get_widget (tool->main_dialog, "connection_def_gw_omenu");

	g_object_set_data (G_OBJECT (tool), CONNECTION_UI_STRING, (gpointer) ui);

	for (i = XST_CONNECTION_OTHER; i < XST_CONNECTION_LAST; i++)
		xst_iface_desc[i].pixbuf = load_pixbuf (xst_iface_desc[i].icon);
}

/* NULL if false, else GtkWidget in data in found node */
static GtkWidget *
connection_default_gw_find_item (GtkWidget *omenu, gchar *dev)
{
	GList *l;
	gchar *value;

	for (l = g_object_get_data (G_OBJECT (omenu), "list");
	     l; l = l->next) {
		value = g_object_get_data (G_OBJECT (l->data), "value");
		if (!strcmp (dev, value))
			return l->data;
	}

	return NULL;
}

static void
connection_default_gw_activate (GtkMenuItem *item, gpointer data)
{
	g_object_set_data (G_OBJECT (tool), "gatewaydev", data);
}

void
connection_default_gw_add (XstConnection *cxn)
{
	GtkWidget *omenu, *menu, *item;
	GList *l;
	gchar *cpy, *dev;
	XstConnectionUI *ui;

	dev = cxn->dev;
	
	if (cxn->type == XST_CONNECTION_LO)
		return;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	omenu = ui->def_gw_omenu;
	menu  = gtk_option_menu_get_menu (GTK_OPTION_MENU (omenu));

	if (connection_default_gw_find_item (omenu, dev))
		return;
	
	cpy = g_strdup (dev);
	item = gtk_menu_item_new_with_label (dev);
	g_signal_connect (G_OBJECT (item), "activate",
			    G_CALLBACK (connection_default_gw_activate),
			    (gpointer) cpy);
	g_object_set_data (G_OBJECT (item), "value", cpy);
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	
	l = g_object_get_data (G_OBJECT (omenu), "list");
	l = g_list_append (l, item);
	g_object_set_data (G_OBJECT (omenu), "list", l);
}

void
connection_default_gw_remove (gchar *dev)
{
	GtkWidget *omenu, *menu, *item;
	GList *l;
	gchar *cpy;
	XstConnectionUI *ui;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);

	omenu = ui->def_gw_omenu;
	menu  = gtk_option_menu_get_menu (GTK_OPTION_MENU (omenu));

	g_return_if_fail ((item = connection_default_gw_find_item (omenu, dev)));
	
	l = g_object_get_data (G_OBJECT (omenu), "list");
	l = g_list_remove (l, item);
	g_object_set_data (G_OBJECT (omenu), "list", l);

	cpy = g_object_get_data (G_OBJECT (item), "value");
	g_free (cpy);
	
	gtk_widget_destroy (item);
}

void
connection_default_gw_init (XstTool *tool, gchar *dev)
{
	GtkWidget *omenu, *menu, *item;
	XstConnectionUI *ui;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);

	omenu = ui->def_gw_omenu;
	menu  = gtk_option_menu_get_menu (GTK_OPTION_MENU (omenu));

	item = gtk_menu_get_active (GTK_MENU (menu));
	g_signal_connect (G_OBJECT (item), "activate",
			    G_CALLBACK (connection_default_gw_activate),
			    (gpointer) NULL);
	/* Strange bug caused the "Auto" item to be unsensitive the first time. */
	gtk_menu_shell_select_item (GTK_MENU_SHELL (menu), item);
	
	item = connection_default_gw_find_item (omenu, dev);
	if (item) {
		GList *l;

		l = g_object_get_data (G_OBJECT (omenu), "list");
		
		gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), g_list_index (l, item) + 1);
		gtk_menu_shell_select_item (GTK_MENU_SHELL (menu), item);
	}
}

XstConnection *
connection_default_gw_get_connection (XstTool *tool)
{
	gchar *dev;
	XstConnectionUI *ui;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	dev = g_object_get_data (G_OBJECT (tool), "gatewaydev");
	g_return_val_if_fail (dev != NULL, NULL);

	return connection_find_by_dev (ui->list, dev);
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
		if ((cxn->ip_config == IP_MANUAL) &&
		    (!cxn->gateway || !*cxn->gateway))
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

	gateway = g_object_get_data (G_OBJECT (tool), "gateway");
	if (gateway)
		g_free (gateway);
	gateway = NULL;

	if (!cxn) {
		g_object_set_data (G_OBJECT (tool), "gateway", NULL);
		g_object_set_data (G_OBJECT (tool), "gatewaydev", NULL);
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
		if (cxn->remote_address && *cxn->remote_address)
			gateway = g_strdup (cxn->remote_address);
		break;
	case XST_CONNECTION_PPP:
		g_object_set_data (G_OBJECT (tool), "gatewaydev", NULL);
		break;
	case XST_CONNECTION_LO:
	default:
		g_object_set_data (G_OBJECT (tool), "gatewaydev", NULL);
		g_warning ("connection_default_gw_set_manual: shouldn't be here.");
		break;
	}

	g_object_set_data (G_OBJECT (tool), "gateway", gateway);
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
connection_activate (XstConnection *cxn, gboolean activate)
{
	g_return_if_fail (cxn != NULL);

	if (cxn->enabled == activate)
		return;

	cxn->enabled = activate;
	connection_list_append (cxn);
	xst_dialog_modify (tool->main_dialog);
}

void
connection_add_to_list (XstConnection *cxn)
{
	g_return_if_fail (cxn != NULL);

	connection_list_append (cxn);
	connection_default_gw_add (cxn);
}

static void
connection_update_complexity_advanced (XstTool *tool)
{
	XstConnection   *cxn;
	XstConnectionUI *ui;
	GtkTreeModel    *model;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	cxn = g_object_steal_data (G_OBJECT (model), "lo");
	if (cxn)
		connection_add_to_list (cxn);
}

static gboolean
update_complexity_basic_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	XstConnection *cxn;

	gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, &cxn, -1);
	if (cxn->type == XST_CONNECTION_LO) {
		g_object_set_data (G_OBJECT (model), "lo", cxn);
		gtk_list_store_remove (GTK_LIST_STORE (model), iter);
		return TRUE;
	}

	return FALSE;
}

static void
connection_update_complexity_basic (XstTool *tool)
{
	XstConnectionUI *ui;
	GtkTreeModel    *model;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	gtk_tree_model_foreach (model, update_complexity_basic_cb, NULL);
}

void
connection_update_complexity (XstTool *tool, XstDialogComplexity complexity)
{
	switch (complexity) {
	case XST_DIALOG_BASIC:
		connection_update_complexity_basic (tool);
		break;
	case XST_DIALOG_ADVANCED:
		connection_update_complexity_advanced (tool);
	}
}

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

static gchar *
connection_find_new_device (xmlNode *root, XstConnectionType type)
{
	xmlNode *node;
	gchar *prefix, *dev, *numstr;
	gint max;
	gchar *devs[] = {
		NULL,
		"eth",
		"wvlan",
		"ppp",
		"plip",
		"irlan",
		"lo",
		NULL
	};

	prefix = devs[(gint) type];
	if (!prefix)
		return g_strdup ("NIL");

	max = -1;

	for (node = xst_xml_element_find_first (root, "interface");
	     node; node = xst_xml_element_find_next (node, "interface")) {
		dev = xst_xml_get_child_content (node, "dev");
		if (dev) {
			if (strstr (dev, prefix) == dev) {
				numstr = dev + strlen (prefix);
				if (*numstr && max < atoi (numstr))
					max = atoi (numstr);
			}

			g_free (dev);
		}
	}

	max++;

	return g_strdup_printf ("%s%u", prefix, max);
}					

XstConnection *
connection_new_from_type (XstConnectionType type, xmlNode *root)
{
	XstConnection *cxn;

	cxn = g_new0 (XstConnection, 1);
	cxn->type = type;
	cxn->file = NULL;

	cxn->activation = ACTIVATION_NONE;
	cxn->bulb_state = FALSE;
	
	/* set up some defaults */
	cxn->user = FALSE;
	cxn->autoboot = TRUE;
	cxn->update_dns = TRUE;
	cxn->ip_config = cxn->tmp_ip_config = IP_MANUAL;
	
	cxn->dev = connection_find_new_device (root, cxn->type);

	switch (cxn->type) {
	case XST_CONNECTION_PPP:
		cxn->user = TRUE;
		cxn->autoboot = FALSE;
		break;
	case XST_CONNECTION_PLIP:
		cxn->autoboot = FALSE;
		break;
	case XST_CONNECTION_ETH:
	case XST_CONNECTION_WVLAN:
	case XST_CONNECTION_IRLAN:
	case XST_CONNECTION_LO:
	default:
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
		cxn->ip_config = connection_config_type_from_str (s);
		cxn->tmp_ip_config = cxn->ip_config;
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

	connection_add_to_list (cxn);

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

		message = gtk_message_dialog_new (cxn->window ? GTK_WINDOW (tool->main_dialog) : NULL,
						  GTK_DIALOG_MODAL,
						  GTK_MESSAGE_ERROR,
						  GTK_BUTTONS_OK,
						  error);

		gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);
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
	connection_list_append (cxn);

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
	gtk_widget_destroy (cxn->window);

	if (cxn->creating) {
		connection_list_remove (cxn);
		connection_free (cxn);
		xst_dialog_modify (tool->main_dialog);
	}
}

static void
on_connection_config_dialog_destroy (GtkWidget *w, XstConnection *cxn)
{
	g_object_unref (G_OBJECT (cxn->xml));
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

	cxn = g_object_get_data (G_OBJECT (w), "user_data");

	if (cxn->frozen)
		return;

	ip = GPOINTER_TO_INT (data);

	if (cxn->tmp_ip_config == ip)
		return;

	cxn->tmp_ip_config = ip;

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
		g_object_set_data (G_OBJECT (menuitem), "user_data", cxn);
		g_signal_connect (G_OBJECT (menuitem), "activate",
				  G_CALLBACK (ip_config_menu_cb),
				  GINT_TO_POINTER (i));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
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
#warning FIXME
#if 0	
	gnome_entry_load_history (GNOME_ENTRY (W ("ppp_serial_port_g")));
#endif	
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
		{ "on_ip_netmask_focus_out", on_ip_address_focus_out },
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


/**
 * connection_dialog_set_visible_pages:
 * @cxn: 
 * 
 * Hides and shows the pages from the connection dialog notebook according to the
 * connection type. It also sets the the icons for the pages depending on the
 * connection type.
 **/
static void
connection_dialog_set_visible_pages (XstConnection *cxn)
{
	GtkNotebook *nb;
	
	nb = GTK_NOTEBOOK (W ("connection_nb"));

	if (cxn->type == XST_CONNECTION_PPP) {
		gtk_image_set_from_file (GTK_IMAGE (W ("connection_pixmap")), PIXMAPS_DIR "/ppp.png");
		fill_ppp (cxn);
		fill_ppp_adv (cxn);
		gtk_notebook_remove_page (nb,
					  gtk_notebook_page_num (nb,
								 W ("ip_vbox")));
		callbacks_check_dialer (GTK_WINDOW (cxn->window), tool);
	} else {
		gtk_notebook_remove_page (nb,
					  gtk_notebook_page_num (nb,
								 W ("ppp_vbox")));
		gtk_notebook_remove_page (nb,
					  gtk_notebook_page_num (nb,
								 W ("ppp_adv_vbox")));
	}
       
	if (cxn->type == XST_CONNECTION_WVLAN) {
		gtk_image_set_from_file (GTK_IMAGE (W ("connection_pixmap")), PIXMAPS_DIR "/wavelan-48.png");
		fill_wvlan (cxn);
		/* FIXME: temprorarily disabling this notebook, until we get support for this. */
		gtk_notebook_remove_page (nb,
					  gtk_notebook_page_num (nb,
								 W ("wvlan_vbox")));
	} else
		gtk_notebook_remove_page (nb,
					  gtk_notebook_page_num (nb,
								 W ("wvlan_vbox")));

	if (cxn->type == XST_CONNECTION_PLIP) {
		gtk_image_set_from_file (GTK_IMAGE (W ("connection_pixmap")), PIXMAPS_DIR "/plip-48.png");
		fill_ptp (cxn);
		gtk_notebook_remove_page (nb,
					  gtk_notebook_page_num (nb,
								 W ("ip_vbox")));
	} else 
		gtk_notebook_remove_page (nb,
					  gtk_notebook_page_num (nb,
								 W ("ptp_vbox")));

	if (cxn->type == XST_CONNECTION_ETH) {
		gtk_image_set_from_file (GTK_IMAGE (W ("connection_pixmap")),
					 PIXMAPS_DIR "/connection-ethernet.png");
	}

	if (cxn->type == XST_CONNECTION_IRLAN) {
		gtk_image_set_from_file (GTK_IMAGE (W ("connection_pixmap")), PIXMAPS_DIR "/irda-48.png");
	}

	if (cxn->type == XST_CONNECTION_LO) {
		gtk_widget_hide (W("ip_update_dns"));
		gtk_widget_hide (W("ip_bootproto_box"));
	}
}

void
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
	
	if (state == FALSE)
	{
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "connection_configure", TRUE);
	}
}

void
connection_configure (XstConnection *cxn)
{
	gchar *s;

	if (cxn->window) {
		gtk_widget_show (cxn->window);
		return;
	}

	g_assert (!cxn->xml);

	cxn->frozen = TRUE;

	s = g_strconcat (INTERFACES_DIR, "/", "network.glade", NULL);
	cxn->xml = glade_xml_new (s, "connection_config_dialog", NULL);

	g_assert (cxn->xml);
	g_free (s);

	hookup_callbacks (cxn);

	cxn->window = W("connection_config_dialog");
	cxn->tmp_ip_config = cxn->ip_config;

	fill_general (cxn);
	fill_ip      (cxn);

	if (!xst_xml_element_get_boolean (cxn->node->parent, "smartdhcpcd"))
		gtk_widget_hide (W("ip_update_dns"));
	if (!xst_xml_element_get_boolean (cxn->node->parent, "userifacfectl"))
		gtk_widget_hide (W("status_user"));

	connection_dialog_set_visible_pages (cxn);

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
	case XST_CONNECTION_IRLAN:
		return TRUE;
	case XST_CONNECTION_PLIP:
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

gboolean
connection_list_has_dialer (XstTool *tool)
{
	XstConnectionUI *ui;
	XstConnection   *cxn;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gboolean         valid;
	xmlNodePtr       root;
	gboolean         has_dialer = FALSE;
	gboolean         need_dialer = FALSE;


	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	root = xst_xml_doc_get_root (tool->config);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, CONNECTION_LIST_COL_DATA, &cxn, -1);
		if (cxn && cxn->type == XST_CONNECTION_PPP) {
			need_dialer = TRUE;
			break;
		}

		valid = gtk_tree_model_iter_next (model, &iter);
	}

	if (!need_dialer)
		return TRUE;

	has_dialer = (gboolean) g_object_get_data (G_OBJECT (tool), "dialinstalled");

	return has_dialer;
}

static gboolean
list_save_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	XstConnection *cxn;

	gtk_tree_model_get (model, iter, CONNECTION_LIST_COL_DATA, &cxn, -1);
	connection_save_to_node (cxn, (xmlNode *)data);

	return FALSE;
}

void
connection_list_save (XstTool *tool)
{
	XstConnectionUI *ui;
	XstConnection   *cxn;
	GtkTreeModel    *model;
	xmlNodePtr       root;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	root = xst_xml_doc_get_root (tool->config);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui->list));

	gtk_tree_model_foreach (model, list_save_cb, root);

	cxn = g_object_get_data (G_OBJECT (model), "lo");
	if (cxn)
		connection_save_to_node (cxn, root);
}

void
connection_list_select_connection (XstConnection *cxn)
{
	XstConnectionUI  *ui;
	GtkTreeSelection *select;
	GtkTreeIter       iter;

	ui = (XstConnectionUI *)g_object_get_data (G_OBJECT (tool), CONNECTION_UI_STRING);
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui->list));
	if (connection_iter (cxn, &iter))
		gtk_tree_selection_select_iter (select, &iter);
}
