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

#include "tool.h"

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

static void
connection_set_modified (Connection *cxn, gboolean state)
{
	if (cxn->frozen || !tool_get_access ())
		return;

	gtk_widget_set_sensitive (W ("connection_apply"), state);
	cxn->modified = state;
}

void
init_icons (void)
{
	ConnectionType i;
	GdkPixbuf *pb, *pb2;
	char *path;
	char *icons[CONNECTION_LAST] = {
		"networking.png",
		"networking.png",
		"gnome-laptop.png",
		"gnome-modem.png"
	};

	for (i = CONNECTION_OTHER; i < CONNECTION_LAST; i++) {
		path = g_concat_dir_and_file (PIXMAPS_DIR, icons[i]);
		
		pb = gdk_pixbuf_new_from_file (path);
		g_free (path);
		if (!pb) 
			continue;
		
		pb2 = gdk_pixbuf_scale_simple (pb, 20, 20, GDK_INTERP_BILINEAR);
		gdk_pixbuf_unref (pb);
		
		gdk_pixbuf_render_pixmap_and_mask (pb2, &mini_pm[i], &mini_mask[i], 127);
		gdk_pixbuf_unref (pb2);
	}
}

static void
update_row (Connection *cxn)
{
	GtkWidget *clist;
	int row;
	char *s;

	clist = tool_widget_get ("connection_list");

	s = g_strdup_printf ("%s: %s", cxn->device, cxn->description);
	
	row = gtk_clist_find_row_from_data (GTK_CLIST (clist), cxn);
	
	gtk_clist_set_pixtext (GTK_CLIST (clist), row, 1, s, GNOME_PAD_SMALL,
			       mini_pm[cxn->type], mini_mask[cxn->type]);

	tool_set_modified (TRUE);

	g_free (s);
}

static void
add_connection_to_list (Connection *cxn)
{
	GtkWidget *clist;
	int row;
	char *text[3] = { NULL };

	clist = tool_widget_get ("connection_list");

	row = gtk_clist_append (GTK_CLIST (clist), text);
	gtk_clist_set_row_data (GTK_CLIST (clist), row, cxn);

	update_row (cxn);
}

Connection *
connection_new_from_node (xmlNode *node)
{
#warning FIXME: implement
	return NULL;
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
		cxn->description = g_strdup (_("New Ethernet connection"));
		break;
	case CONNECTION_WVLAN:
		cxn->device = g_strdup ("wvlan0");
		cxn->description = g_strdup (_("New WaveLAN connection"));
		break;
	case CONNECTION_PPP:
		cxn->device = g_strdup ("ppp0");
		cxn->description = g_strdup (_("New PPP connection"));
		break;
	default:
		cxn->device = g_strdup ("???");
		cxn->description = g_strdup ( _("New unknown connection"));
		break;
	}	

	/* set up some defaults */
	cxn->autoboot = TRUE;
	cxn->dhcp_dns = TRUE;
	cxn->ip_config = IP_DHCP;

	cxn->ip      = g_strdup ("10.0.1.10");
	cxn->subnet  = g_strdup ("255.255.0.0");
	cxn->gateway = g_strdup ("10.0.1.1");

	add_connection_to_list (cxn);
	
	return cxn;
}

void
connection_free (Connection *cxn)
{
#warning FIXME: implement
}

void
on_status_button_clicked (GtkWidget *w, Connection *cxn)
{
	
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
	cxn->dhcp_dns = GTK_TOGGLE_BUTTON (W ("status_dhcp"))->active;
}

static void
empty_ip (Connection *cxn)
{
	cxn->ip_config = cxn->tmp_ip_config;

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

	gnome_pixmap_load_file (GNOME_PIXMAP (W ("status_icon")),
				cxn->active
				? PIXMAPS_DIR "/gnome-light-on.png"
				: PIXMAPS_DIR "/gnome-light-off.png");

	gtk_label_set_text (GTK_LABEL (W("status_label")),
			    cxn->active
			    ? _("This device is currently active.")
			    : _("This device is not currently active."));

	gtk_label_set_text (GTK_LABEL (W("status_button_label")),
			    cxn->active
			    ? _("Deactivate")
			    : _("Activate"));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_boot")), cxn->autoboot);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (W ("status_dhcp")), cxn->dhcp_dns);
}

static void
fill_ip (Connection *cxn)
{
	gtk_option_menu_set_history (GTK_OPTION_MENU (W ("connection_config")), cxn->ip_config);

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

	fill_general (cxn);
	fill_ip      (cxn);

	switch (cxn->type) {
	case CONNECTION_WVLAN:
		fill_wvlan (cxn);
		break;
	case CONNECTION_PPP:
		fill_ppp (cxn);
		break;
	default:
		break;
	}

	cxn->frozen = FALSE;

	connection_set_modified (cxn, FALSE);

	gtk_widget_show (cxn->window);
}

void
connection_save_to_node (Connection *cxn, xmlNode *node)
{
#warning FIXME: implement
}

