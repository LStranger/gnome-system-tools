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
	CONNECTION_UNKNOWN = -1,
	CONNECTION_OTHER,
	CONNECTION_ETH,
	CONNECTION_WVLAN,
	CONNECTION_PPP,
	CONNECTION_PLIP,
	CONNECTION_LO,
	CONNECTION_LAST
} ConnectionType;

typedef enum {
        IP_MANUAL,
        IP_DHCP,
	IP_BOOTP
} IPConfigType;

typedef struct {
	GtkWidget *window;
	GladeXML *xml;
	xmlNode *node;
	
	ConnectionType type;

	gboolean modified;
	gboolean frozen;

	/* General */
	gchar *file;
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
} Connection;

extern void connection_init_icons (void);
extern Connection *connection_new_from_node (xmlNode *node);
extern Connection *connection_new_from_dev_name (char *dev_name);
extern Connection *connection_new_from_type (ConnectionType type);

extern void connection_free (Connection *);

extern void connection_configure (Connection *cxn);

extern void connection_save_to_node (Connection *cxn, xmlNode *node);

#endif /* CONNECTION_H */
