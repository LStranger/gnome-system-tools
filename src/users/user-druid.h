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

#ifndef USER_DRUID_H
#define USER_DRUID_H

#include <gnome.h>
#include <glade/glade.h>

#include "xst.h"
#include "user_settings.h"

BEGIN_GNOME_DECLS

#define USER_DRUID_TYPE        (user_druid_get_type ())
#define USER_DRUID(o)          (GTK_CHECK_CAST ((o), USER_DRUID_TYPE, UserDruid))
#define USER_DRUID_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), USER_DRUID_TYPE, UserDruidClass))
#define USER_IS_DRUID(o)       (GTK_CHECK_TYPE ((o), USER_DRUID_TYPE))
#define USER_IS_DRUID_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), USER_DRUID_TYPE))

typedef struct {
	GtkWindow parent;
	
	GnomeDruid *druid;
	UserAccountGui *gui;
} UserDruid;

typedef struct {
	GtkWindowClass parent_class;
	
	/* signals */
	
} UserDruidClass;

GtkType user_druid_get_type (void);

UserDruid *user_druid_new (void);

END_GNOME_DECLS

#endif /* USER_DRUID_H */
