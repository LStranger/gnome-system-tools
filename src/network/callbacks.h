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
#include <tree.h>

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

void init_editable_filters (XstDialog *dialog);
void init_hint_entries (void);
void on_network_admin_show (GtkWidget *w, gpointer null);

extern void on_network_notebook_switch_page (GtkWidget *notebook, 
					     GtkNotebookPage *page,
					     gint page_num, gpointer user_data);

void filter_editable (GtkEditable *e, const gchar *text, 
		      gint length, gint *pos, gpointer data);

#define connect_editable_filter(w, r) gtk_signal_connect (GTK_OBJECT (w), "insert_text", \
			                                  GTK_SIGNAL_FUNC (filter_editable), \
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
void on_connection_list_select_row (GtkCList *clist, gint row, gint column,
				    GdkEvent * event, gpointer user_data);
void on_connection_list_unselect_row (GtkCList *clist, gint row, gint column,
				      GdkEvent * event, gpointer user_data);
void on_dns_dhcp_toggled (GtkWidget *w, gpointer null);
void on_samba_use_toggled (GtkWidget *w, gpointer null);
void on_wins_use_toggled (GtkWidget *w, gpointer null);

void on_status_button_toggled (GtkWidget *w, gpointer null);

void callbacks_check_dialer (GtkWindow *window, XstTool *tool);

gboolean callbacks_check_hostname_hook     (XstDialog *dialog, gpointer data);
gboolean callbacks_update_connections_hook (XstDialog *dialog, gpointer data);
gboolean callbacks_check_dialer_hook       (XstDialog *dialog, gpointer data);
gboolean callbacks_check_gateway_hook      (XstDialog *dialog, gpointer data);

gboolean callbacks_tool_not_found_hook     (XstTool *tool, XstReportLine *rline, gpointer data);

#endif /*  __CALLBACKS_H__  */
