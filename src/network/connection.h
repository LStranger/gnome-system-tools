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
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <tree.h>
#include <glade/glade.h>
#include <gtk/gtkwidget.h>

typedef enum {
	XST_CONNECTION_UNKNOWN = -1,
	XST_CONNECTION_OTHER,
	XST_CONNECTION_ETH,
	XST_CONNECTION_WVLAN,
	XST_CONNECTION_PPP,
	XST_CONNECTION_PLIP,
	XST_CONNECTION_LO,
	XST_CONNECTION_LAST
} XstConnectionType;

typedef enum {
        IP_MANUAL,
        IP_DHCP,
	IP_BOOTP
} IPConfigType;

typedef struct {
	GtkWidget *window;
	GladeXML *xml;
	xmlNode *node;
	
	XstConnectionType type;

	gboolean modified;
	gboolean frozen;

	/* General */
	gchar *dev;
	gchar *name;

	gboolean enabled;
	gboolean user;
	gboolean autoboot;
	gboolean dhcp_dns;

	/* IP Settings */
	IPConfigType ip_config;

	/* this is for the option menu because it sucks */
	IPConfigType tmp_ip_config;

	gchar *address;
	gchar *netmask;
	gchar *broadcast;
	gchar *network;
	gchar *gateway;

	/* Wavelan */
	gboolean adhoc;
	gchar *session_id;
	gint channel;
	gfloat frequency;

	/* PPP */
	gchar *phone_number;
	gchar *login;
	gchar *password;
	gboolean persist;
	gchar *serial_port;
	gchar *wvsection;
	gboolean stupid;
	gboolean set_default_gw;
	gboolean peerdns;
	gchar *dns1;
	gchar *dns2;
	gchar *ppp_options;
} XstConnection;

extern void connection_init_icons (void);
extern XstConnection *connection_new_from_node (xmlNode *node);
extern XstConnection *connection_new_from_dev_name (char *dev_name, xmlNode *root);
extern XstConnection *connection_new_from_type (XstConnectionType type, xmlNode *root);
extern XstConnection *connection_new_from_type_add (XstConnectionType type, xmlNode *root);
extern gchar *connection_get_serial_port_from_node (xmlNode *node, gchar *wvsection);
extern gchar *connection_wvsection_name_generate (gchar *dev, xmlNode *root);

extern void connection_free (XstConnection *);

extern void connection_configure (XstConnection *cxn);

extern void connection_save_to_node (XstConnection *cxn, xmlNode *node);

#endif /* CONNECTION_H */
