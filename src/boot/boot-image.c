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

#include "xst.h"
#include "boot-image.h"
#include "e-table.h"

extern XstTool *tool;

static XstBootImageTypeTable boot_image_type_table[] = {
	{ N_("Unknown"), TYPE_UNKNOWN },
	{ N_("Windows NT"), TYPE_WINNT },
	{ N_("Windows 9x"), TYPE_WIN9X },
	{ N_("Dos"), TYPE_DOS },
	{ N_("Linux"), TYPE_LINUX },
	{ NULL, -1 }	
};

BootImage *
boot_image_new (void)
{
	BootImage *image;

	image = g_new0 (BootImage, 1);
	image->new = TRUE;

	return image;
}

BootImage *
boot_image_get_by_node (xmlNodePtr node)
{
	BootImage *image;

	g_return_val_if_fail (node != NULL, NULL);
	
	image = g_new0 (BootImage, 1);

	image->node = node;
	image->new = FALSE;

	image->type = boot_value_type (node);
	image->label = boot_value_label (node);	
	image->image = boot_value_image (node, TRUE);
	image->is_default = boot_value_default (image->label);
	
	if (image->type == TYPE_LINUX) {
		image->root = boot_value_root (node);
		image->append = boot_value_append (node);
	}
	
	return image;
}

void
boot_image_save (BootImage *image)
{
	xmlNodePtr node;
	
	g_return_if_fail (image != NULL);

	if (image->new)
		image->node = boot_table_add ();

	node = image->node;

	if (image->is_default)
		boot_value_set_default (node);
	
	boot_value_set_type (node, image->type);
	boot_value_set_label (node, image->label);
	boot_value_set_image (node, image->image);
	boot_value_set_root (node, image->root);
	boot_value_set_append (node, image->append);
}

void
boot_image_destroy (BootImage *image)
{
	if (image) {
		if (image->label) g_free (image->label);
		if (image->image) g_free (image->image);
		if (image->root) g_free (image->root);
		if (image->append) g_free (image->append);
	
		g_free (image);
	}
}

/* Helpers */

gchar *
type_to_label (XstBootImageType type)
{
	gint i;

	for (i = 0; boot_image_type_table[i].label; i++)
		if (type == boot_image_type_table[i].type)
			return g_strdup (boot_image_type_table[i].label);

	g_warning ("type_to_label: unknown type.");
	return g_strdup ("");
}

XstBootImageType
label_to_type (const gchar *label)
{
	gint i;

	for (i = 0; boot_image_type_table[i].label; i++)
		if (!strcmp (label, boot_image_type_table[i].label))
			return boot_image_type_table[i].type;

	g_warning ("label_to_type: unknown label.");
	return TYPE_UNKNOWN;
}

GList *
type_labels_list (void)
{
	gint i;
	GList *list = NULL;

	for (i = 0; boot_image_type_table[i].label; i++)
		list = g_list_prepend (list, boot_image_type_table[i].label);
	
	return list;
}
