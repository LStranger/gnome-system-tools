/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* disks.c: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
 * 
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
 * Authors: Alvaro Peña Gonzalez <apg@esware.com>
 *          Carlos Garcia Campos <elkalmail@yahoo.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "gst.h"

#include "disks-config.h"
#include "disks-storage.h"

GstDisksConfig*
gst_disks_config_new (void)
{
	GstDisksConfig *cfg;

	cfg = g_new0 (GstDisksConfig, 1);

	cfg->storages = NULL;
	/*cfg->num_storages = 0;*/

	return cfg;
}

void 
gst_disks_config_add_storage (GstDisksConfig *cfg, GstDisksStorage *storage)
{
	g_return_if_fail (cfg != NULL);
	g_return_if_fail (GST_IS_DISKS_STORAGE (storage));
	
	cfg->storages = g_list_append (cfg->storages, (gpointer) storage);
	/*cfg->num_storages ++;*/
}
