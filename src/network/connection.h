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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <glade/glade.h>
#include <gtk/gtkwidget.h>

#include "xst.h"

typedef enum {
	XST_CONNECTION_OTHER = 0,
	XST_CONNECTION_ETH,
	XST_CONNECTION_WVLAN,
	XST_CONNECTION_PPP,
	XST_CONNECTION_PLIP,
	XST_CONNECTION_IRLAN,
	XST_CONNECTION_LO,
	XST_CONNECTION_UNKNOWN,
	XST_CONNECTION_LAST
} XstConnectionType;

typedef enum {
	XST_CONNECTION_ERROR_NONE = 0,
	XST_CONNECTION_ERROR_ENABLED,
	XST_CONNECTION_ERROR_PPP,
	XST_CONNECTION_ERROR_STATIC,
	XST_CONNECTION_ERROR_OTHER /* Always last */
} XstConnectionErrorType;

typedef enum {
        IP_MANUAL,
        IP_DHCP,
	IP_BOOTP
} IPConfigType;

typedef enum {
	ACTIVATION_NONE,
	ACTIVATION_UP,
	ACTIVATION_DOWN
} ActivationType;

typedef struct {
	GtkWidget *window;
	GladeXML *xml;
	xmlNode *node;
	
	XstConnectionType type;
	ActivationType    activation;
	gboolean          bulb_state;

	gboolean modified;
	gboolean creating;
	gboolean frozen;

	/* General */
	gchar *dev;
	gchar *name;
	gchar *file;

	gboolean enabled;
	gboolean user;
	gboolean autoboot;
	gboolean update_dns;

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
	gboolean noauth;
	gchar *serial_port;
	gchar *wvsection;
	gboolean stupid;
	gboolean set_default_gw;
	gchar *dns1;
	gchar *dns2;
	gchar *ppp_options;
	gint volume;
	gchar *dial_command;

	/* PtP (PLIP) */
	gchar *remote_address;
} XstConnection;

extern void connection_init_gui (XstTool *tool);
extern XstConnection *connection_new_from_node (xmlNode *node);
extern XstConnection *connection_new_from_dev_name (char *dev_name, xmlNode *root);
extern XstConnection *connection_new_from_type (XstConnectionType type, xmlNode *root);
extern XstConnection *connection_new_from_type_add (XstConnectionType type, xmlNode *root);
extern gchar *connection_get_serial_port_from_node (xmlNode *node, gchar *wvsection);
extern gchar *connection_wvsection_name_generate (gchar *dev, xmlNode *root);
extern void connection_add_to_list (XstConnection *cxn);
extern void connection_default_gw_add (XstConnection *cxn);
extern void connection_default_gw_remove (gchar *dev);
extern void connection_default_gw_init (XstTool *tool, gchar *dev);
extern XstConnection *connection_default_gw_get_connection (XstTool *tool);
extern XstConnectionErrorType connection_default_gw_check_manual (XstConnection *cxn, gboolean ignore_enabled);
extern void connection_default_gw_fix (XstConnection *cxn, XstConnectionErrorType error);
extern void connection_default_gw_set_manual (XstTool *tool, XstConnection *cxn);
extern void connection_default_gw_set_auto (XstTool *tool);
extern void connection_update_complexity (XstTool *tool, XstDialogComplexity complexity);
extern void connection_actions_set_sensitive (gboolean state);
extern void connection_free (XstConnection *);
extern void connection_configure (XstConnection *cxn);
extern void connection_save_to_node (XstConnection *cxn, xmlNode *node);

XstConnection *connection_list_get_active (void);
void           connection_list_remove     (XstConnection *cxn);
void           connection_activate        (XstConnection *cxn, gboolean activate);
void           connection_list_update     (void);
gboolean       connection_list_has_dialer (XstTool *tool);
void           connection_list_save       (XstTool *tool);
void           connection_list_select_connection (XstConnection *cxn);

#endif /* CONNECTION_H */
