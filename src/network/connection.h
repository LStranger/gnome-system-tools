/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#ifndef __CONNECTION_H
#define __CONNECTION_H

#include "network-iface.h"

typedef struct _GstConnectionDialog GstConnectionDialog;

struct _GstConnectionDialog {
  GstIface  *iface;
  gboolean   changed;
  gboolean   standalone;
  GtkWidget *dialog;

  /* notebook pages */
  GtkWidget *notebook;
  GtkWidget *general_page;
  GtkWidget *ppp_general_page;
  GtkWidget *account_page;
  GtkWidget *options_page;

  GtkWidget *connection_configured;
  GtkWidget *connection_device;
  GtkWidget *connection_ppp_configured;
  GtkWidget *connection_ppp_device;

  /* ethernet */
  GtkWidget *bootproto_combo;
  GtkWidget *address;
  GtkWidget *netmask;
  GtkWidget *gateway;

  /* wireless */
  GtkWidget *essid;
  GtkWidget *wep_key;

  /* plip */
  GtkWidget *local_address;
  GtkWidget *remote_address;

  /* modem */
  GtkWidget *login;
  GtkWidget *password;
  GtkWidget *serial_port;
  GtkWidget *detect_modem;
  GtkWidget *phone_number;
  GtkWidget *dial_prefix;
  GtkWidget *volume;
  GtkWidget *dial_type;
  GtkWidget *default_gw;
  GtkWidget *persist;

  /* frames */
  GtkWidget *wireless_frame;
  GtkWidget *ethernet_frame;
  GtkWidget *plip_frame;
  GtkWidget *modem_frame;

  GtkWidget *ok_button;
};

GstConnectionDialog *connection_dialog_init (void);
void connection_dialog_prepare    (GstConnectionDialog*, GstIface*);
void connection_save (GstConnectionDialog*);
void connection_check_fields (GstConnectionDialog*);
void connection_check_netmask (GtkWidget*, GtkWidget*);
gchar *connection_detect_modem (void);

#endif /* __CONNECTION_H */
