/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-export-nfs.c: this file is part of shares-admin, a gnome-system-tool frontend 
 * for shared folders administration.
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
	GSList *acl;
};

static void gst_share_nfs_class_init    (GstShareNFSClass *klass);
static void gst_share_nfs_init          (GstShareNFS      *share);
static void gst_share_nfs_finalize      (GObject          *object);
static void gst_share_nfs_get_xml       (GstShare         *share,
					 xmlNodePtr        parent);

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
	GObjectClass  *object_class = G_OBJECT_CLASS (class);
	GstShareClass *share_class  = GST_SHARE_CLASS (class);

	parent_class = g_type_class_peek_parent (class);

	object_class->set_property = NULL;
	object_class->get_property = NULL;
	object_class->finalize     = gst_share_nfs_finalize;

	share_class->get_xml = gst_share_nfs_get_xml;
}

static void
gst_share_nfs_init (GstShareNFS *share)
{
	g_return_if_fail (GST_IS_SHARE_NFS (share));

	share->_priv = g_new0 (GstShareNFSPrivate, 1);
	share->_priv->acl = NULL;
}

static void
gst_share_nfs_finalize (GObject *object)
{
	GstShareNFS *share = GST_SHARE_NFS (object);

	g_return_if_fail (GST_IS_SHARE_NFS (share));

	if (share->_priv) {
		gst_share_nfs_clear_acl (share);

		g_free (share->_priv);
		share->_priv = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_share_nfs_get_xml (GstShare *share, xmlNodePtr parent)
{
	xmlNodePtr  node, allow;
	GSList     *list;
	GstShareACLElement *element;

	node = gst_xml_element_add (parent, "export");
	gst_xml_element_set_attribute (node, "type", "nfs");

	gst_xml_set_child_content (node, "path", gst_share_get_path (share));

	list = GST_SHARE_NFS (share)->_priv->acl;

	while (list) {
		element = list->data;
		
		allow = gst_xml_element_add (node, "allow");
		gst_xml_set_child_content (allow, "pattern", element->element);
		gst_xml_element_set_state (allow, "write", !element->read_only);

		list = g_slist_next (list);
	}
}

GstShareNFS*
gst_share_nfs_new (const gchar *path)
{
	GstShareNFS *share;

	share = g_object_new (GST_TYPE_SHARE_NFS,
			      "share-path", path,
			      NULL);

	return share;
}

GstShareNFS*
gst_share_nfs_new_from_xml (xmlNodePtr node)
{
	gchar       *type, *path, *pattern;
	xmlNodePtr   acl, boolnode;
	gboolean     read_only;
	GstShareNFS *share;

	type = gst_xml_element_get_attribute (node, "type");

	g_return_val_if_fail (type != NULL, NULL);
	g_return_val_if_fail (strcmp (type, "nfs") == 0, NULL);

	path = gst_xml_get_child_content (node, "path");
	share = gst_share_nfs_new (path);

	/* Feed the ACL */
	for (acl = gst_xml_element_find_first (node, "allow");
	     acl; acl = gst_xml_element_find_next (acl, "allow")) {
		pattern   = gst_xml_get_child_content (acl, "pattern");
		boolnode  = gst_xml_element_find_first (acl, "write");
		read_only = !gst_xml_element_get_bool_attr (boolnode, "state");

		gst_share_nfs_add_acl_element (share, pattern, read_only);

		g_free (pattern);
	}

	g_free (path);
	g_free (type);

	return share;
}

static void
acl_element_free (GstShareACLElement *element, gpointer data)
{
	g_free (element->element);
	g_free (element);
}

static void
clear_acl (GSList **list)
{
	g_slist_foreach (*list, (GFunc) acl_element_free, NULL);
	g_slist_free (*list);
	*list = NULL;
}

void
gst_share_nfs_add_acl_element (GstShareNFS *share,
			       const gchar *element,
			       gboolean     read_only)
{
	GstShareACLElement *elem;

	g_return_if_fail (share != NULL);
	g_return_if_fail (GST_IS_SHARE_NFS (share));
	
	elem = g_new0 (GstShareACLElement, 1);
	elem->element = g_strdup (element);
	elem->read_only = read_only;

	share->_priv->acl = g_slist_append (share->_priv->acl, elem);
}

void
gst_share_nfs_clear_acl (GstShareNFS *share)
{
	clear_acl (&share->_priv->acl);
}

void
gst_share_nfs_set_acl (GstShareNFS *share, GSList *acl)
{
	g_return_if_fail (share != NULL);
	g_return_if_fail (GST_IS_SHARE_NFS (share));

	gst_share_nfs_clear_acl (share);
	share->_priv->acl = acl;
}

const GSList*
gst_share_nfs_get_acl (GstShareNFS *share)
{
	g_return_val_if_fail (share != NULL, NULL);
	g_return_val_if_fail (GST_IS_SHARE_NFS (share), NULL);
	
	return share->_priv->acl;
}
