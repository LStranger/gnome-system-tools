/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2003 Ximian, Inc.
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

#ifndef NETWORK_DRUID_H
#define NETWORK_DRUID_H

#include <gnome.h>
#include <glade/glade.h>

#include "gst.h"
#include "connection.h"

typedef enum {
	NETWORK_DRUID_START,
	NETWORK_DRUID_CONNECTION_TYPE,
	NETWORK_DRUID_WIRELESS,
	NETWORK_DRUID_OTHER_1,
	NETWORK_DRUID_PLIP_1,
	NETWORK_DRUID_PPP_1,
	NETWORK_DRUID_PPP_2,
	NETWORK_DRUID_ACTIVATE,
	NETWORK_DRUID_FINISH
} NetworkDruidPages;

typedef enum {
	DRUID_START_PAGE,
	DRUID_NORMAL_PAGE,
	DRUID_FINISH_PAGE
} NetworkDruidPageType;
	

typedef struct {
	GtkWidget *window;
	GstTool *tool;
	GstConnection *cxn;

	gboolean fixed_type; /* if the druid has a fixed GstConnectionType that cannot change */
	gint current_page;

	gint npages;
	gchar *first_page_title;
	gchar *title;
	gchar *finish_title;
} NetworkDruidData;

void network_druid_new (GnomeDruid*, GtkWidget*, GstTool*, GstConnectionType);
void network_druid_clear (GnomeDruid*, gboolean);
GstConnection* network_druid_get_connection_data (GnomeDruid*);
void network_druid_check_page (GnomeDruid*, NetworkDruidPages);
gboolean network_druid_set_page_next (GnomeDruid*);
gboolean network_druid_set_page_back (GnomeDruid*);
void network_druid_set_window_title (GnomeDruid*);

#endif /* NETWORK_DRUID_H */
