/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Hans Petter Jansson <hpj@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <gnome.h>
#include "gst-xml.h"
#include "connection.h"

typedef enum {
	EF_ALLOW_NONE  = 0,
	EF_ALLOW_TEXT  = 1 << 0,
	EF_ALLOW_ENTER = 1 << 1,
	EF_ALLOW_SPACE = 1 << 2,
	EF_ALLOW_IP    = 1 << 3
} EditableFilterRules;

typedef enum {
	IP_UNK,
	IP_V4,
	IP_V6,
	IP_LAST
} IpVersion;

extern xmlDocPtr doc;

void init_editable_filters (GstDialog *dialog);
void init_hint_entries (void);
void on_network_admin_show (GtkWidget *w, gpointer null);

extern void on_network_notebook_switch_page (GtkWidget *notebook, 
					     GtkNotebookPage *page,
					     gint page_num, gpointer user_data);

void filter_editable (GtkEditable *e, const gchar *text, 
		      gint length, gint *pos, gpointer data);

#define connect_editable_filter(w, r) g_signal_connect (G_OBJECT (w), "insert_text", \
			                                  G_CALLBACK (filter_editable), \
			                                  GINT_TO_POINTER (r))


/* libglade callbacks */
void on_network_notebook_switch_page (GtkWidget *notebook, 
				      GtkNotebookPage *page,
				      gint page_num, gpointer user_data);


gint update_hint (GtkWidget *w, GdkEventFocus *e, gpointer null);

void on_connection_add_clicked (GtkWidget *w, gpointer null);
void on_connection_delete_clicked (GtkWidget *w, gpointer null);
void on_connection_configure_clicked (GtkWidget *w, gpointer null);
void on_connection_activate_clicked (GtkWidget *w, gpointer null);
void on_connection_deactivate_clicked (GtkWidget *w, gpointer null);
void on_dns_dhcp_toggled (GtkWidget *w, gpointer null);
void on_samba_use_toggled (GtkWidget *w, gpointer null);
void on_wins_use_toggled (GtkWidget *w, gpointer null);

void on_status_button_toggled (GtkWidget *w, gpointer null);

void callbacks_check_dialer (GtkWindow *window, GstTool *tool);

gboolean callbacks_check_hostname_hook     (GstDialog *dialog, gpointer data);
gboolean callbacks_update_connections_hook (GstDialog *dialog, gpointer data);
gboolean callbacks_check_dialer_hook       (GstDialog *dialog, gpointer data);
gboolean callbacks_check_gateway_hook      (GstDialog *dialog, gpointer data);

gboolean callbacks_tool_not_found_hook     (GstTool *tool, GstReportLine *rline, gpointer data);

/* connection callbacks */
void on_connection_list_clicked (GtkWidget*, gpointer);
void on_connection_ok_clicked (GtkWidget*, GstConnection*);
void on_connection_cancel_clicked (GtkWidget*, GstConnection*);
void on_connection_config_dialog_destroy (GtkWidget*, GstConnection*);
gint on_connection_config_dialog_delete_event (GtkWidget*, GdkEvent*, GstConnection*);
void on_connection_modified (GtkWidget*, GstConnection*);
void on_wvlan_adhoc_toggled (GtkWidget*, GstConnection*);
void on_ppp_update_dns_toggled (GtkWidget*, GstConnection*);
gboolean on_ip_address_focus_out (GtkWidget*, GdkEventFocus*, GstConnection*);
void on_ppp_autodetect_modem_clicked (GtkWidget*, GstConnection*);

/* Hosts tab callbacks */
void on_hosts_ip_changed (GtkEditable*, gpointer);
void on_hosts_alias_changed (GtkTextBuffer*, gpointer);
void on_hosts_add_clicked (GtkWidget*, gpointer);
void on_hosts_delete_clicked (GtkWidget*, gpointer);

/* Network connection druid callbacks */
gboolean on_network_druid_hide (GtkWidget*, gpointer);
gboolean on_network_druid_page_next (GnomeDruidPage*, GnomeDruid*, gpointer);
gboolean on_network_druid_page_back (GnomeDruidPage*, GnomeDruid*, gpointer);
void on_network_druid_page_prepare  (GnomeDruidPage*, GnomeDruid*, gpointer);
void on_network_druid_finish        (GnomeDruidPage*, GnomeDruid*, gpointer);
void on_network_druid_entry_changed (GtkWidget*, gpointer);
void on_network_druid_config_type_changed (GtkWidget*, gpointer);
gboolean on_network_druid_ip_address_focus_out (GtkWidget*, GdkEventFocus*, gpointer);


/* network profiles callbacks */
void on_network_profiles_button_clicked (GtkWidget*, gpointer);
void on_network_profile_new_clicked (GtkWidget*, gpointer);
void on_network_profile_delete_clicked (GtkWidget*, gpointer);
void on_network_profile_table_selection_changed (GtkWidget*, gpointer);
void on_network_profile_option_selected (GtkWidget*, gpointer);

#endif /*  __CALLBACKS_H__  */
