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
 * Authors: Tambet Ingo <tambet@ximian.com>
 *
 */

#ifndef BOOT_DRUID_H
#define BOOT_DRUID_H

#include <gnome.h>
#include <glade/glade.h>

#include "gst.h"
#include "boot-settings.h"

G_BEGIN_DECLS

#define BOOT_DRUID_TYPE        (boot_druid_get_type ())
#define BOOT_DRUID(o)          (GTK_CHECK_CAST ((o), BOOT_DRUID_TYPE, BootDruid))
#define BOOT_DRUID_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BOOT_DRUID_TYPE, BootDruidClass))
#define BOOT_IS_DRUID(o)       (GTK_CHECK_TYPE ((o), BOOT_DRUID_TYPE))
#define BOOT_IS_DRUID_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BOOT_DRUID_TYPE))

typedef struct {
	GtkWindow parent;
	
	GnomeDruid *druid;
	BootSettingsGui *gui;

	gint npages;
	gchar *druid_first_page_title;
	gchar *druid_title;
	gchar *druid_finish_title;
} BootDruid;

typedef struct {
	GtkWindowClass parent_class;
	
	/* signals */
	
} BootDruidClass;

GtkType boot_druid_get_type (void);

BootDruid *boot_druid_new (void);

G_END_DECLS

#endif /* BOOT_DRUID_H */
