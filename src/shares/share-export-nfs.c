/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export-nfs.c: this file is part of shares-admin, a gnome-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>.
 */

#include <config.h>
#include <glib.h>
#include "share-export.h"
#include "share-export-nfs.h"
#include "gst.h"

struct _GstShareNFSPrivate {
};

static void gst_share_nfs_class_init   (GstShareNFSClass *klass);
static void gst_share_nfs_init         (GstShareNFS *share);
static void gst_share_nfs_finalize     (GObject *object);

static gpointer parent_class;

GType
gst_share_nfs_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo share_nfs_info = {
			sizeof (GstShareNFSClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gst_share_nfs_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GstShareNFS),
			0,		/* n_preallocs */
			(GInstanceInitFunc) gst_share_nfs_init,
		};

		type = g_type_register_static (GST_TYPE_SHARE, "GstShareNFS",
					       &share_nfs_info, 0);
	}

	return type;
}

static void
gst_share_nfs_class_init (GstShareNFSClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->set_property = NULL;
	object_class->get_property = NULL;
	object_class->finalize     = gst_share_nfs_finalize;
}

static void
gst_share_nfs_init (GstShareNFS *share)
{
	g_return_if_fail (GST_IS_SHARE_NFS (share));

	share->_priv = g_new0 (GstShareNFSPrivate, 1);
}

static void
gst_share_nfs_finalize (GObject *object)
{
	GstShareNFS *share = GST_SHARE_NFS (object);

	g_return_if_fail (GST_IS_SHARE_NFS (share));

	if (share->_priv) {
		g_free (share->_priv);
		share->_priv = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

GstShareNFS*
gst_share_nfs_new (gchar            *name,
		   gchar            *comment,
		   gchar            *path)
{
	GstShareNFS *share;

	share = g_object_new (GST_TYPE_SHARE_NFS, NULL);

	gst_share_set_name    (GST_SHARE (share), name);
	gst_share_set_comment (GST_SHARE (share), comment);
	gst_share_set_path    (GST_SHARE (share), path);

	return share;
}

GstShareNFS*
gst_share_nfs_new_from_xml (xmlNodePtr node)
{
	gchar *type, *name, *comment, *path;

	type = gst_xml_element_get_attribute (node, "type");

	g_return_val_if_fail (type != NULL, NULL);
	g_return_val_if_fail (strcmp (type, "nfs") == 0, NULL);

	name    = gst_xml_get_child_content (node, "name");
	comment = gst_xml_get_child_content (node, "comment");
	path    = gst_xml_get_child_content (node, "path");

	return gst_share_nfs_new (name, comment, path);
}
