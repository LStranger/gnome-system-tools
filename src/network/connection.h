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

#include "gst.h"

typedef enum {
	GST_CONNECTION_OTHER = 0,
	GST_CONNECTION_ETH,
	GST_CONNECTION_WLAN,
	GST_CONNECTION_PPP,
	GST_CONNECTION_PLIP,
	GST_CONNECTION_IRLAN,
	GST_CONNECTION_LO,
	GST_CONNECTION_UNKNOWN,
	GST_CONNECTION_LAST
} GstConnectionType;

enum {
	CONNECTION_LIST_COL_STAT,
	CONNECTION_LIST_COL_DEV_PIX,
	CONNECTION_LIST_COL_DEV_TYPE,
	CONNECTION_LIST_COL_DEVICE,

	CONNECTION_LIST_COL_DATA,
	CONNECTION_LIST_COL_LAST
};

typedef enum {
	GST_CONNECTION_ERROR_NONE = 0,
	GST_CONNECTION_ERROR_ENABLED,
	GST_CONNECTION_ERROR_PPP,
	GST_CONNECTION_ERROR_STATIC,
	GST_CONNECTION_ERROR_OTHER /* Always last */
} GstConnectionErrorType;

typedef enum {
	GST_CONNECTION_VOLUME_NONE,
	GST_CONNECTION_VOLUME_LOW,
	GST_CONNECTION_VOLUME_MEDIUM,
	GST_CONNECTION_VOLUME_LOUD
} GstConnectionVolume;

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

enum {
	CONNECTION_POPUP_NONE,
	CONNECTION_POPUP_ADD,
	CONNECTION_POPUP_SEPARATOR,
	CONNECTION_POPUP_CONFIGURE,
	CONNECTION_POPUP_DELETE
};

typedef struct {
	xmlNode *node;
	
	GstConnectionType type;
	ActivationType    activation;

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
	gchar *essid;
	gchar *key;

	/* PPP */
	gchar *phone_number;
	gchar *external_line;
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
	GstConnectionVolume volume;
	gchar *dial_command;

	/* PtP (PLIP) */
	gchar *remote_address;
} GstConnection;

extern void connection_init_gui (GstTool *tool);
extern GstConnection *connection_new_from_node (xmlNodePtr, gboolean);
extern GstConnection *connection_new_from_dev_name (char *dev_name, xmlNode *root);
extern GstConnection *connection_new_from_type (GstConnectionType type, xmlNode *root);
extern GstConnection *connection_new_from_type_add (GstConnectionType type, xmlNode *root);
extern gchar *connection_get_serial_port_from_node (xmlNode *node, gchar *wvsection);
extern gchar *connection_wvsection_name_generate (gchar *dev, xmlNode *root);
extern void connection_add_to_list (GstConnection *cxn);
extern void connection_default_gw_add (GstConnection *cxn);
extern void connection_default_gw_remove (gchar *dev);
extern void connection_default_gw_init (GstTool *tool, gchar *dev);
extern GstConnection *connection_default_gw_get_connection (GstTool *tool);
extern GstConnectionErrorType connection_default_gw_check_manual (GstConnection *cxn, gboolean ignore_enabled);
extern void connection_default_gw_fix (GstConnection *cxn, GstConnectionErrorType error);
extern void connection_default_gw_set_manual (GstTool *tool, GstConnection *cxn);
extern void connection_default_gw_set_auto (GstTool *tool);
extern void connection_actions_set_sensitive (gboolean state);
extern void connection_free (GstConnection *);
extern void connection_configure (GstConnection *cxn);
extern void connection_configure_device (xmlNodePtr, gchar*);
extern void connection_save_to_node (GstConnection *cxn, xmlNode *node);

GstConnection *connection_list_get_active (void);
GstConnection *connection_list_get_by_path (GtkTreePath*);
void           connection_list_remove     (GstConnection *cxn);
void           connection_show_activated  (GstConnection *cxn, gboolean activate);
void           connection_list_update     (void);
gboolean       connection_list_has_dialer (GstTool *tool);
void           connection_list_save       (GstTool *tool);
void           connection_list_select_connection (GstConnection *cxn);
void           connection_list_clear      (GstTool *tool);

gboolean connection_config_save (GstConnection*, GtkWidget*, gboolean);
void     connection_check_netmask_gui (GtkWidget*, GtkWidget*);
void     connection_set_modified (GstConnection*, gboolean);

gchar *connection_find_new_device (xmlNodePtr, GstConnectionType);
void connection_set_bcast_and_network (GstConnection *cxn);

gboolean connection_poll_stat (GstTool*);
gchar*   connection_autodetect_modem (void);
gchar*   connection_description_from_type (GstConnectionType);
void     connection_fill_ip_menu (GtkWidget*);
void     connection_update_ip_config (GstConnection*);
void     connection_enable (GstConnection*, gboolean);
void     connection_apply_and_enable (GstTool*, GstConnection*);

gint connection_get_count (GstTool*);

#endif /* CONNECTION_H */
