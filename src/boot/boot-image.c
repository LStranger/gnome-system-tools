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

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "xst.h"
#include "boot-image.h"
#include "e-table.h"

extern XstTool *tool;

static XstBootImageTypeTable boot_image_type_table[] = {
	{ N_("Unknown"),    TYPE_UNKNOWN },
	{ N_("Windows NT"), TYPE_WINNT },
	{ N_("Windows 9x"), TYPE_WIN9X },
	{ N_("Dos"),        TYPE_DOS },
	{ N_("Linux"),      TYPE_LINUX },
	{ NULL,             -1 }	
};

gint
boot_image_count (xmlNodePtr root)
{
	guint       count;
	xmlNodePtr  n;

	g_return_val_if_fail (root != NULL, -1);
	
	for (count = 0, n = xst_xml_element_find_first (root, "entry");
	     n != NULL;
	     count++, n = xst_xml_element_find_next (n, "entry"));

	return count;
}

BootImage *
boot_image_new (void)
{
	BootImage *image;
	guint      count;
	xmlNodePtr root;

	root = xst_xml_doc_get_root (tool->config);
	
	count = boot_image_count (root);
	if (count >= MAX_IMAGES || count < 0)
		return NULL;
	
	image = g_new0 (BootImage, 1);
	image->new = TRUE;
	image->node = root;

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
	image->is_default = boot_value_default (image->label);
	
	if (image->type == TYPE_LINUX) {
		image->image = boot_value_image (node, TRUE);
		image->root = boot_value_root (node);
		image->append = boot_value_append (node);
	} else
		image->image = boot_value_device (node, TRUE);
	
	return image;
}

void
boot_image_save (BootImage *image)
{
	xmlNodePtr  node;
	gchar      *error;
	
	g_return_if_fail (image != NULL);	

	error = boot_image_check (image);
	if (error != NULL) {
		g_warning ("boot_image_save: Image has error '%s', not saving.", error);
		g_free (error);
		return;
	}

	if (image->new)
		image->node = boot_table_add ();
	
	node = image->node;

	if (image->is_default)
		boot_value_set_default (node);
	
	boot_value_set_type (node, image->type);
	boot_value_set_label (node, image->label);
	boot_value_set_image (node, image->image, image->type);
	boot_value_set_root (node, image->root);
	boot_value_set_append (node, image->append);
}

void
boot_image_destroy (BootImage *image)
{
	if (image) {
		if (image->label)  g_free (image->label);
		if (image->image)  g_free (image->image);
		if (image->root)   g_free (image->root);
		if (image->append) g_free (image->append);
	
		g_free (image);
	}
}

/* Checking functions */

static gboolean
boot_image_valid_chars (const gchar *string)
{
	gchar *s;

	s = string;
	while (*s != '\0') {
		if (isalnum (*s) || *s == '-' || *s == '/' || *s == '.' || *s == '_') 
			s++;
		else
			return FALSE;
	}
	
	return TRUE;
}

static gboolean
boot_image_label_exists (BootImage *image)
{
	xmlNodePtr  root;
	xmlNodePtr  node;
	gchar      *buf;
	gboolean    found = FALSE;
		
	g_return_val_if_fail (image != NULL, FALSE);

	/* new (non-existing) images have node == parent */
	if (image->new)
		root = image->node;
	else
		root = image->node->parent;

	for (node = xst_xml_element_find_first (root, "entry");
	     node != NULL;
	     node = xst_xml_element_find_next (node, "entry")) {

		if (node == image->node)
			continue;

		buf = xst_xml_get_child_content (node, "label");
		if (buf) {
			if (strcmp (buf, image->label) == 0) {
				found = TRUE;
				g_free (buf);
				break;
			}

			g_free (buf);
		}
	}

	return found;
}

static gchar *
boot_image_file_exists (const gchar *string)
{
	gchar       *error = NULL;
	struct stat  s;

	if (stat (string, &s)) {
		if (errno) {
			error = g_strdup_printf (_("Invalid path: %s"), string);
		}
	}

	return error;
}

gchar *
boot_image_valid_label (BootImage *image)
{
	gchar *error = NULL;

	if (image->label == NULL || strlen (image->label) == 0)
		return NULL;

	if (boot_image_label_exists (image)) {
		error = g_strdup_printf (_("Image '%s' already exists, please set another name."),
					 image->label);
		return error;
	}
	
	if (!boot_image_valid_chars (image->label)) {
		error = g_strdup_printf (_("Invalid image name: '%s'"), image->label);
		return error;
	}


	
	return error;
}

gchar *
boot_image_valid_device (BootImage *image)
{
	gchar *error = NULL;

	if (!boot_image_valid_chars (image->image)) {
		error = g_strdup_printf (_("Invalid image: '%s'"), image->image);
		return error;
	}

	error = boot_image_file_exists (image->image);

	return error;
}

gchar *
boot_image_valid_root (BootImage *image)
{
	gchar *error = NULL;

	if (image->root == NULL || strlen (image->root) == 0)
		return NULL; /* Not required */
	
	if (!boot_image_valid_chars (image->root)) {
		error = g_strdup_printf (N_("Invalid root device: '%s'"), image->root);
		return error;
	}

	error = boot_image_file_exists (image->root);

	return error;
}

gchar *
boot_image_check (BootImage *image)
{
	gchar *error;
	
	error = boot_image_valid_label (image);
	if (error)
		return error;

	error = boot_image_valid_device (image);
	if (error)
		return error;

	error = boot_image_valid_root (image);
	if (error)
		return error;

	/* everything is fine, no errors */
	return NULL;
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
		list = g_list_prepend (list, _(boot_image_type_table[i].label));
	
	return list;
}
