/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __GST_HOSTS_H__
#define __GST_HOSTS_H__

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

#define STATICHOST_UI_STRING "statichost"

enum {
	STATICHOST_LIST_COL_IP,
	STATICHOST_LIST_COL_ALIAS,

	STATICHOST_LIST_COL_LAST
};

typedef struct {
	GtkWidget *list;
	GtkWidget *ip;
	GtkWidget *alias;
	GtkWidget *button_delete;
	GtkWidget *button_add;
} GstStatichostUI;

void gst_hosts_update_sensitivity (void);

void on_hosts_changed        (GtkWidget *w, gpointer null);
void on_hosts_add_clicked    (GtkWidget * button, gpointer user_data);
void on_hosts_delete_clicked (GtkWidget * button, gpointer user_data);
void on_hosts_update_clicked (GtkWidget *b, gpointer null);

void on_hosts_ip_changed     (GtkEditable *w, gpointer not_used);

void hosts_init_gui          (GstTool *tool);
void hosts_list_append       (GstTool *tool, const gchar *text[]);
void hosts_list_remove       (GstTool *tool, const gchar *ip);
gboolean hosts_list_get_selected (gchar **ip, gchar **alias);
void hosts_list_save         (GstTool *tool, xmlNodePtr root);
void hosts_list_clear        (GstTool *tool);

gboolean gst_hosts_ip_is_in_list (const gchar*);

#endif /* __GST_HOSTS_H__ */
