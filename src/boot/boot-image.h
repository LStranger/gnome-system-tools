/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* boot-image.h: this file is part of boot-admin, a ximian-setup-tool frontend 
 * for boot administration.
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#ifndef BOOT_IMAGE_H
#define BOOT_IMAGE_H

#include <gnome.h>
#include <gnome-xml/tree.h>

#include "xst.h"

typedef enum {
	TYPE_UNKNOWN,
	TYPE_WINNT,
	TYPE_WIN9X,
	TYPE_DOS,
	TYPE_LINUX
} XstBootImageType;

typedef struct {
	gchar *label;
	XstBootImageType type;
} XstBootImageTypeTable;

typedef struct {
	xmlNodePtr node;
	gboolean new;
	XstBootImageType type;
	gboolean is_default;
	
	gchar *label;
	gchar *image;
	gchar *root;
	gchar *append;	       	
} BootImage;

BootImage *boot_image_new         (void);
BootImage *boot_image_get_by_node (xmlNodePtr node);
void       boot_image_save        (BootImage *image);
void       boot_image_destroy     (BootImage *image);

/* Helpers */
gchar            *type_to_label          (XstBootImageType type);
XstBootImageType  label_to_type          (const gchar *label);
GList            *type_labels_list       (void);
#endif /* BOOT_IMAGE_H */
