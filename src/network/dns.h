/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __GST_DNS_H__
#define __GST_DNS_H__

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
 * Authors: Carlos García Campos <elkalmail@yahoo.es>
 */

enum {
	DNS_SEARCH_LIST_COL,
	DNS_SEARCH_LIST_COL_LAST
};

enum {
	POPUP_NONE,
	POPUP_DELETE
};

void     dns_search_init_gui          (GstTool *tool);
void     dns_search_list_append       (GtkWidget *treeview, const gchar *text);
void     dns_search_list_remove       (GtkWidget *treeview, GtkWidget *entry);
gboolean gst_dns_search_is_in_list    (GtkWidget *list, const gchar *ip_str);
void     gst_dns_search_update_sensitivity (GtkWidget *list);

#endif /* __GST_DNS_H__ */
